/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <macro.h>
#include "../../monitor/ftrace/ftrace.h"

static void Branch_Cond(Decode *s, word_t src1, word_t src2, word_t imm, uint32_t Cond);
static void ftrace(Decode *s, word_t snpc, word_t dnpc);
static uint64_t _unsigned_multiply_high(uint64_t a, uint64_t b);
static int64_t _multiply_high(int64_t a, int64_t b);
static void ecall_ctrl(Decode *s);
//static word_t csr_ctrl(Decode *s, word_t src1, word_t imm);
static void mret_ctrl(Decode *s);

#define R(i) gpr(i)
#define CR(i) csr(i)

#define Mr vaddr_read
#define Mw vaddr_write
#define Bc Branch_Cond
#define XLEN 64 

/* NEMU does not support user mode & supervisor mode. */
enum {
  MODE_M = 11, MODE_S, MODE_U
};

enum {
  COND_BEQ, COND_BNE, COND_BLT, COND_BGE, COND_BLTU, COND_BGEU,
};

enum {
  TYPE_I, TYPE_U, TYPE_S, TYPE_J, TYPE_R, TYPE_B,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immJ() do { *imm = SEXT(BITS(i, 31, 31), 1) << 20 | BITS(i, 30, 21) << 1 \
                    | BITS(i, 20, 20) << 11 | BITS(i, 19, 12) << 12; } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immR() do { *imm = (SEXT(BITS(i, 31, 25), 7)); } while(0)
#define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | BITS(i, 11, 8) << 1 \
                    | BITS(i, 30, 25) << 5 | BITS(i, 7, 7) << 11; } while(0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_J:                   immJ(); break;
    case TYPE_R: src1R(); src2R(); immR(); break;
    case TYPE_B: src1R(); src2R(); immB(); break;
  }
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  /* Some commands set for RISC-V32*/
  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s->snpc , s->dnpc = SEXT(BITS(imm , 19, 0), 20) + s->pc, ftrace(s, s->pc, s->dnpc));

  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, R(rd) = s->snpc , s->dnpc = (src1 + SEXT(BITS(imm, 11, 0), 12))& ~(word_t)1, ftrace(s, s->pc, s->dnpc));
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? 000 ????? 00110 11", addiw  , I, R(rd) = SEXT(BITS(src1 + imm, 31, 0), 32));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = SEXT(BITS(Mr(src1 + imm, 4), 31, 0), 32));
  INSTPAT("??????? ????? ????? 110 ????? 00000 11", lwu    , I, R(rd) = Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT(BITS(Mr(src1 + imm, 1), 7, 0), 8));
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(BITS(Mr(src1 + imm, 2), 15, 0) , 16));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2));
  INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld     , I, R(rd) = SEXT(Mr(src1 + imm, 8), 64));
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = (int64_t)src1 < (int64_t)imm ? 1 : 0);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = src1 < imm ? 1 : 0);
  INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << BITS(imm, 5, 0));
  INSTPAT("0000000 ????? ????? 001 ????? 00110 11", slliw  , I, R(rd) = SEXT(BITS(src1, 31, 0) << BITS(imm, 4, 0), 32));
  INSTPAT("010000? ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = (int64_t)src1 >> BITS(imm, 5, 0));
  INSTPAT("0100000 ????? ????? 101 ????? 00110 11", sraiw  , I, R(rd) = SEXT((int32_t)BITS(src1, 31, 0) >> BITS(imm, 4, 0), 32));
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & SEXT(BITS(imm, 11, 0), 12));
  INSTPAT("000000? ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = src1 >> BITS(imm, 5, 0));
  INSTPAT("0000000 ????? ????? 101 ????? 00110 11", srliw  , I, R(rd) = SEXT(BITS(src1, 31, 0) >> BITS(imm, 4, 0), 32));
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw  , I, word_t t = (CR(imm).val); (CR(imm).val) = src1, R(rd) = t);
  INSTPAT("??????? ????? ????? 010 ????? 11100 11", csrrs  , I, word_t t = (CR(imm).val); (CR(imm).val) = t | src1, R(rd) = t);
  INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall  , I, ecall_ctrl(s));

  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
  INSTPAT("0000000 ????? ????? 000 ????? 01110 11", addw   , R, R(rd) = SEXT(BITS(src1 + src2, 31, 0), 32));
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01110 11", subw   , R, R(rd) = SEXT(BITS(src1 - src2, 31, 0), 32));
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = (int64_t)src1 % (int64_t)src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = src1 % src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01110 11", remw   , R, R(rd) = SEXT(BITS(src1, 31, 0) % BITS(src2, 31, 0), 32));
  INSTPAT("0000001 ????? ????? 111 ????? 01110 11", remuw  , R, R(rd) = SEXT(BITS(src1, 31, 0) % BITS(src2, 31, 0), 32));
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = src1 < src2 ? 1 : 0);
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = (int64_t)src1 < (int64_t)src2 ? 1 : 0); 
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << BITS(src2, 5, 0));
  INSTPAT("0000000 ????? ????? 001 ????? 01110 11", sllw   , R, R(rd) = SEXT(BITS(src1, 31, 0) << BITS(src2, 5, 0), 32));
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = src1 >> BITS(src2, 5, 0));
  INSTPAT("0000000 ????? ????? 101 ????? 01110 11", srlw   , R, R(rd) = SEXT(BITS(src1, 31, 0) >> BITS(src2, 4, 0), 32));
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = (int64_t)src1 >> BITS(src2, 5, 0));
  INSTPAT("0100000 ????? ????? 101 ????? 01110 11", sraw   , R, R(rd) = SEXT((int32_t)BITS(src1, 31, 0) >> BITS(src2, 4, 0), 32));
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2);
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = (int64_t)src1 / (int64_t)src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = src1 / src2);
  INSTPAT("0000001 ????? ????? 100 ????? 01110 11", divw   , R, R(rd) = SEXT((int32_t)BITS(src1, 31, 0) / (int32_t)BITS(src2, 31, 0), 32));
  INSTPAT("0000001 ????? ????? 101 ????? 01110 11", divuw  , R, R(rd) = SEXT(BITS(src1, 31, 0) / BITS(src2, 31, 0), 32));
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1 * src2);
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = _multiply_high(SEXT(src1, 64), SEXT(src2, 64)));
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , R, R(rd) = _unsigned_multiply_high(src1, src2));
  INSTPAT("0000001 ????? ????? 000 ????? 01110 11", mulw   , R, R(rd) = SEXT(BITS(src1 * src2, 31, 0), 32));
  INSTPAT("0011000 00010 00000 000 00000 11100 11", mret   , R, mret_ctrl(s));

  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = SEXT(BITS(imm, 31, 12) << 12, 32));

  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, BITS(src2, 7, 0)));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, BITS(src2, 31, 0)));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, BITS(src2, 15, 0)));
  INSTPAT("??????? ????? ????? 011 ????? 01000 11", sd     , S, Mw(src1 + imm, 8, src2));

  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, Bc(s, src1, src2, imm, COND_BEQ));
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, Bc(s, src1, src2, imm, COND_BNE));
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, Bc(s, src1, src2, imm, COND_BLT)); 
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, Bc(s, src1, src2, imm, COND_BGE));
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, Bc(s, src1, src2, imm, COND_BLTU));
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, Bc(s, src1, src2, imm, COND_BGEU));

  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

