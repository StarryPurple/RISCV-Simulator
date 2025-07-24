#ifndef ISM_INSTRUCTION_H
#define ISM_INSTRUCTION_H

#include <cstdint>
#include <limits>

#include "common.h"
#include "utility.h"

namespace insomnia {

/********************** instruction codes ***********************/

enum class InstrType {
  LUI, AUIPC,
  JAL, JALR,
  BEQ, BNE, BLT, BGE, BLTU, BGEU,
  LB, LH, LW, LBU, LHU,
  SB, SH, SW,
  ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
  ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
};

// instr[6:0]: opcode
enum class opcode_t : uint8_t {
  LUI     = 0b0110111,
  AUIPC   = 0b0010111,
  JAL     = 0b1101111,
  JALR    = 0b1100111,
  B_INSTR = 0b1100011,
  L_INSTR = 0b0000011, // type I: load
  S_INSTR = 0b0100011,
  I_INSTR = 0b0010011, // type I: immediate
  R_INSTR = 0b0110011,
};

// instr[14:12]: type B identifier code
enum class b_instr_code : uint8_t {
  BEQ  = 0b000,
  BNE  = 0b001,
  BLT  = 0b100,
  BGE  = 0b101,
  BLTU = 0b110,
  BGEU = 0b111,
};

// instr[14:12]: type I-load identifier code
enum class l_instr_code : uint8_t {
  LB  = 0b000,
  LH  = 0b001,
  LW  = 0b010,
  LBU = 0b100,
  LHU = 0b101,
};

// instr[14:12]: type S identifier code
enum class s_instr_code : uint8_t {
  SB = 0b000,
  SH = 0b001,
  SW = 0b010,
};

// instr[14:12]: type I-immediate identifier code
enum class i_instr_code : uint8_t {
  ADDI       = 0b000,
  SLTI       = 0b010,
  SLTIU      = 0b011,
  XORI       = 0b100,
  ORI        = 0b110,
  ANDI       = 0b111,
  SLLI       = 0b001,
  SRLI_SRAI  = 0b101,
};

// instr[31:25]: srli/srai identifier code
enum class srli_srai_code : uint8_t {
  SRLI = 0b0000000,
  SRAI = 0b0100000,
};

// instr[14:12]: type R identifier code
enum class r_instr_code : uint8_t {
  ADD_SUB = 0b000,
  SLL     = 0b001,
  SLT     = 0b010,
  SLTU    = 0b011,
  XOR     = 0b100,
  SRL_SRA = 0b101,
  OR      = 0b110,
  AND     = 0b111,
};

// instr[31:25]: add_sub identifier code
enum class add_sub_code : uint8_t {
  ADD = 0b0000000,
  SUB = 0b0100000,
};

// instr[31:25]: srl/sra identifier code
enum class srl_sra_code : uint8_t {
  SRL = 0b0000000,
  srA = 0b0100000,
};

class Instruction {
public:
  Instruction() = default;
  explicit Instruction(raw_instr_t instr) : _instr(instr) { parse(); }

