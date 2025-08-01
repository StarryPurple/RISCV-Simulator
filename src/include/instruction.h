#ifndef ISM_INSTRUCTION_H
#define ISM_INSTRUCTION_H

#include <cstdint>
#include <limits>

#include "common.h"
#include "utility.h"

/********************** instruction codes ***********************/

namespace insomnia {

using raw_instr_t = uint32_t; // RV32I 4 bytes

enum class InstrType {
  INVALID,
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
  JALR    = 0b1100111, // actually its type is I
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

// Decoder integrated as function resolve(instr).
class Instruction {
public:
  Instruction() = default;
  explicit Instruction(raw_instr_t raw_instr) { resolve(raw_instr); }

  void resolve(raw_instr_t raw_instr) {
    _raw_instr = raw_instr;
    _type = InstrType::INVALID;
    _rs2_shamt = slice_bytes<uint8_t, 24, 20>(raw_instr);
    _rs1       = slice_bytes<uint8_t, 19, 15>(raw_instr);
    _rd        = slice_bytes<uint8_t, 11,  7>(raw_instr);
    auto funct3 = slice_bytes<uint8_t, 14, 12>(raw_instr);
    auto funct7 = slice_bytes<uint8_t, 31, 25>(raw_instr);
    auto opcode = slice_bytes<uint8_t,  6,  0>(raw_instr);
    switch(static_cast<opcode_t>(opcode)) {
    case opcode_t::LUI: {
      _type = InstrType::LUI;
      _imm = slice_bytes<int32_t, 31, 12>(raw_instr);
    } break;
    case opcode_t::AUIPC: {
      _type = InstrType::AUIPC;
      _imm = slice_bytes<int32_t, 31, 12>(raw_instr);
    } break;
    case opcode_t::JAL: {
      _type = InstrType::JAL;
      auto imm20    = slice_bytes<int32_t, 31, 31>(raw_instr);
      auto imm10_1  = slice_bytes<int32_t, 30, 21>(raw_instr);
      auto imm11    = slice_bytes<int32_t, 20, 20>(raw_instr);
      auto imm19_12 = slice_bytes<int32_t, 19, 12>(raw_instr);
      _imm = (imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1);
      _imm = sign_extend<int32_t, 21>(_imm);
    } break;
    case opcode_t::JALR: {
      _type = InstrType::JALR;
      _imm = slice_bytes<int32_t, 31, 20>(raw_instr);
      _imm = sign_extend<int32_t, 12>(_imm);
    } break;
    case opcode_t::B_INSTR: {
      auto imm12    = slice_bytes<int32_t, 31, 31>(raw_instr);
      auto imm10_5  = slice_bytes<int32_t, 30, 25>(raw_instr);
      auto imm4_1   = slice_bytes<int32_t, 11,  8>(raw_instr);
      auto imm11    = slice_bytes<int32_t,  7,  7>(raw_instr);
      _imm = (imm12 << 12) | (imm11 << 11)| (imm10_5 << 5) | (imm4_1 << 1);
      _imm = sign_extend<int32_t, 13>(_imm);
      switch(static_cast<b_instr_code>(funct3)) {
      case b_instr_code::BEQ:  _type = InstrType::BEQ;  break;
      case b_instr_code::BNE:  _type = InstrType::BNE;  break;
      case b_instr_code::BLT:  _type = InstrType::BLT;  break;
      case b_instr_code::BGE:  _type = InstrType::BGE;  break;
      case b_instr_code::BLTU: _type = InstrType::BLTU; break;
      case b_instr_code::BGEU: _type = InstrType::BGEU; break;
      default: break;
        // throw std::runtime_error("instruction parse error: unrecognizable B-instruction code.");
      }
    }  break;
    case opcode_t::L_INSTR: {
      _imm = slice_bytes<int32_t, 31, 20>(raw_instr);
      _imm = sign_extend<int32_t, 12>(_imm);
      switch(static_cast<l_instr_code>(funct3)) {
      case l_instr_code::LB:  _type = InstrType::LB;  break;
      case l_instr_code::LH:  _type = InstrType::LH;  break;
      case l_instr_code::LW:  _type = InstrType::LW;  break;
      case l_instr_code::LBU: _type = InstrType::LBU; break;
      case l_instr_code::LHU: _type = InstrType::LHU; break;
      default: break;
        // throw std::runtime_error("instruction parse error: unrecognizable I (load)-instruction code.");
      }
    } break;
    case opcode_t::S_INSTR: {
      auto imm11_5 = slice_bytes<int32_t, 31, 25>(raw_instr);
      auto imm4_0  = slice_bytes<int32_t, 11,  7>(raw_instr);
      _imm = (imm11_5 << 5) | (imm4_0 << 0);
      _imm = sign_extend<int32_t, 12>(_imm);
      switch(static_cast<s_instr_code>(funct3)) {
      case s_instr_code::SB: _type = InstrType::SB; break;
      case s_instr_code::SH: _type = InstrType::SH; break;
      case s_instr_code::SW: _type = InstrType::SW; break;
      default: break;
        // throw std::runtime_error("instruction parse error: unrecognizable S-instruction code.");
      }
    } break;
    case opcode_t::I_INSTR: {
      _imm = slice_bytes<int32_t, 31, 20>(raw_instr);
      _imm = sign_extend<int32_t, 12>(_imm);
      switch(static_cast<i_instr_code>(funct3)) {
      case i_instr_code::ADDI:  _type = InstrType::ADDI;  break;
      case i_instr_code::SLTI:  _type = InstrType::SLTI;  break;
      case i_instr_code::SLTIU: _type = InstrType::SLTIU; break;
      case i_instr_code::XORI:  _type = InstrType::XORI;  break;
      case i_instr_code::ORI:   _type = InstrType::ORI;   break;
      case i_instr_code::ANDI:  _type = InstrType::ANDI;  break;
      case i_instr_code::SLLI:  _type = InstrType::SLLI; _imm = _rs2_shamt;  break;
      case i_instr_code::SRLI_SRAI: {
        _imm = _rs2_shamt;
        switch(static_cast<srli_srai_code>(funct7)) {
        case srli_srai_code::SRLI: _type = InstrType::SRLI; break;
        case srli_srai_code::SRAI: _type = InstrType::SRAI; break;
        default: break;
          // throw std::runtime_error("instruction parse error: unrecognizable I (imm)-instruction code.");
        }
      } break;
      default: break;
        // throw std::runtime_error("instruction parse error: unrecognizable I (imm)-instruction code.");
      }
    } break;
    case opcode_t::R_INSTR: {
      switch(static_cast<r_instr_code>(funct3)) {
      case r_instr_code::ADD_SUB: {
        switch(static_cast<add_sub_code>(funct7)) {
        case add_sub_code::ADD: _type = InstrType::ADD; break;
        case add_sub_code::SUB: _type = InstrType::SUB; break;
        default: break;
          // throw std::runtime_error("instruction parse error: unrecognizable R-instruction code.");
        }
      } break;
      case r_instr_code::SLL:  _type = InstrType::SLL;  break;
      case r_instr_code::SLT:  _type = InstrType::SLT;  break;
      case r_instr_code::SLTU: _type = InstrType::SLTU; break;
      case r_instr_code::XOR:  _type = InstrType::XOR;  break;
      case r_instr_code::SRL_SRA: {
        switch(static_cast<srl_sra_code>(funct7)) {
        case srl_sra_code::SRL: _type = InstrType::SRL; break;
        case srl_sra_code::srA: _type = InstrType::SRA; break;
        default: break;
          // throw std::runtime_error("instruction parse error: unrecognizable R-instruction code.");
        }
      } break;
      case r_instr_code::OR:  _type = InstrType::OR;  break;
      case r_instr_code::AND: _type = InstrType::AND; break;
      default: break;
        // throw std::runtime_error("instruction parse error: unrecognizable R-instruction code.");
      }
    } break;
    default: break;
      // throw std::runtime_error("instruction parse error: unrecognizable opcode.");
    }
  }

