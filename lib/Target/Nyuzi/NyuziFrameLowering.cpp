//===-- NyuziFrameLowering.cpp - Nyuzi Frame Information
//------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Nyuzi implementation of TargetFrameLowering
// class.
//
//===----------------------------------------------------------------------===//

#include "NyuziFrameLowering.h"
#include "NyuziInstrInfo.h"
#include "NyuziMachineFunctionInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

const NyuziFrameLowering *NyuziFrameLowering::create(const NyuziSubtarget &ST) {
  return new NyuziFrameLowering(ST);
}

void NyuziFrameLowering::emitPrologue(MachineFunction &MF) const {
  MachineBasicBlock &MBB = MF.front();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  const NyuziInstrInfo &TII =
      *static_cast<const NyuziInstrInfo *>(MF.getSubtarget().getInstrInfo());
  MachineModuleInfo &MMI = MF.getMMI();
  const MCRegisterInfo *MRI = MMI.getContext().getRegisterInfo();
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  // Compute stack size. Allocate space, keeping SP 64 byte aligned so we
  // can do block vector load/stores
  int StackSize = RoundUpToAlignment(MFI->getStackSize(), getStackAlignment()); 

  // Bail if there is no stack allocation
  if (StackSize == 0 && !MFI->adjustsStack()) return;

  TII.adjustStackPointer(MBB, MBBI, -StackSize);

  // emit ".cfi_def_cfa_offset StackSize" (debug information)
  unsigned CFIIndex = MMI.addFrameInst(
      MCCFIInstruction::createDefCfaOffset(nullptr, -StackSize));
  BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION))
      .addCFIIndex(CFIIndex);

  // Find the instruction past the last instruction that saves a callee-saved
  // register to the stack.  We need to set up FP after its old value has been
  // saved.
  const std::vector<CalleeSavedInfo> &CSI = MFI->getCalleeSavedInfo();
  if (CSI.size()) {
    for (unsigned i = 0; i < CSI.size(); ++i)
      ++MBBI;

    // Iterate over list of callee-saved registers and emit .cfi_offset
    // directives (debug information)
    for (const auto &I : CSI) {
      int64_t Offset = MFI->getObjectOffset(I.getFrameIdx());
      unsigned Reg = I.getReg();
      unsigned CFIIndex = MMI.addFrameInst(MCCFIInstruction::createOffset(
          nullptr, MRI->getDwarfRegNum(Reg, 1), Offset));
      BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION))
          .addCFIIndex(CFIIndex);
    }
  }

  // fp = sp
  if (hasFP(MF)) {
    BuildMI(MBB, MBBI, DL, TII.get(Nyuzi::MOVESS))
        .addReg(Nyuzi::FP_REG)
        .addReg(Nyuzi::SP_REG);

    // emit ".cfi_def_cfa_register $fp" (debug information)
    unsigned CFIIndex = MMI.addFrameInst(MCCFIInstruction::createDefCfaRegister(
        nullptr, MRI->getDwarfRegNum(Nyuzi::FP_REG, true)));
    BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION))
        .addCFIIndex(CFIIndex);
  }
}

void NyuziFrameLowering::emitEpilogue(MachineFunction &MF,
                                           MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  const NyuziInstrInfo &TII =
      *static_cast<const NyuziInstrInfo *>(MF.getSubtarget().getInstrInfo());
  DebugLoc DL = MBBI->getDebugLoc();
  assert(MBBI->getOpcode() == Nyuzi::RET &&
         "Can only put epilog before 'retl' instruction!");

  // if framepointer enabled, restore the stack pointer.
  if (hasFP(MF)) {
    // Find the first instruction that restores a callee-saved register.
    MachineBasicBlock::iterator I = MBBI;
    for (unsigned i = 0; i < MFI->getCalleeSavedInfo().size(); ++i)
      --I;

    BuildMI(MBB, I, DL, TII.get(Nyuzi::MOVESS))
        .addReg(Nyuzi::SP_REG)
        .addReg(Nyuzi::FP_REG);
  }

  uint64_t StackSize = RoundUpToAlignment(MFI->getStackSize(), getStackAlignment());
  if (!StackSize)
    return;

  TII.adjustStackPointer(MBB, MBBI, StackSize);
}