  void parse() {
    _fields.funct3    = slice_bytes<uint8_t, 14, 12>(_instr);
    _fields.funct7    = slice_bytes<uint8_t, 31, 25>(_instr);
    _fields.rs2_shamt = slice_bytes<uint8_t, 24, 20>(_instr);
    _fields.rs1       = slice_bytes<uint8_t, 19, 15>(_instr);
    _fields.rd        = slice_bytes<uint8_t, 11, 7>(_instr);
    _fields.opcode    = slice_bytes<uint8_t, 6, 0>(_instr);
    switch(static_cast<opcode_t>(_fields.opcode)) {
    case opcode_t::LUI: {
      _type = InstrType::LUI;
      _fields.imm = slice_bytes<int32_t, 31, 12>(_instr);
    } break;
    case opcode_t::AUIPC: {
      _type = InstrType::AUIPC;
      _fields.imm = slice_bytes<int32_t, 31, 12>(_instr);
    } break;
    case opcode_t::JAL: {
      _type = InstrType::JAL;
      int32_t imm20    = slice_bytes<int32_t, 31, 31>(_instr);
      int32_t imm10_1  = slice_bytes<int32_t, 30, 21>(_instr);
      int32_t imm11    = slice_bytes<int32_t, 20, 20>(_instr);
      int32_t imm19_12 = slice_bytes<int32_t, 19, 12>(_instr);
      _fields.imm = (imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1);
    } break;
    case opcode_t::JALR: {
      _type = InstrType::JALR;
      _fields.imm = slice_bytes<int32_t, 31, 20>(_instr);
    } break;
    case opcode_t::B_INSTR: {
      int32_t imm12    = slice_bytes<int32_t, 31, 31>(_instr);
      int32_t imm10_5  = slice_bytes<int32_t, 30, 25>(_instr);
      int32_t imm4_1   = slice_bytes<int32_t, 11, 8>(_instr);
      int32_t imm11    = slice_bytes<int32_t, 7, 7>(_instr);
      _fields.imm = (imm12 << 12) | (imm11 << 11)| (imm10_5 << 5) | (imm4_1 << 1);
      switch(static_cast<b_instr_code>(_fields.funct3)) {
      case b_instr_code::BEQ:  _type = InstrType::BEQ;  break;
      case b_instr_code::BNE:  _type = InstrType::BNE;  break;
      case b_instr_code::BLT:  _type = InstrType::BLT;  break;
      case b_instr_code::BGE:  _type = InstrType::BGE;  break;
      case b_instr_code::BLTU: _type = InstrType::BLTU; break;
      case b_instr_code::BGEU: _type = InstrType::BGEU; break;
      default: throw std::runtime_error("instruction parse error: unrecognizable B-instruction code.");
      }
    }  break;
    case opcode_t::L_INSTR: {
      _fields.imm = slice_bytes<int32_t, 31, 20>(_instr);
      switch(static_cast<l_instr_code>(_fields.funct3)) {
      case l_instr_code::LB:  _type = InstrType::LB;  break;
      case l_instr_code::LH:  _type = InstrType::LH;  break;
      case l_instr_code::LW:  _type = InstrType::LW;  break;
      case l_instr_code::LBU: _type = InstrType::LBU; break;
      case l_instr_code::LHU: _type = InstrType::LHU; break;
      default: throw std::runtime_error("instruction parse error: unrecognizable I (load)-instruction code.");
      }
    } break;
    case opcode_t::S_INSTR: {
      int32_t imm11_5 = slice_bytes<int32_t, 31, 25>(_instr);
      int32_t imm4_0  = slice_bytes<int32_t, 11, 7>(_instr);
      _fields.imm = (imm11_5 << 5) | (imm4_0 << 0);
      switch(static_cast<s_instr_code>(_fields.funct3)) {
      case s_instr_code::SB: _type = InstrType::SB; break;
      case s_instr_code::SH: _type = InstrType::SH; break;
      case s_instr_code::SW: _type = InstrType::SW; break;
      default: throw std::runtime_error("instruction parse error: unrecognizable S-instruction code.");
      }
    } break;
    case opcode_t::I_INSTR: {
      _fields.imm = slice_bytes<int32_t, 31, 20>(_instr);
      switch(static_cast<i_instr_code>(_fields.funct3)) {
      case i_instr_code::ADDI:  _type = InstrType::ADDI;  break;
      case i_instr_code::SLTI:  _type = InstrType::SLTI;  break;
      case i_instr_code::SLTIU: _type = InstrType::SLTIU; break;
      case i_instr_code::XORI:  _type = InstrType::XORI;  break;
      case i_instr_code::ORI:   _type = InstrType::ORI;   break;
      case i_instr_code::ANDI:  _type = InstrType::ANDI;  break;
      case i_instr_code::SLLI:  _type = InstrType::SLLI;  break;
      case i_instr_code::SRLI_SRAI: {
        switch(static_cast<srli_srai_code>(_fields.funct7)) {
        case srli_srai_code::SRLI: _type = InstrType::SRLI; break;
        case srli_srai_code::SRAI: _type = InstrType::SRAI; break;
        default: throw std::runtime_error("instruction parse error: unrecognizable I (imm)-instruction code.");
        }
      } break;
      default: throw std::runtime_error("instruction parse error: unrecognizable I (imm)-instruction code.");
      }
    } break;
    case opcode_t::R_INSTR: {
      switch(static_cast<r_instr_code>(_fields.funct3)) {
      case r_instr_code::ADD_SUB: {
        switch(static_cast<add_sub_code>(_fields.funct7)) {
        case add_sub_code::ADD: _type = InstrType::ADD; break;
        case add_sub_code::SUB: _type = InstrType::SUB; break;
        default: throw std::runtime_error("instruction parse error: unrecognizable R-instruction code.");
        }
      } break;
      case r_instr_code::SLL:  _type = InstrType::SLL;  break;
      case r_instr_code::SLT:  _type = InstrType::SLT;  break;
      case r_instr_code::SLTU: _type = InstrType::SLTU; break;
      case r_instr_code::XOR:  _type = InstrType::XOR;  break;
      case r_instr_code::SRL_SRA: {
        switch(static_cast<srl_sra_code>(_fields.funct7)) {
        case srl_sra_code::SRL: _type = InstrType::SRL; break;
        case srl_sra_code::srA: _type = InstrType::SRA; break;
        default: throw std::runtime_error("instruction parse error: unrecognizable R-instruction code.");
        }
      } break;
      case r_instr_code::OR:  _type = InstrType::OR;  break;
      case r_instr_code::AND: _type = InstrType::AND; break;
      default: throw std::runtime_error("instruction parse error: unrecognizable R-instruction code.");
      }
    } break;
    default: throw std::runtime_error("instruction parse error: unrecognizable opcode.");
    }
  }

  InstrType type() const { return _type; }
  uint8_t rs1() const { return _fields.rs1; }
  uint8_t rs2() const { return _fields.rs2_shamt; }
  uint8_t rd() const { return _fields.rd; }
  uint8_t shamt() const { return _fields.rs2_shamt; }
  int32_t imm() const { return _fields.imm; }

private:
  raw_instr_t _instr;
  InstrType _type;
  struct {
    uint8_t  funct3;    // instr[14:12]
    uint8_t  funct7;    // instr[31:25]
    uint8_t  rs2_shamt; // instr[24:20]
    uint8_t  rs1;       // instr[19:15]
    uint8_t  rd;        // instr[11:7]  register destination
    uint8_t  opcode;    // instr[6:0]
    int32_t  imm;       // immediate value
  } _fields;
};

}

#endif // ISM_INSTRUCTION_H