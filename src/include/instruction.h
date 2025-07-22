#ifndef ISM_INSTRUCTION_H
#define ISM_INSTRUCTION_H

#include <variant>
#include <cstdint>

namespace insomnia {

/********************** instruction codes ***********************/

// instr[6:0]: opcode
enum class opcode : uint8_t {
  lui     = 0b0110111,
  auipc   = 0b0010111,
  jal     = 0b1101111,
  jalr    = 0b1100111,
  b_instr = 0b1100011,
  l_instr = 0b0000011, // type I: load
  s_instr = 0b0100011,
  i_instr = 0b0010011, // type I: immediate
  r_instr = 0b0110011,
};

// instr[14:12]: type B identifier code
enum class b_instr_code : uint8_t {
  beq  = 0b000,
  bne  = 0b001,
  blt  = 0b100,
  bge  = 0b101,
  bltu = 0b110,
  bgeu = 0b111,
};

// instr[14:12]: type I-load identifier code
enum class l_instr_code : uint8_t {
  lb  = 0b000,
  lh  = 0b001,
  lw  = 0b010,
  lbu = 0b100,
  lhu = 0b101,
};

// instr[14:12]: type S identifier code
enum class s_instr_code : uint8_t {
  sb = 0b000,
  sh = 0b001,
  sw = 0b010,
};

// instr[14:12]: type I-immediate identifier code
enum class i_instr_code : uint8_t {
  addi       = 0b000,
  slti       = 0b010,
  sltiu      = 0b011,
  xori       = 0b100,
  ori        = 0b110,
  andi       = 0b111,
  slli       = 0b001,
  srli_srai  = 0b101,
};

// instr[31:25]: srli/srai identifier code
enum class srli_srai_code : uint8_t {
  srli = 0b0000000,
  srai = 0b0100000,
};

// instr[14:12]: type R identifier code
enum class r_instr_code : uint8_t {
  add_sub = 0b000,
  sll     = 0b001,
  slt     = 0b010,
  sltu    = 0b011,
  xor_    = 0b100,
  srl_sra = 0b101,
  or_     = 0b110,
  and_    = 0b111,
};

// instr[31:25]: add_sub identifier code
enum class add_sub_code : uint8_t {
  add = 0b0000000,
  sub = 0b0100000,
};

// instr[31:25]: srl/sra identifier code
enum class srl_sra_code : uint8_t {
  srl = 0b0000000,
  sra = 0b0100000,
};

class Instruction {

};

}

#endif // ISM_INSTRUCTION_H