static void ftrace(Decode *s, word_t snpc, word_t dnpc) {
#ifdef CONFIG_FTRACE
  word_t i = s->isa.inst.val;
  word_t rs1 = BITS(i, 19, 15);
  word_t rd = BITS(i, 11, 7);
  if (rd == 1)  ftrace_call(snpc, dnpc);
  else if(rd == 0 && rs1 == 1)  ftrace_ret(snpc);
#endif
  return;
}

static void Branch_Cond(Decode *s, word_t src1, word_t src2, word_t imm, uint32_t Cond) {
  switch(Cond) {
    case COND_BEQ:  if (!(src1 == src2)) return;  break; 
    case COND_BNE:  if (!(src1 != src2)) return;  break;
    case COND_BLT:  if (!((int64_t)src1 < (int64_t)src2)) return; break;
    case COND_BGE:  if (!((int64_t)src1 >= (int64_t)src2)) return; break;
    case COND_BGEU: if (!(src1 >= src2)) return; break;
    case COND_BLTU: if (!(src1 < src2)) return; break;
    default:
      printf("Undefined COND_JMP.\n");
      assert(0);
  }
  /* All of valid COND_JMP will execute this statement. */
  s->dnpc = s->pc + imm;
}

/* Should be optimized. */
// TODO
static uint64_t _unsigned_multiply_high(uint64_t a, uint64_t b) {
  uint32_t a_low = (uint32_t)a;
  uint32_t a_high = (uint32_t)(a >> 32);
  uint32_t b_low = (uint32_t)b;
  uint32_t b_high = (uint32_t)(b >> 32);

  uint64_t low_product = (uint64_t)a_low * b_low;
  uint64_t high_product = (uint64_t)a_high * b_high;

  uint64_t low_result = low_product + (((uint64_t)(uint32_t)(low_product >> 32) + (uint64_t)(uint32_t)(a_low * b_high) + (uint64_t)(uint32_t)(a_high * b_low)) >> 32);
  uint64_t high_result = high_product + (uint64_t)(uint32_t)(low_result >> 32) + (uint64_t)(uint32_t)(a_high * b_high);

  return high_result;
}

