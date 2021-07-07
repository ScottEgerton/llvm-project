//===-- RISCVBaseInfo.cpp - Top level definitions for RISCV MC ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone enum definitions for the RISCV target
// useful for the compiler back-end and the MC libraries.
//
//===----------------------------------------------------------------------===//

#include "RISCVBaseInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
namespace RISCVSysReg {
#define GET_SysRegsList_IMPL
#include "RISCVGenSearchableTables.inc"
} // namespace RISCVSysReg

namespace RISCVABI {
ABI computeTargetABI(const Triple &TT, FeatureBitset FeatureBits,
                     StringRef ABIName) {
  auto TargetABI = getTargetABI(ABIName);
  bool IsRV64 = TT.isArch64Bit();
  bool IsRV32E = FeatureBits[RISCV::FeatureRV32E];

  if (!ABIName.empty() && TargetABI == ABI_Unknown) {
    errs()
        << "'" << ABIName
        << "' is not a recognized ABI for this target (ignoring target-abi)\n";
  } else if (ABIName.startswith("ilp32") && IsRV64) {
    errs() << "32-bit ABIs are not supported for 64-bit targets (ignoring "
              "target-abi)\n";
    TargetABI = ABI_Unknown;
  } else if (ABIName.startswith("lp64") && !IsRV64) {
    errs() << "64-bit ABIs are not supported for 32-bit targets (ignoring "
              "target-abi)\n";
    TargetABI = ABI_Unknown;
  } else if (IsRV32E && TargetABI != ABI_ILP32E && TargetABI != ABI_Unknown) {
    // TODO: move this checking to RISCVTargetLowering and RISCVAsmParser
    errs()
        << "Only the ilp32e ABI is supported for RV32E (ignoring target-abi)\n";
    TargetABI = ABI_Unknown;
  }

  if (TargetABI != ABI_Unknown)
    return TargetABI;

  // For now, default to the ilp32/ilp32e/lp64 ABI if no explicit ABI is given
  // or an invalid/unrecognised string is given. In the future, it might be
  // worth changing this to default to ilp32f/lp64f and ilp32d/lp64d when
  // hardware support for floating point is present.
  if (IsRV32E)
    return ABI_ILP32E;
  if (IsRV64)
    return ABI_LP64;
  return ABI_ILP32;
}

ABI getTargetABI(StringRef ABIName) {
  auto TargetABI = StringSwitch<ABI>(ABIName)
                       .Case("ilp32", ABI_ILP32)
                       .Case("ilp32f", ABI_ILP32F)
                       .Case("ilp32d", ABI_ILP32D)
                       .Case("ilp32e", ABI_ILP32E)
                       .Case("lp64", ABI_LP64)
                       .Case("lp64f", ABI_LP64F)
                       .Case("lp64d", ABI_LP64D)
                       .Default(ABI_Unknown);
  return TargetABI;
}

// To avoid the BP value clobbered by a function call, we need to choose a
// callee saved register to save the value. RV32E only has X8 and X9 as callee
// saved registers and X8 will be used as fp. So we choose X9 as bp.
MCRegister getBPReg() { return RISCV::X9; }

// Returns the register holding shadow call stack pointer.
MCRegister getSCSPReg() { return RISCV::X18; }

} // namespace RISCVABI

namespace RISCVFeatures {

void validate(const Triple &TT, const FeatureBitset &FeatureBits) {
  if (TT.isArch64Bit() && !FeatureBits[RISCV::Feature64Bit])
    report_fatal_error("RV64 target requires an RV64 CPU");
  if (!TT.isArch64Bit() && FeatureBits[RISCV::Feature64Bit])
    report_fatal_error("RV32 target requires an RV32 CPU");
  if (TT.isArch64Bit() && FeatureBits[RISCV::FeatureRV32E])
    report_fatal_error("RV32E can't be enabled for an RV64 target");
}

} // namespace RISCVFeatures

void RISCVVType::printVType(unsigned VType, raw_ostream &OS) {
  RISCVVSEW VSEW = getVSEW(VType);
  RISCVVLMUL VLMUL = getVLMUL(VType);

  unsigned Sew = 1 << (static_cast<unsigned>(VSEW) + 3);
  OS << "e" << Sew;

  switch (VLMUL) {
  case RISCVVLMUL::LMUL_RESERVED:
    llvm_unreachable("Unexpected LMUL value!");
  case RISCVVLMUL::LMUL_1:
  case RISCVVLMUL::LMUL_2:
  case RISCVVLMUL::LMUL_4:
  case RISCVVLMUL::LMUL_8: {
    unsigned LMul = 1 << static_cast<unsigned>(VLMUL);
    OS << ",m" << LMul;
    break;
  }
  case RISCVVLMUL::LMUL_F2:
  case RISCVVLMUL::LMUL_F4:
  case RISCVVLMUL::LMUL_F8: {
    unsigned LMul = 1 << (8 - static_cast<unsigned>(VLMUL));
    OS << ",mf" << LMul;
    break;
  }
  }

  if (isTailAgnostic(VType))
    OS << ",ta";
  else
    OS << ",tu";

  if (isMaskAgnostic(VType))
    OS << ",ma";
  else
    OS << ",mu";
}
// namespace RISCVZCE {
// #define ALIST_PRINT(ENCODE, STR) \
//   case (ENCODE): \
//     OS << STR;
void RISCVZCE::printAlist(unsigned opcode, unsigned SlistImm, raw_ostream &OS) {
  OS << "{";
  if(opcode == RISCV::C_PUSH || opcode == RISCV::PUSH){
    switch ((SLISTENCODE)SlistImm)
    {
    default:
      break;
    case SLISTENCODE::RA:
      break;
    case SLISTENCODE::RA_S0:
      OS << "a0";
      break;
    case SLISTENCODE::RA_S0_S1:
      OS << "a0-a1";
      break;
    case SLISTENCODE::RA_S0_S2:
    case SLISTENCODE::RA_S0_S2_E:
      OS << "a0-a2";
      break;
    case SLISTENCODE::RA_S0_S3:
    case SLISTENCODE::RA_S0_S4:
    case SLISTENCODE::RA_S0_S5:
    case SLISTENCODE::RA_S0_S6:
    case SLISTENCODE::RA_S0_S7:
    case SLISTENCODE::RA_S0_S8:
    case SLISTENCODE::RA_S0_S9:
    case SLISTENCODE::RA_S0_S10:
    case SLISTENCODE::RA_S0_S11:
    case SLISTENCODE::RA_S0_S3_E:
    case SLISTENCODE::RA_S0_S4_E:
      OS << "a0-a3";
      break;
    }
  }
  else {
    switch (SlistImm) {
      case 0:
        OS << "a0-a2";
        break;
      case 1:
      case 2:
        OS << "a0-a3";
        break;
    }
  }
  OS << "}";
}