  raw_instr_t raw_instr() const { return _raw_instr; }
  bool valid() const { return _type != InstrType::INVALID; }
  InstrType type() const { return _type; }
  uint8_t rs1() const { return _rs1; }
  uint8_t rs2() const { return _rs2_shamt; }
  uint8_t rd() const { return _rd; }
  uint8_t shamt() const { return _rs2_shamt; }
  int32_t imm() const { return _imm; }

  bool has_src1() const {
    return !(_type == InstrType::LUI || _type == InstrType::AUIPC || _type == InstrType::JAL ||
      _type == InstrType::INVALID);
  }

  bool has_src2() const {
    switch(_type) {
    case InstrType::ADD: case InstrType::SUB:  case InstrType::SLL:
    case InstrType::SLT: case InstrType::SLTU: case InstrType::XOR:
    case InstrType::SRL: case InstrType::SRA:  case InstrType::OR:
    case InstrType::AND:
    case InstrType::BEQ: case InstrType::BNE:  case InstrType::BLT:
    case InstrType::BGE: case InstrType::BLTU: case InstrType::BGEU:
    case InstrType::SB:  case InstrType::SH:   case InstrType::SW:
      return true;
    default:
      return false;
    }
  }

  bool write_rf() const {
    switch(_type) {
    case InstrType::BEQ: case InstrType::BNE:  case InstrType::BLT:
    case InstrType::BGE: case InstrType::BLTU: case InstrType::BGEU:
    case InstrType::SB:  case InstrType::SH:   case InstrType::SW:
    case InstrType::INVALID:
      return false;
    default:
      return true;
    }
  }

