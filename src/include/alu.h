#ifndef ISM_ALU_H
#define ISM_ALU_H

#include "common.h"
#include "utility.h"
#include "wire_harness.h"

namespace insomnia {

// Maybe you'll write some more differentiated ALUs?

// (High functional) Common Arithmetic and Logic Unit.
// Can do all kinds of calculation in 1 clock cycle.
class CommonALU : public CPUModule {
  struct Registers {
    bool is_busy = false;
    rob_index_t rob_index;
    InstrType instr_type = InstrType::INVALID;
    mem_val_t src1_value;
    mem_val_t src2_value;
    int32_t imm;
    uint8_t dst_reg;
    mem_ptr_t instr_addr;
    bool is_branch;
    mem_ptr_t pred_pc;
  };
public:
  CommonALU(
    std::shared_ptr<const WH_RS_ALU> rs_input,
    std::shared_ptr<const WH_FLUSH_CDB> flush_input,
    std::shared_ptr<WH_ALU_CDB> cdb_output,
    std::shared_ptr<WH_ALU_RS> rs_output
    ) :
  _rs_input(std::move(rs_input)),
  _flush_input(std::move(flush_input)),
  _cdb_output(std::move(cdb_output)),
  _rs_output(std::move(rs_output)),
  _cur_regs(), _nxt_regs() {}

  void sync() override {
    _cur_regs = _nxt_regs;
  }

