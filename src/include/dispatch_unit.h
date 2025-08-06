#ifndef ISM_DISPATCH_UNIT_H
#define ISM_DISPATCH_UNIT_H

#include "instruction.h"
#include "wire_harness.h"

namespace insomnia {

class DispatchUnit : public CPUModule {
  class MappingTableEntry {
  public:
    bool is_ready = true;
    rob_index_t rob_index = 0;

    auto operator<=>(const MappingTableEntry &) const = default;
  };

  enum class State {
    IDLE,
    FETCHED_DECODED,
    WAIT_ROB_ALLOC,
    WAIT_OPERANDS,
    OPERANDS_READY,
    DISPATCHING,
    STALLED
  };

  struct Registers {
    State state = State::IDLE;

    bool instr_valid = false;
    Instruction instr;
    mem_ptr_t instr_addr;

    mem_ptr_t next_pc;

    bool src1_ready = false;
    mem_val_t src1_value = 0;
    rob_index_t src1_index = 0;

    bool src2_ready = false;
    mem_val_t src2_value = 0;
    rob_index_t src2_index = 0;

    rf_index_t dst_reg = 0;

    bool rob_request_sent = false;
    bool rob_entry_allocated_ack = false;
    rob_index_t alloc_rob_index = 0;
  };

  std::array<MappingTableEntry, RFSize> _mapping_table;

public:
  DispatchUnit(
    std::shared_ptr<const WH_IFU_DU> ifu_input,
    std::shared_ptr<const WH_RF_DU> rf_input,
    std::shared_ptr<const WH_ROB_DU> rob_input,
    std::shared_ptr<const WH_CDB_OUT> cdb_input,
    std::shared_ptr<const WH_FLUSH_PIPELINE> flush_input,
    std::shared_ptr<const WH_RS_DU> rs_input,
    std::shared_ptr<WH_DU_IFU> ifu_output,
    std::shared_ptr<WH_DU_RS> rs_output,
    std::shared_ptr<WH_DU_LSB> lsb_output,
    std::shared_ptr<WH_DU_RF> rf_output,
    std::shared_ptr<WH_DU_ROB> rob_output
    ) :
  _ifu_input(std::move(ifu_input)),
  _rf_input(std::move(rf_input)),
  _rob_input(std::move(rob_input)),
  _cdb_input(std::move(cdb_input)),
  _flush_input(std::move(flush_input)),
  _rs_input(std::move(rs_input)),
  _ifu_output(std::move(ifu_output)),
  _rs_output(std::move(rs_output)),
  _lsb_output(std::move(lsb_output)),
  _rf_output(std::move(rf_output)),
  _rob_output(std::move(rob_output)),
  _cur_regs(), _nxt_regs() {}

  void sync() override {
    _cur_regs = _nxt_regs;

    if(_cur_regs.rob_entry_allocated_ack) {
      _mapping_table[_cur_regs.dst_reg].is_ready = false;
      _mapping_table[_cur_regs.dst_reg].rob_index = _cur_regs.alloc_rob_index;
    }

    if(_cdb_input->lsb_entry.is_valid) {
      for(std::size_t i = 0; i < RFSize; ++i) {
        if(!_mapping_table[i].is_ready && _mapping_table[i].rob_index == _cdb_input->lsb_entry.rob_index) {
          _mapping_table[i].is_ready = true;
        }
      }
    }

    if(_cdb_input->alu_entry.is_valid) {
      for(std::size_t i = 0; i < RFSize; ++i) {
        if(!_mapping_table[i].is_ready && _mapping_table[i].rob_index == _cdb_input->alu_entry.rob_index) {
          _mapping_table[i].is_ready = true;
        }
      }
    }

    if(_rob_input->is_commit) {
      for(std::size_t i = 0; i < RFSize; ++i) {
        if(!_mapping_table[i].is_ready && _mapping_table[i].rob_index == _rob_input->commit_index) {
          _mapping_table[i].is_ready = true;
        }
      }
    }
  }

