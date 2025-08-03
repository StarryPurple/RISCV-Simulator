#ifndef ISM_RESERVATION_STATION_H
#define ISM_RESERVATION_STATION_H

#include "wire_harness.h"
#include "common.h"

namespace insomnia {

template <std::size_t StnSize>
class ReservationStation : public CPUModule {
  struct Entry {
    bool is_valid = false;
    rob_index_t rob_index;
    InstrType instr_type = InstrType::INVALID;
    mem_ptr_t instr_addr;

    bool src1_ready = false;
    mem_val_t src1_value;
    rob_index_t src1_index;

    bool src2_ready = false;
    mem_val_t src2_value;
    rob_index_t src2_index;

    int32_t imm;
    uint8_t dst_reg;

    bool is_branch;

    mem_ptr_t pred_pc;

    auto operator<=>(const Entry &) const = default;
  };
  struct Registers {
    std::array<Entry, StnSize> entries;
    std::size_t size = 0;
  };
public:
  ReservationStation(
    std::shared_ptr<const WH_DU_RS> du_input,
    std::shared_ptr<const WH_CDB_OUT> cdb_input,
    std::shared_ptr<const WH_FLUSH_CDB> flush_input,
    std::shared_ptr<const WH_ALU_RS> alu_input,
    std::shared_ptr<WH_RS_ALU> alu_output,
    std::shared_ptr<WH_RS_DU> rs_output_for_du
    ) :
  _du_input(std::move(du_input)),
  _cdb_input(std::move(cdb_input)),
  _flush_input(std::move(flush_input)),
  _alu_input(std::move(alu_input)),
  _alu_output(std::move(alu_output)),
  _du_output(std::move(rs_output_for_du)),
  _cur_regs(), _nxt_regs() {}

  void sync() override {
    _cur_regs = _nxt_regs;
  }

  bool update() override {
    // debug("RS");
    _nxt_regs = _cur_regs;

    WH_RS_ALU alu_output{};
    WH_RS_DU du_output{};

    bool update_signal = false;

    if(_flush_input->is_flush) {
      for(std::size_t i = 0; i < StnSize; ++i) {
        _nxt_regs.entries[i].is_valid = false;
      }
      _nxt_regs.size = 0;
    }

    if(_cdb_input->entry.is_valid) {
      for(std::size_t i = 0; i < StnSize; ++i) {
        if(_nxt_regs.entries[i].is_valid) {
          if(!_nxt_regs.entries[i].src1_ready && _nxt_regs.entries[i].src1_index == _cdb_input->entry.rob_index) {
            _nxt_regs.entries[i].src1_value = _cdb_input->entry.value;
            _nxt_regs.entries[i].src1_ready = true;
          }
          if(!_nxt_regs.entries[i].src2_ready && _nxt_regs.entries[i].src2_index == _cdb_input->entry.rob_index) {
            _nxt_regs.entries[i].src2_value = _cdb_input->entry.value;
            _nxt_regs.entries[i].src2_ready = true;
          }
        }
      }
    }

    std::size_t dispatch_idx = StnSize;
    rob_index_t earliest_rob_idx = std::numeric_limits<rob_index_t>::max();

    if(_alu_input->can_accept_instr) {
      for(std::size_t i = 0; i < StnSize; ++i) {
        Entry& entry = _nxt_regs.entries[i];
        if(entry.is_valid && entry.src1_ready && entry.src2_ready) {
          if(entry.rob_index < earliest_rob_idx) {
            earliest_rob_idx = entry.rob_index;
            dispatch_idx = i;
          }
        }
      }
    }

    if(dispatch_idx != StnSize) {
      Entry& dispatched_entry = _nxt_regs.entries[dispatch_idx];

      alu_output = WH_RS_ALU{
        .is_valid = true,
        .rob_index = dispatched_entry.rob_index,
        .instr_type = dispatched_entry.instr_type,
        .src1_value = dispatched_entry.src1_value,
        .src2_value = dispatched_entry.src2_value,
        .imm = dispatched_entry.imm,
        .dst_reg = dispatched_entry.dst_reg,
        .instr_addr = dispatched_entry.instr_addr,
        .is_branch = dispatched_entry.is_branch,
        .pred_pc = dispatched_entry.pred_pc
      };

      dispatched_entry.is_valid = false;
      --_nxt_regs.size;
    }

    // find a free slot for the incoming instruction
    std::size_t free_entry_idx = StnSize;
    if(_nxt_regs.size < StnSize) {
      for(std::size_t i = 0; i < StnSize; ++i) {
        if(!_nxt_regs.entries[i].is_valid) {
          free_entry_idx = i;
          break;
        }
      }
    }

    if(_du_input->is_valid && free_entry_idx != StnSize) {
        _nxt_regs.entries[free_entry_idx] = Entry{
          .is_valid = true,
          .rob_index = _du_input->rob_index,
          .instr_type = _du_input->instr_type,
          .instr_addr = _du_input->instr_addr,
          .src1_ready = _du_input->src1_ready,
          .src1_value = _du_input->src1_value,
          .src1_index = _du_input->src1_index,
          .src2_ready = _du_input->src2_ready,
          .src2_value = _du_input->src2_value,
          .src2_index = _du_input->src2_index,
          .imm = _du_input->imm,
          .dst_reg = _du_input->dst_reg,
          .is_branch = _du_input->is_branch,
          .pred_pc = _du_input->pred_pc
        };
        ++_nxt_regs.size;
    }

    du_output = WH_RS_DU{
      .can_accept_instr = (_nxt_regs.size < StnSize)
    };

    if(*_du_output != du_output) {
      *_du_output = du_output;
      update_signal = true;
    }

    if(*_alu_output != alu_output) {
      *_alu_output = alu_output;
      update_signal = true;
    }

    return update_signal;
  }

private:
  const std::shared_ptr<const WH_DU_RS> _du_input;
  const std::shared_ptr<const WH_CDB_OUT> _cdb_input;
  const std::shared_ptr<const WH_FLUSH_CDB> _flush_input;
  const std::shared_ptr<const WH_ALU_RS> _alu_input;

  const std::shared_ptr<WH_RS_ALU> _alu_output;
  const std::shared_ptr<WH_RS_DU> _du_output;

  Registers _cur_regs, _nxt_regs;
};

}

#endif // ISM_RESERVATION_STATION_H