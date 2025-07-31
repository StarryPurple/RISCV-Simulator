#ifndef ISM_LOAD_STORE_BUFFER_H
#define ISM_LOAD_STORE_BUFFER_H

#include <unordered_map>

#include "common.h"
#include "circular_queue.h"
#include "wire_harness.h"

namespace insomnia {

// Must only be used in insomnia::CPU.
template <std::size_t BufSize>
class LoadStoreBuffer : public CPUModule {
  struct Entry {
    bool is_valid = false;
    bool is_load = false;
    bool is_store = false;
    rob_index_t rob_index;
    bool is_addr_ready = false;
    mem_ptr_t addr;
    bool is_data_ready = false; // load or store
    mem_val_t data; // load or store
    mptr_diff_t data_len;
    bool is_ready; // load: data ready to broadcast. store: finished writing.
  };
  struct Registers {
    std::array<Entry, BufSize> store_buf; // use ROB index as index.
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
    _nxt_regs = _cur_regs;

    WH_LSB_MIU miu_output;
    WH_LSB_CDB data_output;

    bool update_signal = false;
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