// Returns true if the prologue inserter should reserve space for outgoing arguments 
// to called.  
bool NyuziFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  return !MF.getFrameInfo()->hasVarSizedObjects();
}

// We must use an FP in a few situations.  Note that this *must* return true if
// hasReservedCallFrame returns false.  Otherwise an ADJCALLSTACKDOWN could mess
// up frame offsets from the stack pointer.
bool NyuziFrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  return MF.getTarget().Options.DisableFramePointerElim(MF) 
    || MFI->hasVarSizedObjects() 
    || MFI->isFrameAddressTaken();
}

void NyuziFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI) const {
  MachineInstr &MI = *MBBI;

  const NyuziInstrInfo &TII =
      *static_cast<const NyuziInstrInfo *>(MF.getSubtarget().getInstrInfo());

  // Note the check for hasReservedCallFrame.  If it returns true, 
  // PEI::calculateFrameObjectOffsets has already reserved stack locations for 
  // these variables and we don't need to adjust the stack here.
  int Amount = MI.getOperand(0).getImm();
  if (Amount != 0 && !hasReservedCallFrame(MF)) {
    assert(hasFP(MF) && "Cannot adjust stack mid-function without a frame pointer");
    
    if (MI.getOpcode() == Nyuzi::ADJCALLSTACKDOWN)
      Amount = -Amount;

    TII.adjustStackPointer(MBB, MBBI, Amount);
  }

  MBB.erase(MBBI);
}

uint64_t NyuziFrameLowering::getWorstCaseStackSize(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();

  int64_t Offset = 0;

  // Iterate over fixed sized objects.
  for (int I = MFI->getObjectIndexBegin(); I != 0; ++I)
    Offset = std::max(Offset, -MFI->getObjectOffset(I));

  // Conservatively assume all callee-saved registers will be saved.
  for (const uint16_t *R = TRI.getCalleeSavedRegs(&MF); *R; ++R) {
    unsigned Size = TRI.getMinimalPhysRegClass(*R)->getSize();
    Offset = RoundUpToAlignment(Offset + Size, Size);
  }

  unsigned MaxAlign = MFI->getMaxAlignment();

  // Check that MaxAlign is not zero if there is a stack object that is not a
  // callee-saved spill.
  assert(!MFI->getObjectIndexEnd() || MaxAlign);

  // Iterate over other objects.
  for (unsigned I = 0, E = MFI->getObjectIndexEnd(); I != E; ++I)
    Offset = RoundUpToAlignment(Offset + MFI->getObjectSize(I), MaxAlign);

  // Call frame.
  if (MFI->adjustsStack() && hasReservedCallFrame(MF))
    Offset = RoundUpToAlignment(Offset + MFI->getMaxCallFrameSize(),
                                std::max(MaxAlign, getStackAlignment()));

  return RoundUpToAlignment(Offset, getStackAlignment());
}

void NyuziFrameLowering::processFunctionBeforeCalleeSavedScan(
    MachineFunction &MF, RegScavenger *RS) const {
  if (hasFP(MF))
    MF.getRegInfo().setPhysRegUsed(Nyuzi::FP_REG);

  // The register scavenger allows us to allocate virtual registers
  // during epilogue/prologue insertion, after register allocation has
  // run. We only need to do this if the frame is to large to be
  // addressed by immediate offsets. If it isn't, don't bother creating
  // a stack slot for it.  Note that we may in some cases create the scavenge
  // slot when it isn't needed.
  if (getWorstCaseStackSize(MF) < 0x2000)
    return;

  const TargetRegisterClass *RC = &Nyuzi::GPR32RegClass;
  int FI = MF.getFrameInfo()->CreateStackObject(RC->getSize(),
                                                RC->getAlignment(), false);
  RS->addScavengingFrameIndex(FI);
}
