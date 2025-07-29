#ifndef ISM_REORDER_BUFFER_H
#define ISM_REORDER_BUFFER_H

#include "circular_queue.h"
#include "wire_harness.h"
#include "common.h"

namespace insomnia {

template <std::size_t BufSize>
class ReorderBuffer : public CPUModule {
  struct Entry {
    bool is_ready; // whether the result is valid

    bool is_br; // in case the predictor needs it
    bool is_jalr;
    mem_ptr_t instr_addr;
    mem_ptr_t pred_pc; // the pc provided by predictor
    mem_ptr_t real_pc; // the pc calculated

    bool is_store; // in case the LSB needs it
    mem_ptr_t store_addr; // valid when is_store is true
    mem_val_t store_value; // valid when is_store is true
    mptr_diff_t data_len; // valid

    // The following is not valid at branch instructions.
    // But they are valid at jump instructions, since they too have a register write-in.

    bool write_rf; // whether rf write is needed
    rf_index_t dst_reg; // 0~31, RF index
    mem_val_t rf_value; // execution result
  };
  struct Registers {
    circular_queue<Entry, BufSize> queue; // entries
  };
public:
  ReorderBuffer(
    std::shared_ptr<const WH_DU_ROB> du_input,
    std::shared_ptr<const WH_DATA_CDB> data_input,
    std::shared_ptr<WH_ROB_LSB> lsb_output,
    std::shared_ptr<WH_ROB_DU> du_output,
    std::shared_ptr<WH_ROB_PRED> pred_output,
    std::shared_ptr<WH_ROB_RF> rf_output,
    std::shared_ptr<WH_FLUSH_CDB> flush_output
    ) :
  _du_input(std::move(du_input)), _data_input(std::move(data_input)),
  _lsb_output(std::move(lsb_output)), _du_output(std::move(du_output)),
  _pred_output(std::move(pred_output)), _rf_output(std::move(rf_output)),
  _flush_output(std::move(flush_output)),
  _cur_regs(), _nxt_regs() {}
  void sync() override {
    _cur_regs = _nxt_regs;
  }
  bool update() override {
    _nxt_regs = _cur_regs;

    WH_ROB_LSB lsb_output{};
    WH_ROB_DU du_output{};
    WH_ROB_PRED pred_output{};
    WH_ROB_RF rf_output{};
    WH_FLUSH_CDB flush_output{};

    // assign new slots: DU
    // listen and accept execution results: CDB
    // commit value: RF
    // commit Store instructions: LSB
    // flush all: Flush Signal Pipeline
    // The operations above can be done in one cycle, so no state machine is needed in ROB.

    // This instruction might be flushed, so it must be judged before instruction commit.
    if(_du_input->is_valid && !_nxt_regs.queue.full()) {
      du_output.is_valid = true;
      _nxt_regs.queue.push(Entry{
        .is_ready = false,
        .is_br = _du_input->is_br,
        .is_jalr = _du_input->is_jalr,
        .instr_addr = _du_input->instr_addr,
        .pred_pc = _du_input->pred_pc,
        .is_store = _du_input->is_store,
        .store_addr = _du_input->store_addr,
        .store_value = _du_input->store_value,
        .data_len = _du_input->data_len,
        .dst_reg = _du_input->dst_reg,
        .write_rf = _du_input->write_rf
      });
      du_output.rob_index = _nxt_regs.queue.back_index();
    }
    for(const auto &entry: _data_input->entries) {
      if(entry.is_valid) {
        auto &record = _nxt_regs.queue.at(entry.rob_index);
        record.is_ready = true;
        if(record.is_br || record.is_jalr) {
          record.real_pc = entry.real_pc;
        }
        if(record.is_store) {
          record.store_value = entry.value;
        }
        if(record.write_rf) {
          record.rf_value = entry.value;
        }
      }
    }
    if(!_nxt_regs.queue.empty() && _nxt_regs.queue.front().is_ready) {
      auto record = _nxt_regs.queue.front();
      if(record.is_br || record.is_jalr) {
        // A branch/jalr instruction.
        // PRED interaction
        pred_output.is_valid = true;
        pred_output.instr_addr = record.instr_addr;
        pred_output.real_pc = record.real_pc;
        pred_output.is_br = record.is_br;
        pred_output.is_pred_taken = (record.pred_pc == record.real_pc);
        if(record.pred_pc != record.real_pc) {
          // Prediction failed. Broadcast flushing signal and clear everything.
          flush_output.is_flush = true;
          flush_output.pc = record.real_pc;
          _nxt_regs.queue.clear();
        } else if(record.write_rf) {
          // Prediction succeeded & instruction is jalr/need to write RF.
          // write x[rd] = val.
          // RF interaction
          rf_output.is_valid = true;
          rf_output.dst_reg = record.dst_reg;
          rf_output.value = record.rf_value;
          _nxt_regs.queue.pop();
        }
      } else if(record.is_store) {
        // A memory store instruction.
        // LSB interaction
        lsb_output.is_valid = true;
        lsb_output.rob_index = _nxt_regs.queue.front_index();
        _nxt_regs.queue.pop();
      } else {
        // just a normal arithmetic instruction.
        // RF interaction
        rf_output.is_valid = true;
        rf_output.dst_reg = record.dst_reg;
        rf_output.value = record.rf_value;
        _nxt_regs.queue.pop();
      }
    }

    bool update_signal = false;
    if(*_lsb_output != lsb_output) {
      *_lsb_output = lsb_output;
      update_signal = true;
    }
    if(*_du_output != du_output) {
      *_du_output = du_output;
      update_signal = true;
    }
    if(*_pred_output != pred_output) {
      *_pred_output = pred_output;
      update_signal = true;
    }
    if(*_rf_output != rf_output) {
      *_rf_output = rf_output;
      update_signal = true;
    }
    if(*_flush_output != flush_output) {
      *_flush_output = flush_output;
      update_signal = true;
    }
    return update_signal;
  }
private:
  const std::shared_ptr<const WH_DU_ROB> _du_input;
  const std::shared_ptr<const WH_DATA_CDB> _data_input;
  const std::shared_ptr<WH_ROB_LSB> _lsb_output;
  const std::shared_ptr<WH_ROB_DU> _du_output;
  const std::shared_ptr<WH_ROB_PRED> _pred_output;
  const std::shared_ptr<WH_ROB_RF> _rf_output;
  const std::shared_ptr<WH_FLUSH_CDB> _flush_output;
  Registers _cur_regs, _nxt_regs;
};


}

#endif // ISM_REORDER_BUFFER_H