  bool update() override {
    debug("ALU");
    _nxt_regs = _cur_regs;

    WH_ALU_CDB cdb_output{};
    WH_ALU_RS rs_output{};


    if(_flush_input->is_flush) {
      _nxt_regs.is_busy = false;
      _nxt_regs.instr_type = InstrType::INVALID;
    }

    if(!_nxt_regs.is_busy) {
      if(_rs_input->is_valid) {
        _nxt_regs.is_busy = true;
        _nxt_regs.rob_index = _rs_input->rob_index;
        _nxt_regs.instr_type = _rs_input->instr_type;
        _nxt_regs.src1_value = _rs_input->src1_value;
        _nxt_regs.src2_value = _rs_input->src2_value;
        _nxt_regs.imm = _rs_input->imm;
        _nxt_regs.dst_reg = _rs_input->dst_reg;
        _nxt_regs.instr_addr = _rs_input->instr_addr;
        _nxt_regs.is_branch = _rs_input->is_branch;
        _nxt_regs.pred_pc = _rs_input->pred_pc;
      }
    }

    if(_nxt_regs.is_busy) {
      mem_val_t result = 0;
      mem_ptr_t real_branch_pc = 0;
      bool is_branch = false;
      uint32_t shamt = static_cast<uint32_t>(_nxt_regs.imm) & 0x1f;

      switch(_nxt_regs.instr_type) {
      case InstrType::ADD:
        result = _nxt_regs.src1_value + _nxt_regs.src2_value;
        break;
      case InstrType::ADDI:
        result = _nxt_regs.src1_value + _nxt_regs.imm;
        break;
      case InstrType::SUB:
        result = _nxt_regs.src1_value - _nxt_regs.src2_value;
        break;
      case InstrType::SLT:
        result = (_nxt_regs.src1_value < _nxt_regs.src2_value) ? 1 : 0;
        break;
      case InstrType::SLTU:
        result = (static_cast<uint64_t>(_nxt_regs.src1_value) < static_cast<uint64_t>(_nxt_regs.src2_value)) ? 1 : 0;
        break;
      case InstrType::XOR:
        result = _nxt_regs.src1_value ^ _nxt_regs.src2_value;
        break;
      case InstrType::XORI:
        result = _nxt_regs.src1_value ^ _nxt_regs.imm;
        break;
      case InstrType::OR:
        result = _nxt_regs.src1_value | _nxt_regs.src2_value;
        break;
      case InstrType::ORI:
        result = _nxt_regs.src1_value | _nxt_regs.imm;
        break;
      case InstrType::AND:
        result = _nxt_regs.src1_value & _nxt_regs.src2_value;
        break;
      case InstrType::ANDI:
        result = _nxt_regs.src1_value & _nxt_regs.imm;
        break;
      case InstrType::SLL:
        result = _nxt_regs.src1_value << (_nxt_regs.src2_value & 0x1f); // shamt from rs2 (lower 5 bits)
        break;
      case InstrType::SLLI:
        result = _nxt_regs.src1_value << shamt;
        break;
      case InstrType::SRL:
        result = static_cast<uint32_t>(_nxt_regs.src1_value) >> (_nxt_regs.src2_value & 0x1f);
        break;
      case InstrType::SRLI:
        result = static_cast<uint32_t>(_nxt_regs.src1_value) >> shamt;
        break;
      case InstrType::SRA:
        result = _nxt_regs.src1_value >> (_nxt_regs.src2_value & 0x1f);
        break;
      case InstrType::SRAI:
        result = _nxt_regs.src1_value >> shamt;
        break;
      case InstrType::LUI:
        result = _nxt_regs.imm << 12;
        break;
      case InstrType::AUIPC:
        result = _nxt_regs.instr_addr + (_nxt_regs.imm << 12);
        break;
      case InstrType::BEQ:
        is_branch = true;
        if(_nxt_regs.src1_value == _nxt_regs.src2_value) {
          real_branch_pc = _nxt_regs.instr_addr + _nxt_regs.imm;
        } else {
          real_branch_pc = _nxt_regs.instr_addr + 4;
        }
        break;
      case InstrType::BNE:
        is_branch = true;
        if(_nxt_regs.src1_value != _nxt_regs.src2_value) {
          real_branch_pc = _nxt_regs.instr_addr + _nxt_regs.imm;
        } else {
          real_branch_pc = _nxt_regs.instr_addr + 4;
        }
        break;
      case InstrType::BLT:
        is_branch = true;
        if(_nxt_regs.src1_value < _nxt_regs.src2_value) {
          real_branch_pc = _nxt_regs.instr_addr + _nxt_regs.imm;
        } else {
          real_branch_pc = _nxt_regs.instr_addr + 4;
        }
        break;
      case InstrType::BGE:
        is_branch = true;
        if(_nxt_regs.src1_value >= _nxt_regs.src2_value) {
          real_branch_pc = _nxt_regs.instr_addr + _nxt_regs.imm;
        } else {
          real_branch_pc = _nxt_regs.instr_addr + 4;
        }
        break;
      case InstrType::BLTU:
        is_branch = true;
        if(static_cast<uint64_t>(_nxt_regs.src1_value) < static_cast<uint64_t>(_nxt_regs.src2_value)) {
          real_branch_pc = _nxt_regs.instr_addr + _nxt_regs.imm;
        } else {
          real_branch_pc = _nxt_regs.instr_addr + 4;
        }
        break;
      case InstrType::BGEU:
        is_branch = true;
        if(static_cast<uint64_t>(_nxt_regs.src1_value) >= static_cast<uint64_t>(_nxt_regs.src2_value)) {
          real_branch_pc = _nxt_regs.instr_addr + _nxt_regs.imm;
        } else {
          real_branch_pc = _nxt_regs.instr_addr + 4;
        }
        break;
      case InstrType::JAL:
        is_branch = true;
        real_branch_pc = _nxt_regs.instr_addr + _nxt_regs.imm;
        result = _nxt_regs.instr_addr + 4;
        break;
      case InstrType::JALR:
        is_branch = true;
        real_branch_pc = (_nxt_regs.src1_value + _nxt_regs.imm) & ~1;
        result = _nxt_regs.instr_addr + 4;
        break;
      default:
        break;
      }

      cdb_output.entry = CDBEntry{
        .is_valid = true,
        .rob_index = _nxt_regs.rob_index,
        .real_pc = is_branch ? real_branch_pc : 0,
        .value = result
      };

      _nxt_regs.is_busy = false;
      _nxt_regs.instr_type = InstrType::INVALID;
    }

    rs_output = WH_ALU_RS{
      .can_accept_instr = !_nxt_regs.is_busy
    };

    bool update_signal = false;

    if(*_cdb_output != cdb_output) {
      *_cdb_output = cdb_output;
      update_signal = true;
    }

    if(*_rs_output != rs_output) {
      *_rs_output = rs_output;
      update_signal = true;
    }

    return update_signal;
  }

private:
  const std::shared_ptr<const WH_RS_ALU> _rs_input;
  const std::shared_ptr<WH_ALU_CDB> _cdb_output;
  const std::shared_ptr<WH_ALU_RS> _rs_output;
  const std::shared_ptr<const WH_FLUSH_CDB> _flush_input;

  Registers _cur_regs, _nxt_regs;
};
}

#endif // ISM_ALU_H