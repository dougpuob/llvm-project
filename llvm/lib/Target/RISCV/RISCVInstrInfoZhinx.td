//===- RISCVInstrInfoZhinx.td - RISC-V 'Zhinx' instructions -*- tblgen -*-====//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the RISC-V instructions from the standard 'Zhinx'
// half float in integer.
// This version is still experimental as the 'Zhinx' extension hasn't been
// ratified yet.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Operand and SDNode transformation definitions.
//===----------------------------------------------------------------------===//

def GPRF16Op : RegisterOperand<GPRF16> {
  let ParserMatchClass = GPRAsFPR;
}

//===----------------------------------------------------------------------===//
// Instruction class templates
//===----------------------------------------------------------------------===//

let DecoderNamespace = "RVZfinx" in {
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class FPFMAHINX_rrr_frm<RISCVOpcode opcode, string opcodestr>
    : RVInstR4Frm<0b10, opcode, (outs GPRF16Op:$rd),
                  (ins GPRF16Op:$rs1, GPRF16Op:$rs2, GPRF16Op:$rs3, frmarg:$funct3),
                  opcodestr, "$rd, $rs1, $rs2, $rs3, $funct3">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class FPALUHINX_rr<bits<7> funct7, bits<3> funct3, string opcodestr>
    : RVInstR<funct7, funct3, OPC_OP_FP, (outs GPRF16Op:$rd),
              (ins GPRF16Op:$rs1, GPRF16Op:$rs2), opcodestr, "$rd, $rs1, $rs2">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class FPALUHINX_rr_frm<bits<7> funct7, string opcodestr>
    : RVInstRFrm<funct7, OPC_OP_FP, (outs GPRF16Op:$rd),
                 (ins GPRF16Op:$rs1, GPRF16Op:$rs2, frmarg:$funct3), opcodestr,
                  "$rd, $rs1, $rs2, $funct3">;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class FPCmpHINX_rr<bits<3> funct3, string opcodestr>
    : RVInstR<0b1010010, funct3, OPC_OP_FP, (outs GPROp:$rd),
              (ins GPRF16Op:$rs1, GPRF16Op:$rs2), opcodestr, "$rd, $rs1, $rs2">,
      Sched<[WriteFCmp16, ReadFCmp16, ReadFCmp16]>;
} // DecoderNamespace = "RVZfinx"

class FPFMAHINXDynFrmAlias<FPFMAHINX_rrr_frm Inst, string OpcodeStr>
    : InstAlias<OpcodeStr#" $rd, $rs1, $rs2, $rs3",
                (Inst GPRF16Op:$rd, GPRF16Op:$rs1, GPRF16Op:$rs2, GPRF16Op:$rs3, 0b111)>;

class FPALUHINXDynFrmAlias<FPALUHINX_rr_frm Inst, string OpcodeStr>
    : InstAlias<OpcodeStr#" $rd, $rs1, $rs2",
                (Inst GPRF16Op:$rd, GPRF16Op:$rs1, GPRF16Op:$rs2, 0b111)>;

//===----------------------------------------------------------------------===//
// Instructions
//===----------------------------------------------------------------------===//