static int64_t _multiply_high(int64_t a, int64_t b) {
  int32_t a_low = (int32_t)a;
  int32_t a_high = (int32_t)(a >> 32);
  int32_t b_low = (int32_t)b;
  int32_t b_high = (int32_t)(b >> 32);

  int64_t low_product = (int64_t)a_low * b_low;
  int64_t high_product = (int64_t)a_high * b_high;

  int64_t low_result = low_product + (((int64_t)(int32_t)(low_product >> 32) + (int64_t)(int32_t)(a_low * b_high) + (int64_t)(int32_t)(a_high * b_low)) >> 32);
  int64_t high_result = high_product + (int64_t)(int32_t)(low_result >> 32) + (int64_t)(int32_t)(a_high * b_high);

  return high_result;
}

/* What should be done in ecall ? 
* 1. Save the instruction address where the exception occurs to the mepc register
* 2. Save the interrupt type code to the mcause register (11 indicates an ecall from M mode)
* 3. If the interrupt has additional information, save it to the mtval register (omitted here)
* 4. If it is an externally triggered interrupt, set mstatus[MPIE] = mstatus[MIE] (save), and then set mstatus[MIE] = 0 (close the interrupt)
* 5. Save the current privilege mode to mstatus[MPP]
* 6. Set the current privilege mode to Machine mode
* 7. Jump to the corresponding interrupt response program according to the setting of the mtvec register 
*/
static void ecall_ctrl(Decode *s) {
  /* ics-project does not support multi-user mode, so we use MODE_M by default
  * The following code may change in the future. 
  */
#ifdef CONFIG_ETRACE
  Log("An exception occured at pc:" FMT_WORD " privilege mode: " FMT_WORD, s->pc, MODE_M);
#endif
  s->dnpc = isa_raise_intr(MODE_M, s->pc);
}

static void mret_ctrl(Decode *s) {
  s->dnpc = CR(0x341).val;
  CR(0x300).status.MIE = CR(0x300).status.MPIE;
  CR(0x300).status.MPIE = 1;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