  bool update() override {
    // debug("DU");
    _nxt_regs = _cur_regs;

    WH_DU_IFU ifu_output{};
    WH_DU_RS rs_output{};
    WH_DU_LSB lsb_output{};
    WH_DU_RF rf_output{};
    WH_DU_ROB rob_output{};

    bool update_signal = false;

    _nxt_regs.rob_request_sent = false;
    _nxt_regs.rob_entry_allocated_ack = false;

    if(_flush_input->is_flush) {
      _nxt_regs.state = State::IDLE;
      _nxt_regs.instr_valid = false;
      _nxt_regs.rob_request_sent = false;
      _nxt_regs.rob_entry_allocated_ack = false;

      // Reset mapping table on flush
      for(std::size_t i = 0; i < RFSize; ++i){
        _mapping_table[i].is_ready = true;
      }

      ifu_output.can_accept_req = true;

      if(*_ifu_output != ifu_output) {
        *_ifu_output = ifu_output;
        update_signal = true;
      }
      if(*_rs_output != rs_output) {
        *_rs_output = rs_output;
        update_signal = true;
      }
      if(*_lsb_output != lsb_output) {
        *_lsb_output = lsb_output;
        update_signal = true;
      }
      if(*_rf_output != rf_output) {
        *_rf_output = rf_output;
        update_signal = true;
      }
      if(*_rob_output != rob_output) {
        *_rob_output = rob_output;
        update_signal = true;
      }
      return update_signal;
    }

    switch(_nxt_regs.state) {
    case State::IDLE: {
      ifu_output.can_accept_req = true;
      if(_ifu_input->is_valid) {
        // ifu_output.can_accept_req = false; // handle it first
        _nxt_regs.instr_valid = true;
        _nxt_regs.instr = Instruction(_ifu_input->raw_instr);
        _nxt_regs.instr_addr = _ifu_input->instr_addr;
        _nxt_regs.next_pc = _ifu_input->pred_pc;

        _nxt_regs.dst_reg = _nxt_regs.instr.rd();
        _nxt_regs.state = State::FETCHED_DECODED;
      }
    } break;

    case State::FETCHED_DECODED:
    case State::WAIT_ROB_ALLOC: {
      rob_output = WH_DU_ROB{
        .is_valid = true,
        .raw_instr = _nxt_regs.instr.raw_instr(),
        .is_br = _nxt_regs.instr.is_br(),
        .is_jalr = _nxt_regs.instr.is_jalr(),
        .instr_addr = _nxt_regs.instr_addr,
        .pred_pc = _nxt_regs.next_pc,
        .is_load = _nxt_regs.instr.is_load(),
        .is_store = _nxt_regs.instr.is_store(),
        .data_len = _nxt_regs.instr.mem_data_len(),
        .write_rf = _nxt_regs.instr.write_rf(),
        .dst_reg = _nxt_regs.instr.rd(),
        .instr = _nxt_regs.instr
      };
      _nxt_regs.rob_request_sent = true;
      if(_rob_input->is_alloc_valid) {
        _nxt_regs.alloc_rob_index = _rob_input->rob_index;
        _nxt_regs.rob_entry_allocated_ack = true;

        // fetch rf data using the old mapping table
        rf_index_t rs1_idx = _nxt_regs.instr.rs1();
        if(!_nxt_regs.instr.has_src1() || rs1_idx == 0) {
          _nxt_regs.src1_ready = true;
          _nxt_regs.src1_value = 0;
          _nxt_regs.src1_index = 0;
        } else if(_rob_input->has_src1) {
          // more updated than mapping table
          _nxt_regs.src1_ready = true;
          _nxt_regs.src1_index = 0;
          _nxt_regs.src1_value = _rob_input->src1;
        } else if(_mapping_table[rs1_idx].is_ready) {
          rf_output.is_valid = true;
          rf_output.reqRi = true;
          rf_output.Ri = rs1_idx;
          _nxt_regs.src1_ready = false;
          _nxt_regs.src1_index = 0;
        } else {
          _nxt_regs.src1_ready = false;
          _nxt_regs.src1_index = _mapping_table[rs1_idx].rob_index;
          _nxt_regs.src1_value = 0;
        }

        rf_index_t rs2_idx = _nxt_regs.instr.rs2();
        if(!_nxt_regs.instr.has_src2() || rs2_idx == 0) {
          _nxt_regs.src2_ready = true;
          _nxt_regs.src2_value = 0;
          _nxt_regs.src2_index = 0;
        } else if(_rob_input->has_src2) {
          // more updated than mapping table
          _nxt_regs.src2_ready = true;
          _nxt_regs.src2_index = 0;
          _nxt_regs.src2_value = _rob_input->src2;
        } else if(_mapping_table[rs2_idx].is_ready) {
          rf_output.is_valid = true;
          rf_output.reqRj = true;
          rf_output.Rj = rs2_idx;
          _nxt_regs.src2_ready = false;
          _nxt_regs.src2_index = 0;
        } else {
          _nxt_regs.src2_ready = false;
          _nxt_regs.src2_index = _mapping_table[rs2_idx].rob_index;
          _nxt_regs.src2_value = 0;
        }
        _nxt_regs.state = State::WAIT_OPERANDS;
      } else {
        _nxt_regs.state = State::WAIT_ROB_ALLOC;
        rob_output.is_valid = true; // ?
      }
    } break;

    case State::WAIT_OPERANDS: {
      if(_rf_input->is_valid) {
        if(!_nxt_regs.src1_ready) {
          _nxt_regs.src1_value = _rf_input->Vi;
          _nxt_regs.src1_ready = true;
        }
        if(!_nxt_regs.src2_ready) {
          _nxt_regs.src2_value = _rf_input->Vj;
          _nxt_regs.src2_ready = true;
        }
      }

      if(_cdb_input->lsb_entry.is_valid){
        if(!_nxt_regs.src1_ready && _nxt_regs.src1_index == _cdb_input->lsb_entry.rob_index){
          _nxt_regs.src1_value = _cdb_input->lsb_entry.value;
          _nxt_regs.src1_ready = true;
        }
        if(!_nxt_regs.src2_ready && _nxt_regs.src2_index == _cdb_input->lsb_entry.rob_index){
          _nxt_regs.src2_value = _cdb_input->lsb_entry.value;
          _nxt_regs.src2_ready = true;
        }
      }
      if(_cdb_input->alu_entry.is_valid){
        if(!_nxt_regs.src1_ready && _nxt_regs.src1_index == _cdb_input->alu_entry.rob_index){
          _nxt_regs.src1_value = _cdb_input->alu_entry.value;
          _nxt_regs.src1_ready = true;
        }
        if(!_nxt_regs.src2_ready && _nxt_regs.src2_index == _cdb_input->alu_entry.rob_index){
          _nxt_regs.src2_value = _cdb_input->alu_entry.value;
          _nxt_regs.src2_ready = true;
        }
      }

      if(_nxt_regs.src1_ready && _nxt_regs.src2_ready){
        _nxt_regs.state = State::OPERANDS_READY;
      }
    } break;

    case State::OPERANDS_READY: {
      bool is_branch_instr = _nxt_regs.instr.is_br() || _nxt_regs.instr.is_jal() || _nxt_regs.instr.is_jalr();

      if(_rs_input->can_accept_instr) {
        rs_output = WH_DU_RS{
          .is_valid = true,
          .rob_index = _nxt_regs.alloc_rob_index,
          .instr_type = _nxt_regs.instr.type(),
          .src1_ready = _nxt_regs.src1_ready,
          .src1_value = _nxt_regs.src1_value,
          .src1_index = _nxt_regs.src1_index,
          .src2_ready = _nxt_regs.src2_ready,
          .src2_value = _nxt_regs.src2_value,
          .src2_index = _nxt_regs.src2_index,
          .imm = _nxt_regs.instr.imm(),
          .dst_reg = _nxt_regs.dst_reg,
          .instr_addr = _nxt_regs.instr_addr,
          .is_branch = is_branch_instr,
          .pred_pc = _nxt_regs.instr_addr + 4
        };
        if(_nxt_regs.instr.is_load()) {
          lsb_output = WH_DU_LSB{
            .is_valid = true,
            .data_len = _nxt_regs.instr.mem_data_len(),
            .is_load = true,
            .rob_index = _nxt_regs.alloc_rob_index,
          };
        } else if(_nxt_regs.instr.is_store()) {
          lsb_output = WH_DU_LSB{
            .is_valid = true,
            .data_len = _nxt_regs.instr.mem_data_len(),
            .is_store = true,
            .data_ready = _nxt_regs.src2_ready,
            .data_index = _nxt_regs.src2_index,
            .data_value = _nxt_regs.src2_value,
            .rob_index = _nxt_regs.alloc_rob_index,
          };
        }
        _nxt_regs.state = State::DISPATCHING;
      } else {
        _nxt_regs.state = State::STALLED;
      }
    }
    break;

    case State::DISPATCHING: {
      _nxt_regs.state = State::IDLE;
      _nxt_regs.instr_valid = false;
      ifu_output.can_accept_req = false; // nah, no.
    } break;

    case State::STALLED: {
      if(_rs_input->can_accept_instr) {
        _nxt_regs.state = State::OPERANDS_READY;
      }
    } break;
    }

    if(*_ifu_output != ifu_output) {
      *_ifu_output = ifu_output;
      update_signal = true;
    }

    if(*_rs_output != rs_output) {
      *_rs_output = rs_output;
      update_signal = true;
    }

    if(*_lsb_output != lsb_output) {
      *_lsb_output = lsb_output;
      update_signal = true;
    }

    if(*_rf_output != rf_output) {
      *_rf_output = rf_output;
      update_signal = true;
    }

    if(*_rob_output != rob_output) {
      *_rob_output = rob_output;
      update_signal = true;
    }

    return update_signal;
  }
private:
  const std::shared_ptr<const WH_IFU_DU> _ifu_input;
  const std::shared_ptr<const WH_RF_DU> _rf_input;
  const std::shared_ptr<const WH_ROB_DU> _rob_input;
  const std::shared_ptr<const WH_CDB_OUT> _cdb_input;
  const std::shared_ptr<const WH_FLUSH_PIPELINE> _flush_input;
  const std::shared_ptr<const WH_RS_DU> _rs_input;

  const std::shared_ptr<WH_DU_IFU> _ifu_output;
  const std::shared_ptr<WH_DU_RS> _rs_output;
  const std::shared_ptr<WH_DU_LSB> _lsb_output;
  const std::shared_ptr<WH_DU_RF> _rf_output;
  const std::shared_ptr<WH_DU_ROB> _rob_output;

  Registers _cur_regs, _nxt_regs;
};

}

#endif // ISM_DISPATCH_UNIT_H