// instructions in Zhinx extension on RV32/64
let Predicates = [HasStdExtZhinx] in {
def FMADD_H_INX  : FPFMAHINX_rrr_frm<OPC_MADD, "fmadd.h">,
                   Sched<[WriteFMA16, ReadFMA16, ReadFMA16, ReadFMA16]>;
def              : FPFMAHINXDynFrmAlias<FMADD_H_INX, "fmadd.h">;
def FMSUB_H_INX  : FPFMAHINX_rrr_frm<OPC_MSUB, "fmsub.h">,
                   Sched<[WriteFMA16, ReadFMA16, ReadFMA16, ReadFMA16]>;
def              : FPFMAHINXDynFrmAlias<FMSUB_H_INX, "fmsub.h">;
def FNMSUB_H_INX : FPFMAHINX_rrr_frm<OPC_NMSUB, "fnmsub.h">,
                   Sched<[WriteFMA16, ReadFMA16, ReadFMA16, ReadFMA16]>;
def              : FPFMAHINXDynFrmAlias<FNMSUB_H_INX, "fnmsub.h">;
def FNMADD_H_INX : FPFMAHINX_rrr_frm<OPC_NMADD, "fnmadd.h">,
                   Sched<[WriteFMA16, ReadFMA16, ReadFMA16, ReadFMA16]>;
def              : FPFMAHINXDynFrmAlias<FNMADD_H_INX, "fnmadd.h">;

def FADD_H_INX : FPALUHINX_rr_frm<0b0000010, "fadd.h">,
                 Sched<[WriteFALU16, ReadFALU16, ReadFALU16]>;
def            : FPALUHINXDynFrmAlias<FADD_H_INX, "fadd.h">;
def FSUB_H_INX : FPALUHINX_rr_frm<0b0000110, "fsub.h">,
                 Sched<[WriteFALU16, ReadFALU16, ReadFALU16]>;
def            : FPALUHINXDynFrmAlias<FSUB_H_INX, "fsub.h">;
def FMUL_H_INX : FPALUHINX_rr_frm<0b0001010, "fmul.h">,
                 Sched<[WriteFMul16, ReadFMul16, ReadFMul16]>;
def            : FPALUHINXDynFrmAlias<FMUL_H_INX, "fmul.h">;
def FDIV_H_INX : FPALUHINX_rr_frm<0b0001110, "fdiv.h">,
                 Sched<[WriteFDiv16, ReadFDiv16, ReadFDiv16]>;
def            : FPALUHINXDynFrmAlias<FDIV_H_INX, "fdiv.h">;

def FSQRT_H_INX : FPUnaryOpINX_r_frm<0b0101110, GPRF16Op, GPRF16Op, "fsqrt.h">,
                  Sched<[WriteFSqrt16, ReadFSqrt16]> {
  let rs2 = 0b00000;
}
def             : FPUnaryOpINXDynFrmAlias<FSQRT_H_INX, "fsqrt.h", GPRF16Op, GPRF16Op>;

def FSGNJ_H_INX  : FPALUHINX_rr<0b0010010, 0b000, "fsgnj.h">,
                   Sched<[WriteFSGNJ16, ReadFSGNJ16, ReadFSGNJ16]>;
def FSGNJN_H_INX : FPALUHINX_rr<0b0010010, 0b001, "fsgnjn.h">,
                   Sched<[WriteFSGNJ16, ReadFSGNJ16, ReadFSGNJ16]>;
def FSGNJX_H_INX : FPALUHINX_rr<0b0010010, 0b010, "fsgnjx.h">,
                   Sched<[WriteFSGNJ16, ReadFSGNJ16, ReadFSGNJ16]>;
def FMIN_H_INX   : FPALUHINX_rr<0b0010110, 0b000, "fmin.h">,
                   Sched<[WriteFMinMax16, ReadFMinMax16, ReadFMinMax16]>;
def FMAX_H_INX   : FPALUHINX_rr<0b0010110, 0b001, "fmax.h">,
                   Sched<[WriteFMinMax16, ReadFMinMax16, ReadFMinMax16]>;

def FCVT_W_H_INX : FPUnaryOpINX_r_frm<0b1100010, GPROp, GPRF16Op, "fcvt.w.h">,
                   Sched<[WriteFCvtF16ToI32, ReadFCvtF16ToI32]> {
  let rs2 = 0b00000;
}
def              : FPUnaryOpINXDynFrmAlias<FCVT_W_H_INX, "fcvt.w.h", GPROp, GPRF16Op>;

def FCVT_WU_H_INX : FPUnaryOpINX_r_frm<0b1100010, GPROp, GPRF16Op, "fcvt.wu.h">,
                    Sched<[WriteFCvtF16ToI32, ReadFCvtF16ToI32]> {
  let rs2 = 0b00001;
}
def               : FPUnaryOpINXDynFrmAlias<FCVT_WU_H_INX, "fcvt.wu.h", GPROp, GPRF16Op>;

def FCVT_H_W_INX : FPUnaryOpINX_r_frm<0b1101010, GPRF16Op, GPROp, "fcvt.h.w">,
                   Sched<[WriteFCvtI32ToF16, ReadFCvtI32ToF16]> {
  let rs2 = 0b00000;
}
def              : FPUnaryOpINXDynFrmAlias<FCVT_H_W_INX, "fcvt.h.w", GPRF16Op, GPROp>;

def FCVT_H_WU_INX : FPUnaryOpINX_r_frm<0b1101010, GPRF16Op, GPROp, "fcvt.h.wu">,
                    Sched<[WriteFCvtI32ToF16, ReadFCvtI32ToF16]> {
  let rs2 = 0b00001;
}
def               : FPUnaryOpINXDynFrmAlias<FCVT_H_WU_INX, "fcvt.h.wu", GPRF16Op, GPROp>;

def FEQ_H_INX : FPCmpHINX_rr<0b010, "feq.h">;
def FLT_H_INX : FPCmpHINX_rr<0b001, "flt.h">;
def FLE_H_INX : FPCmpHINX_rr<0b000, "fle.h">;

def FCLASS_H_INX : FPUnaryOpINX_r<0b1110010, 0b001, GPROp, GPRF16Op, "fclass.h">,
                   Sched<[WriteFClass16, ReadFClass16]> {
  let rs2 = 0b00000;
}
} // Predicates = [HasStdExtZhinx]

