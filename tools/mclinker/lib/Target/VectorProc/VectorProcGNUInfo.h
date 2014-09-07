//===- VectorProcGNUInfo.h ---------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef TARGET_VECTORPROC_VECTORPROCGNUINFO_H
#define TARGET_VECTORPROC_VECTORPROCGNUINFO_H
#include <mcld/Target/GNUInfo.h>

#include <llvm/Support/ELF.h>

namespace mcld {

class VectorProcGNUInfo : public GNUInfo
{
public:
  VectorProcGNUInfo(const llvm::Triple& pTriple) : GNUInfo(pTriple) { }

  uint32_t machine() const { return llvm::ELF::EM_VECTORPROC; }

  uint64_t abiPageSize() const { return 32; }	// Cache line size

  uint64_t defaultTextSegmentAddr() const { return 0; }

  // There are no processor-specific flags so this field shall contain zero.
  uint64_t flags() const { return 0x0; }
};

} // namespace of mcld

#endif

