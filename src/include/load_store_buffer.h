#ifndef ISM_LOAD_STORE_BUFFER_H
#define ISM_LOAD_STORE_BUFFER_H

#include <cassert>
#include <unordered_map>

#include "common.h"
#include "circular_queue.h"
#include "wire_harness.h"

namespace insomnia {

template <std::size_t BufSize>
class LoadStoreBuffer : public CPUModule {
  struct Entry {
    bool is_valid = false;
    bool is_load = false;
    bool is_store = false;
    mptr_diff_t data_len;
    rob_index_t rob_index;

    bool addr_ready = false;
    mem_ptr_t addr_value;

    bool data_ready = false;
    rob_index_t data_index;
    mem_val_t data_value;

    bool miu_request_sent = false;
    bool is_executed = false;
    bool is_committed = false;
    bool is_finished = false;
  };

  struct Registers {
    circular_queue<Entry, BufSize> entries;

    bool load_sent = false;
    bool store_sent = false;

    std::size_t load_index;
    std::size_t store_index;

    bool accept_data = false;
    bool accept_addr = false;

    std::size_t data_index;
    std::size_t addr_index;

    mem_val_t data_ack;
    mem_ptr_t addr_ack;
  };

public:
  LoadStoreBuffer(
    std::shared_ptr<const WH_MIU_LSB> miu_input,
    std::shared_ptr<const WH_DU_LSB> du_input,
    std::shared_ptr<const WH_ROB_LSB> rob_input,
    std::shared_ptr<const WH_FLUSH_PIPELINE> flush_input,
    std::shared_ptr<const WH_CDB_OUT> data_input,
    std::shared_ptr<WH_LSB_ROB> rob_output,
    std::shared_ptr<WH_LSB_MIU> miu_output,
    std::shared_ptr<WH_LSB_CDB> data_output
  ) :
    _miu_input(std::move(miu_input)), _du_input(std::move(du_input)),
    _rob_input(std::move(rob_input)), _flush_input(std::move(flush_input)),
    _data_input(std::move(data_input)), _rob_output(std::move(rob_output)),
    _miu_output(std::move(miu_output)), _data_output(std::move(data_output)),
    _cur_regs(), _nxt_regs() {}

  void sync() override {
    _cur_regs = _nxt_regs;
  }

