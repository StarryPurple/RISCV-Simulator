#ifndef ISM_LOAD_STORE_BUFFER_H
#define ISM_LOAD_STORE_BUFFER_H

#include <unordered_map>

#include "common.h"
#include "circular_queue.h"
#include "wire_harness.h"

namespace insomnia {

template <std::size_t BufSize>
class LoadStoreBuffer : public CPUModule {
  public:
    struct Entry {
      bool is_valid = false;
      bool is_load = false;
      bool is_store = false;
      rob_index_t rob_index;

      bool is_addr_ready = false;
      mem_ptr_t addr = 0;

      bool is_data_ready = false;
      mem_val_t data = 0;
      mptr_diff_t data_len = 0;

      bool miu_request_sent = false;
      bool is_executed = false;
      bool is_committed = false;
      bool is_finished = false;
    };

    struct Registers {
      circular_queue<Entry, BufSize> entries;

      bool load_sent = false;
      bool store_sent = false;
    };

  public:
    LoadStoreBuffer(
      std::shared_ptr<const WH_MIU_LSB> miu_input,
      std::shared_ptr<const WH_DU_LSB> du_input,
      std::shared_ptr<const WH_ROB_LSB> rob_input,
      std::shared_ptr<const WH_FLUSH_CDB> flush_input,
      std::shared_ptr<const WH_CDB_OUT> data_input,
      std::shared_ptr<WH_LSB_MIU> miu_output,
      std::shared_ptr<WH_LSB_CDB> data_output
    ) :
      _miu_input(std::move(miu_input)), _du_input(std::move(du_input)),
      _rob_input(std::move(rob_input)), _flush_input(std::move(flush_input)),
      _data_input(std::move(data_input)),
      _miu_output(std::move(miu_output)), _data_output(std::move(data_output)),
      _cur_regs(), _nxt_regs() {}

    void sync() override {
      _cur_regs = _nxt_regs;
    }

    bool update() override {
      debug("LSB");
      _nxt_regs = _cur_regs;

      WH_LSB_MIU miu_output{};
      WH_LSB_CDB data_output{};
      _nxt_regs.load_sent = false;
      _nxt_regs.store_sent = false;

      bool update_signal = false;

      if(_flush_input->is_flush) {
        _nxt_regs.entries.clear();
        update_signal = true;
        if(*_miu_output != miu_output) {
          *_miu_output = miu_output;
          update_signal = true;
        }
        if(*_data_output != data_output) {
          *_data_output = data_output;
          update_signal = true;
        }
        return update_signal;
      }

      if(_miu_input->is_valid) {
        if(!_nxt_regs.entries.empty()) {
          auto &entry = _nxt_regs.entries.front();
          if(entry.is_valid && entry.is_load && !entry.is_data_ready) {
            entry.data = _miu_input->value;
            entry.is_data_ready = true;
            entry.is_executed = true;

            data_output.entry.is_valid = true;
            data_output.entry.rob_index = entry.rob_index;
            data_output.entry.value = entry.data;
          }
        }
      }

      if(_du_input->is_valid) {
        if(!_nxt_regs.entries.full()) {
          Entry entry{};
          entry.is_valid = true;
          entry.rob_index = _du_input->rob_index;
          entry.is_load = _du_input->is_load;
          entry.is_store = _du_input->is_store;
          entry.addr = _du_input->addr;
          entry.is_addr_ready = true;
          entry.data_len = _du_input->data_len;

          if(entry.is_store) {
            entry.data = _du_input->value;
            entry.is_data_ready = _du_input->is_store_data_ready;
          } else {
            entry.is_data_ready = false;
          }

          _nxt_regs.entries.push(entry);
        }
      }

      for(std::size_t i = 0; i < _nxt_regs.entries.size(); ++i) {
        auto &entry = _nxt_regs.entries.at(_nxt_regs.entries.front_index() + i);
        if(entry.is_valid && entry.rob_index == _data_input->entry.rob_index &&
           entry.is_store && !entry.is_data_ready) {
          entry.data = _data_input->entry.value;
          entry.is_data_ready = true;
          entry.is_executed = true;
          break;
        }
      }

      if(_rob_input->is_valid) {
        for(std::size_t i = 0; i < _nxt_regs.entries.size(); ++i) {
          auto &entry = _nxt_regs.entries.at(_nxt_regs.entries.front_index() + i);
          if(entry.is_valid && entry.rob_index == _rob_input->rob_index) {
            entry.is_committed = true;
            break;
          }
        }
      }

      for(std::size_t i = 0; i < _nxt_regs.entries.size(); ++i) {
        auto &entry = _nxt_regs.entries.at(_nxt_regs.entries.front_index() + i);
        if(!entry.is_valid) continue;

        if(entry.is_load && !entry.is_data_ready) {
          bool forwarded = false;
          for(std::size_t j = 0; j < i; ++j) {
            auto& older_entry = _nxt_regs.entries.at(_nxt_regs.entries.front_index() + j);
            if(older_entry.is_valid && older_entry.is_store && older_entry.is_data_ready &&
               older_entry.addr == entry.addr && older_entry.data_len == entry.data_len) {
              entry.data = older_entry.data;
              entry.is_data_ready = true;
              entry.is_executed = true;
              forwarded = true;
              data_output.entry.is_valid = true;
              data_output.entry.rob_index = entry.rob_index;
              data_output.entry.value = entry.data;
              entry.is_finished = true;
              break;
            }
          }

          if(!forwarded && !entry.miu_request_sent && !_nxt_regs.load_sent) {
            miu_output.is_load_request = true;
            miu_output.addr = entry.addr;
            miu_output.data_len = entry.data_len;
            entry.miu_request_sent = true;
            _nxt_regs.load_sent = true;
          }
        }

        if(entry.is_store && entry.is_executed && entry.is_committed &&
           !entry.is_finished && i == 0) {

          if(!_nxt_regs.store_sent) {
            miu_output.is_store_request = true;
            miu_output.addr = entry.addr;
            miu_output.value = entry.data;
            miu_output.data_len = entry.data_len;
            entry.is_finished = true;
            _nxt_regs.store_sent = true;
          }
        }
      }

      while(_nxt_regs.entries.size() > 0) {
        auto &entry = _nxt_regs.entries.front();
        if(entry.is_valid && entry.is_committed && entry.is_finished) {
          _nxt_regs.entries.pop();
        } else {
          break;
        }
      }

      if(*_miu_output != miu_output) {
        *_miu_output = miu_output;
        update_signal = true;
      }
      if(*_data_output != data_output) {
        *_data_output = data_output;
        update_signal = true;
      }
      return update_signal;
    }

  private:
    const std::shared_ptr<const WH_MIU_LSB> _miu_input;
    const std::shared_ptr<const WH_DU_LSB> _du_input;
    const std::shared_ptr<const WH_ROB_LSB> _rob_input;
    const std::shared_ptr<const WH_FLUSH_CDB> _flush_input;
    const std::shared_ptr<const WH_CDB_OUT> _data_input;

    const std::shared_ptr<WH_LSB_MIU> _miu_output;
    const std::shared_ptr<WH_LSB_CDB> _data_output;

    Registers _cur_regs, _nxt_regs;
};

}


#endif // ISM_LOAD_STORE_BUFFER_H