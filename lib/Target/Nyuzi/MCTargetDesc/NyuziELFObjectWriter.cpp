//===-- NyuziELFObjectWriter.cpp - Nyuzi ELF Writer
//-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/NyuziFixupKinds.h"
#include "MCTargetDesc/NyuziMCTargetDesc.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
class NyuziELFObjectWriter : public MCELFObjectTargetWriter {
public:
  NyuziELFObjectWriter(uint8_t OSABI);

  virtual ~NyuziELFObjectWriter();

protected:
  virtual unsigned GetRelocType(const MCValue &Target, const MCFixup &Fixup,
                                bool IsPCRel) const override;
};
}

NyuziELFObjectWriter::NyuziELFObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(/*Is64Bit*/ false, OSABI, ELF::EM_NYUZI,
                              /*HasRelocationAddend*/ true) {}

NyuziELFObjectWriter::~NyuziELFObjectWriter() {}

unsigned NyuziELFObjectWriter::GetRelocType(const MCValue &Target,
                                                 const MCFixup &Fixup,
                                                 bool IsPCRel) const {
  unsigned Type;
  unsigned Kind = (unsigned)Fixup.getKind();
  switch (Kind) {
  default:
    llvm_unreachable("Invalid fixup kind!");
  case FK_Data_4:
  case Nyuzi::fixup_Nyuzi_Abs32:
    Type = ELF::R_NYUZI_ABS32;
    break;

  case Nyuzi::fixup_Nyuzi_PCRel_Branch:
    Type = ELF::R_NYUZI_BRANCH;
    break;

  // In normal cases, these types should not be emitted because they can be fixed up immediately.
  // This generally happens if there is an undefined symbol.  This will cause an error later
  // during linking.
  case Nyuzi::fixup_Nyuzi_PCRel_MemAccExt:
    Type = ELF::R_NYUZI_PCREL_MEM_EXT;
    break;
    
  case Nyuzi::fixup_Nyuzi_PCRel_MemAcc:
    Type = ELF::R_NYUZI_PCREL_MEM;
    break;
  }
  return Type;
}

MCObjectWriter *llvm::createNyuziELFObjectWriter(raw_ostream &OS,
                                                      uint8_t OSABI) {
  MCELFObjectTargetWriter *MOTW = new NyuziELFObjectWriter(OSABI);
  return createELFObjectWriter(MOTW, OS, true);
}