  bool update() override {
    // debug("LSB");
    _nxt_regs = _cur_regs;

    WH_LSB_ROB rob_output{};
    WH_LSB_MIU miu_output{};
    WH_LSB_CDB data_output{};
    _nxt_regs.load_sent = false;
    _nxt_regs.store_sent = false;

    // flush
    if(_flush_input->is_flush) {
      while(!_nxt_regs.entries.empty() && !_nxt_regs.entries.back().is_committed) {
        _nxt_regs.entries.pop_back();
      }

      _nxt_regs.load_sent = false;
      _nxt_regs.store_sent = false;
      _nxt_regs.accept_addr = false;
      _nxt_regs.accept_data = false;

      bool update_signal = false;

      if(*_rob_output != rob_output) {
        *_rob_output = rob_output;
        update_signal = true;
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

    // add entry
    if(_du_input->is_valid && !_nxt_regs.entries.full()) {
      debug("LSB: " + std::to_string(_du_input->rob_index));
      _nxt_regs.entries.push(Entry{
        .is_valid = true,
        .is_load = _du_input->is_load,
        .is_store = _du_input->is_store,
        .data_len = _du_input->data_len,
        .rob_index = _du_input->rob_index,

        .addr_ready = false,

        .data_ready = _du_input->data_ready,
        .data_index = _du_input->data_index,
        .data_value = _du_input->data_value,
      });
    }

    // data load reply
    if(_miu_input->is_load_reply) {
      auto &entry = _nxt_regs.entries.at(_cur_regs.load_index);
      // might be forwarded in waiting load reply
      if(!entry.data_ready) {
        debug("LSB: loaded " + std::to_string(_miu_input->value) +
          " at " + std::to_string(entry.addr_value) + " for rob idx "
          + std::to_string(entry.rob_index));
        entry.data_value = _miu_input->value;
        entry.data_ready = true;
        entry.is_executed = true;
        data_output.entry = CDBEntry{
          .is_valid = true,
          .rob_index = entry.rob_index,
          .value = entry.data_value,
        };
        _nxt_regs.load_sent = false;
      }
    }

    if(_cur_regs.accept_addr) {
      auto &entry = _nxt_regs.entries.at(_cur_regs.addr_index);
      entry.addr_ready = true;
      entry.addr_value = _cur_regs.addr_ack;
    }

    if(_cur_regs.accept_data) {
      auto &entry = _nxt_regs.entries.at(_cur_regs.data_index);
      entry.data_ready = true;
      entry.data_value = _cur_regs.data_ack;
    }

    _nxt_regs.accept_addr = false;
    _nxt_regs.accept_data = false;

    // cdb data broadcast (store_data_value/addr_value)
    if(_data_input->entry.is_valid) {
      for(std::size_t i = 0; i < _nxt_regs.entries.size(); ++i) {
        auto index = (_nxt_regs.entries.front_index() + i) % BufSize;
        auto &entry = _nxt_regs.entries.at(index);
        assert(entry.is_valid);
        // store data value (signal: data_index)
        if(entry.is_store && !entry.data_ready && entry.data_index == _data_input->entry.rob_index) {
          _nxt_regs.accept_data = true;
          _nxt_regs.data_ack = _data_input->entry.value;
          _nxt_regs.data_index = index;
        }
        // l/s addr value (signal: rob_index, from_alu)
        // from_alu is used in filtering the signal called by itself.
        if(!entry.addr_ready && entry.rob_index == _data_input->entry.rob_index && _data_input->from_alu) {
          _nxt_regs.accept_addr = true;
          _nxt_regs.addr_ack = _data_input->entry.value;
          _nxt_regs.addr_index = index;
        }
      }
    }

    // rob commission
    if(_rob_input->is_valid) {
      for(std::size_t i = 0; i < _nxt_regs.entries.size(); ++i) {
        auto &entry = _nxt_regs.entries.at((_nxt_regs.entries.front_index() + i) % BufSize);
        if(entry.rob_index == _rob_input->rob_index) {
          entry.is_committed = true;
          if(entry.is_load) {
            entry.is_finished = true;
          }
          break;
        }
      }
    }

    // data forward & execute load
    for(std::size_t i = 0; i < _nxt_regs.entries.size(); ++i) {
      auto &entry = _nxt_regs.entries.at((_nxt_regs.entries.front_index() + i) % BufSize);
      assert(entry.is_valid);
      // data output invalidness: broadcast one data per cycle.
      if(entry.is_load && !entry.data_ready && !data_output.entry.is_valid) {
        bool has_reliance = false;
        for(std::size_t j = i; j > 0; --j) {
          auto& older_entry = _nxt_regs.entries.at((_nxt_regs.entries.front_index() + j - 1) % BufSize);
          if(older_entry.is_valid && older_entry.is_store && older_entry.addr_value == entry.addr_value) {
            has_reliance = true;
            if(older_entry.data_ready && older_entry.data_len == entry.data_len) {
              entry.data_value = older_entry.data_value;
              entry.data_ready = true;
              entry.is_executed = true;
              data_output.entry = CDBEntry{
                .is_valid = true,
                .rob_index = entry.rob_index,
                .value = entry.data_value,
              };
            }
            // only forward the latest one
            break;
          }
        }

        if(!has_reliance && !_cur_regs.load_sent) {
          // load now.
          miu_output = WH_LSB_MIU{
            .is_load_request = true,
            .addr = entry.addr_value,
            .data_len = entry.data_len
          };
          _nxt_regs.load_sent = true;
          _nxt_regs.load_index = (_nxt_regs.entries.front_index() + i) % BufSize;
        }
        // only one at a time
        break;
      }
    }

    // inform ROB
    for(std::size_t i = 0; i < _nxt_regs.entries.size(); ++i) {
      auto &entry = _nxt_regs.entries.at((_nxt_regs.entries.front_index() + i) % BufSize);
      if(entry.is_store && !entry.is_executed && entry.addr_ready && entry.data_ready) {
        entry.is_executed = true;
        rob_output.is_valid = true;
        rob_output.rob_index = entry.rob_index;
        break; // only notify one
      }
    }

    // execute committed store
    if(!_nxt_regs.entries.empty()) {
      auto &entry = _nxt_regs.entries.front();
      if(entry.is_store && entry.is_executed && entry.is_committed) {
        if(!_cur_regs.store_sent) {
          miu_output = WH_LSB_MIU{
            .is_store_request = true,
            .addr = entry.addr_value,
            .value = entry.data_value,
            .data_len = entry.data_len
          };
          // inform MIU
          _nxt_regs.store_sent = true;
          _nxt_regs.store_index = _nxt_regs.entries.front_index();
        }
      }
    }

    if(_miu_input->is_store_reply) {
      auto &entry = _nxt_regs.entries.at(_nxt_regs.store_index);
      entry.is_finished = true;
      _nxt_regs.store_sent = false;
    }

    while(_nxt_regs.entries.size() > 0) {
      auto &entry = _nxt_regs.entries.front();
      if(entry.is_finished) {
        _nxt_regs.entries.pop();
      } else break;
    }

    bool update_signal = false;

    if(*_rob_output != rob_output) {
      *_rob_output = rob_output;
      update_signal = true;
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
  const std::shared_ptr<const WH_FLUSH_PIPELINE> _flush_input;
  const std::shared_ptr<const WH_CDB_OUT> _data_input;

  const std::shared_ptr<WH_LSB_ROB> _rob_output;
  const std::shared_ptr<WH_LSB_MIU> _miu_output;
  const std::shared_ptr<WH_LSB_CDB> _data_output;

  Registers _cur_regs, _nxt_regs;
};

}


#endif // ISM_LOAD_STORE_BUFFER_H