let Predicates = [HasStdExtZhinxOrZhinxmin] in {
def FCVT_H_S_INX : FPUnaryOpINX_r_frm<0b0100010, GPRF16Op, GPRF32Op, "fcvt.h.s">,
                   Sched<[WriteFCvtF32ToF16, ReadFCvtF32ToF16]> {
  let rs2 = 0b00000;
}
def              : FPUnaryOpINXDynFrmAlias<FCVT_H_S_INX, "fcvt.h.s", GPRF16Op, GPRF32Op>;

def FCVT_S_H_INX : FPUnaryOpINX_r<0b0100000, 0b000, GPRF32Op, GPRF16Op, "fcvt.s.h">,
                   Sched<[WriteFCvtF16ToF32, ReadFCvtF16ToF32]> {
  let rs2 = 0b00010;
}
} // Predicates = [HasStdExtZhinx]


// Instructions in Zhinx extension on RV64
let Predicates = [HasStdExtZhinx, IsRV64] in {
def FCVT_L_H_INX  : FPUnaryOpINX_r_frm<0b1100010, GPROp, GPRF16Op, "fcvt.l.h">,
                    Sched<[WriteFCvtF16ToI64, ReadFCvtF16ToI64]> {
  let rs2 = 0b00010;
}
def               : FPUnaryOpINXDynFrmAlias<FCVT_L_H_INX, "fcvt.l.h", GPROp, GPRF16Op>;

def FCVT_LU_H_INX  : FPUnaryOpINX_r_frm<0b1100010, GPROp, GPRF16Op, "fcvt.lu.h">,
                     Sched<[WriteFCvtF16ToI64, ReadFCvtF16ToI64]> {
  let rs2 = 0b00011;
}
def                : FPUnaryOpINXDynFrmAlias<FCVT_LU_H_INX, "fcvt.lu.h", GPROp, GPRF16Op>;

def FCVT_H_L_INX : FPUnaryOpINX_r_frm<0b1101010, GPRF16Op, GPROp, "fcvt.h.l">,
                   Sched<[WriteFCvtI64ToF16, ReadFCvtI64ToF16]> {
  let rs2 = 0b00010;
}
def              : FPUnaryOpINXDynFrmAlias<FCVT_H_L_INX, "fcvt.h.l", GPRF16Op, GPROp>;

def FCVT_H_LU_INX : FPUnaryOpINX_r_frm<0b1101010, GPRF16Op, GPROp, "fcvt.h.lu">,
                    Sched<[WriteFCvtI64ToF16, ReadFCvtI64ToF16]> {
  let rs2 = 0b00011;
}
def               : FPUnaryOpINXDynFrmAlias<FCVT_H_LU_INX, "fcvt.h.lu", GPRF16Op, GPROp>;
} // Predicates = [HasStdExtZhinx, IsRV64]

// instructions in Zhinx or zhinxmin, and Zdinx extensions
let Predicates = [HasStdExtZhinxOrZhinxmin, HasStdExtZdinx] in {
def FCVT_H_D_INX : FPUnaryOpINX_r_frm<0b0100010, GPRF16Op, GPRF64Op, "fcvt.h.d">,
                   Sched<[WriteFCvtF64ToF16, ReadFCvtF64ToF16]> {
  let rs2 = 0b00001;
}
def              : FPUnaryOpINXDynFrmAlias<FCVT_H_D_INX, "fcvt.h.d", GPRF16Op, GPRF64Op>;

def FCVT_D_H_INX : FPUnaryOpINX_r<0b0100001, 0b000, GPRF64Op, GPRF16Op, "fcvt.d.h">,
                   Sched<[WriteFCvtF16ToF64, ReadFCvtF16ToF64]> {
  let rs2 = 0b00010;
}
} // Predicates = [HasStdExtZhinxOrZhinxmin, HasStdExtZdinx]

//===----------------------------------------------------------------------===//
// Assembler Pseudo Instructions (User-Level ISA, Version 2.2, Chapter 20)
//===----------------------------------------------------------------------===//

let Predicates = [HasStdExtZhinx] in {
def : InstAlias<"fabs.h $rd, $rs", (FSGNJX_H_INX GPRF16Op:$rd, GPRF16Op:$rs, GPRF16Op:$rs)>;
def : InstAlias<"fneg.h $rd, $rs", (FSGNJN_H_INX GPRF16Op:$rd, GPRF16Op:$rs, GPRF16Op:$rs)>;

// fgt.h/fge.h are recognised by the GNU assembler but the canonical
// flt.h/fle.h forms will always be printed. Therefore, set a zero weight.
def : InstAlias<"fgt.h $rd, $rs, $rt",
                (FLT_H_INX GPROp:$rd, GPRF16Op:$rt, GPRF16Op:$rs), 0>;
def : InstAlias<"fge.h $rd, $rs, $rt",
                (FLE_H_INX GPROp:$rd, GPRF16Op:$rt, GPRF16Op:$rs), 0>;
} // Predicates = [HasStdExtZhinx]
