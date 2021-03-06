//===- PDBSymbolTypeEnum.h - enum type info ---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_PDB_PDBSYMBOLTYPEENUM_H
#define LLVM_DEBUGINFO_PDB_PDBSYMBOLTYPEENUM_H

#include "PDBSymbol.h"
#include "PDBTypes.h"

namespace llvm {

class PDBSymbolTypeEnum : public PDBSymbol {
public:
  PDBSymbolTypeEnum(std::unique_ptr<IPDBRawSymbol> EnumTypeSymbol);

  FORWARD_SYMBOL_METHOD(getBuiltinType)
  FORWARD_SYMBOL_METHOD(getClassParentId)
  FORWARD_SYMBOL_METHOD(hasConstructor)
  FORWARD_SYMBOL_METHOD(isConstType)
  FORWARD_SYMBOL_METHOD(hasAssignmentOperator)
  FORWARD_SYMBOL_METHOD(hasCastOperator)
  FORWARD_SYMBOL_METHOD(hasNestedTypes)
  FORWARD_SYMBOL_METHOD(getLength)
  FORWARD_SYMBOL_METHOD(getLexicalParentId)
  FORWARD_SYMBOL_METHOD(getName)
  FORWARD_SYMBOL_METHOD(isNested)
  FORWARD_SYMBOL_METHOD(hasOverloadedOperator)
  FORWARD_SYMBOL_METHOD(isPacked)
  FORWARD_SYMBOL_METHOD(isScoped)
  FORWARD_SYMBOL_METHOD(getSymIndexId)
  FORWARD_SYMBOL_METHOD(getTypeId)
  FORWARD_SYMBOL_METHOD(isUnalignedType)
  FORWARD_SYMBOL_METHOD(isVolatileType)

  static bool classof(const PDBSymbol *S) {
    return S->getSymTag() == PDB_SymType::Enum;
  }
};

} // namespace llvm

#endif // LLVM_DEBUGINFO_PDB_PDBSYMBOLTYPEENUM_H