  bool is_load() const {
    return _type == InstrType::LB || _type == InstrType::LH || _type == InstrType::LW ||
      _type == InstrType::LBU || _type == InstrType::LHU;
  }

  bool is_store() const {
    return _type == InstrType::SB || _type == InstrType::SH || _type == InstrType::SW;
  }

  bool is_br() const {
    return _type == InstrType::BEQ || _type == InstrType::BNE || _type == InstrType::BGE ||
      _type == InstrType::BLT || _type == InstrType::BGEU || _type == InstrType::BLTU;
  }

  bool is_jal() const {
    return _type == InstrType::JAL;
  }

  bool is_jalr() const {
    return _type == InstrType::JALR;
  }

  mptr_diff_t mem_data_len() const {
    switch(_type) {
    case InstrType::LB: case InstrType::LBU: case InstrType::SB: return 1;
    case InstrType::LH: case InstrType::LHU: case InstrType::SH: return 2;
    case InstrType::LW: case InstrType::SW: return 4;
    default: return 0;
    }
  }

  mem_ptr_t branch_target_pc(mem_ptr_t current_pc, mem_val_t rs1_val) const {
    if(is_br() || _type == InstrType::JAL) {
      return current_pc + _imm;
    }
    if(is_jalr()) {
      return (rs1_val + _imm) & ~static_cast<mem_ptr_t>(0b1);
    }
    return current_pc + 4;
  }

  auto operator<=>(const Instruction &) const = default; // should be alright

private:
  raw_instr_t _raw_instr;
  InstrType _type;
  // uint8_t  _funct3;    // instr[14:12]
  // uint8_t  _funct7;    // instr[31:25]
  uint8_t  _rs2_shamt; // instr[24:20]
  uint8_t  _rs1;       // instr[19:15]
  uint8_t  _rd;        // instr[11:7]  register destination
  // uint8_t  _opcode;    // instr[6:0]
  int32_t  _imm;       // immediate value
};

}

#endif // ISM_INSTRUCTION_H