void RISCVZCE::printSlist(unsigned SlistEncode, raw_ostream &OS) {
  OS << "{";
  switch (SlistEncode) {
    case 0:
      OS << "ra";
      break;
    case 1:
      OS << "ra, s0";
      break;
    case 2:
      OS << "ra, s0-s1";
      break;
    case 3:
    case 13:
      OS << "ra, s0-s2";
      break;
    case 4:
    case 14:
      OS << "ra, s0-s3";
      break;
    case 5:
    case 15:
      OS << "ra, s0-s4";
      break;
    case 6:
      OS << "ra, s0-s5";
      break;
    case 7:
      OS << "ra, s0-s6";
      break;
    case 8:
      OS << "ra, s0-s7";
      break;
    case 9:
      OS << "ra, s0-s8";
      break;
    case 10:
      OS << "ra, s0-s9";
      break;
    case 11:
      OS << "ra, s0-s10";
      break;
    case 12:
      OS << "ra, s0-s11";
      break;
  }
  OS << "}";
}

void RISCVZCE::printSlist16(unsigned RlistEncode, raw_ostream &OS) {
  switch ((SLIST16ENCODE)RlistEncode) {
    case SLIST16ENCODE::RA:
      OS << "{ra}";
      return;
    case SLIST16ENCODE::RA_S0:
      OS << "{ra, s0}";
      return;
    case SLIST16ENCODE::RA_S0_S1:
      OS << "{ra, s0-s1}";
      return;
    case SLIST16ENCODE::RA_S0_S2:
      OS << "{ra, s0-s2}";
      return;
    case SLIST16ENCODE::RA_S0_S3:
      OS << "{ra, s0-s3}";
      return;
    case SLIST16ENCODE::RA_S0_S5:
      OS << "{ra, s0-s5}";
      return;
    case SLIST16ENCODE::RA_S0_S7:
      OS << "{ra, s0-s7}";
      return;
    case SLIST16ENCODE::RA_S0_S11:
      OS << "{ra, s0-s11}";
      return;
  }
}

void RISCVZCE::printRlist3(unsigned RlistEncode, raw_ostream &OS) {
  switch ((RLIST3ENCODE)RlistEncode) {
    case RLIST3ENCODE::NO_MATCH:
      return;
    case RLIST3ENCODE::RA:
      OS << "{ra}";
      return;
    case RLIST3ENCODE::RA_S0:
      OS << "{ra, s0}";
      return;
    case RLIST3ENCODE::RA_S0_S1:
      OS << "{ra, s0-s1}";
      return;
    case RLIST3ENCODE::RA_S0_S2:
      OS << "{ra, s0-s2}";
      return;
    case RLIST3ENCODE::RA_S0_S3:
      OS << "{ra, s0-s3}";
      return;
    case RLIST3ENCODE::RA_S0_S5:
      OS << "{ra, s0-s5}";
      return;
    case RLIST3ENCODE::RA_S0_S7:
      OS << "{ra, s0-s7}";
      return;
    case RLIST3ENCODE::RA_S0_S11:
      OS << "{ra, s0-s11}";
      return;
  }
}

void RISCVZCE::printRlist2(unsigned RlistEncode, raw_ostream &OS) {
  switch ((RLIST2ENCODE)RlistEncode) {
    case RLIST2ENCODE::NO_MATCH:
      return;
    case RLIST2ENCODE::RA_S0_S2:
      OS << "{ra, s0-s2}";
      return;
    case RLIST2ENCODE::RA_S0_S3:
      OS << "{ra, s0-s3}";
      return;
    case RLIST2ENCODE::RA_S0_S4:
      OS << "{ra, s0-s4}";
      return;
    case RLIST2ENCODE::RA:
      OS << "{ra}";
      return;
    case RLIST2ENCODE::RA_S0:
      OS << "{ra, s0}";
      return;
    case RLIST2ENCODE::RA_S0_S1:
      OS << "{ra, s0-s1}";
      return;
  }
}
// #undef ALIST_PRINT
// }

void RISCVZCE::printRetval(unsigned RetvalEncode, raw_ostream &OS) {
  OS << '{';
  switch (RetvalEncode) {
    default:
      break;
    case 1:
      OS << '0';
      break;
    case 2:
      OS << '1';
      break;
    case 3:
      OS << "-1";
      break;
  }
  OS << '}';
}

void RISCVZCE::printSpimm(int64_t Spimm, raw_ostream &OS){
  OS << Spimm;
}

} // namespace llvm
