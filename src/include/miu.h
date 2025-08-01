#ifndef ISM_MIU_H
#define ISM_MIU_H

#include "wire_harness.h"

namespace insomnia {

// Maybe someone will implement a RAM here.

// memory is in small endian.
template <std::size_t RAMCap, std::size_t InstrOffset>
class MemoryInterfaceUnit : public CPUModule {
  enum class State {
    IDLE, LSB_LOAD, LSB_STORE, IFU_FETCH
  };
  struct Registers {
    mem_ptr_t addr;
    mem_val_t value;
    mptr_diff_t data_len;
    clock_t clk_delay;
  };
public:
  MemoryInterfaceUnit(
    std::shared_ptr<const WH_LSB_MIU> lsb_input,
    std::shared_ptr<const WH_IFU_MIU> ifu_input,
    std::shared_ptr<WH_MIU_IFU> ifu_output,
    std::shared_ptr<WH_MIU_LSB> lsb_output
    ) :
  _lsb_input(std::move(lsb_input)), _ifu_input(std::move(ifu_input)),
  _ifu_output(std::move(ifu_output)), _lsb_output(std::move(lsb_output)),
  _mem(),
  _cur_stat(State::IDLE), _nxt_stat(State::IDLE),
  _cur_regs(), _nxt_regs() {}

  void sync() override {
    _cur_stat = _nxt_stat;
    _cur_regs = _nxt_regs;
  }
  bool update() override {
    debug("MIU update start");
    _nxt_stat = _cur_stat;
    _nxt_regs = _cur_regs;

    WH_MIU_IFU ifu_output{};
    WH_MIU_LSB lsb_output{};

    switch(_cur_stat) {
    case State::IDLE: {
      try_process();
    } break;
    case State::LSB_LOAD: {
      if(--_nxt_regs.clk_delay == 0) {
        lsb_output.is_valid = true;
        lsb_output.value = read_mem(_cur_regs.addr, _cur_regs.data_len);
        try_process();
      }
    } break;
    case State::LSB_STORE: {
      if(--_nxt_regs.clk_delay == 0) {
        write_mem(_cur_regs.addr, _cur_regs.data_len, _cur_regs.value);
        try_process();
      }
    } break;
    case State::IFU_FETCH: {
      if(--_nxt_regs.clk_delay == 0) {
        ifu_output.is_valid = true;
        ifu_output.instr = static_cast<raw_instr_t>(
          read_mem(_cur_regs.addr + InstrOffset, _cur_regs.data_len));
        ifu_output.instr_addr = _cur_regs.addr;
        try_process();
      }
    } break;
    }

    bool update_signal = false;

    if(*_ifu_output != ifu_output) {
      *_ifu_output = ifu_output;
      update_signal = true;
    }
    if(*_lsb_output != lsb_output) {
      *_lsb_output = lsb_output;
      update_signal = true;
    }
    debug("MIU update end");
    return update_signal;
  }

  // the offset here does not include InstrOffset
  void preload_instruction(raw_instr_t raw_instr, std::size_t offset) {
    static_assert(std::is_same_v<raw_instr_t, mem_val_t>);
    write_mem(InstrOffset + offset, 4, raw_instr);
  }

private:
  const std::shared_ptr<const WH_LSB_MIU> _lsb_input;
  const std::shared_ptr<const WH_IFU_MIU> _ifu_input;
  const std::shared_ptr<WH_MIU_IFU> _ifu_output;
  const std::shared_ptr<WH_MIU_LSB> _lsb_output;
  std::array<uint8_t, RAMCap> _mem;
  State _cur_stat, _nxt_stat;
  Registers _cur_regs, _nxt_regs;

  void try_process() {
    if(_lsb_input->is_load_request && _lsb_input->is_store_request)
      throw std::runtime_error("RAM update: Invalid wire harness");
    _nxt_stat = State::IDLE;
    if(_lsb_input->is_load_request) {
      _nxt_regs.addr = _lsb_input->addr;
      _nxt_regs.data_len = _lsb_input->data_len;
      _nxt_regs.clk_delay = 3;
      _nxt_stat = State::LSB_LOAD;
    } else if(_lsb_input->is_store_request) {
      _nxt_regs.addr = _lsb_input->addr;
      _nxt_regs.data_len = _lsb_input->data_len;
      _nxt_regs.value = _lsb_input->value;
      _nxt_regs.clk_delay = 3;
      _nxt_stat = State::LSB_STORE;
    } else if(_ifu_input->is_valid) {
      _nxt_regs.addr = _ifu_input->pc;
      _nxt_regs.data_len = 4; // fixed
      _nxt_regs.clk_delay = 3;
      _nxt_stat = State::IFU_FETCH;
    }
  }
  mem_val_t read_mem(mem_ptr_t addr, mptr_diff_t data_len) {
    if(addr + data_len > RAMCap)
      throw std::runtime_error("MIU: Read memory access out of RAM bound");
    mem_val_t val = 0;
    for(size_t i = 0; i < data_len; ++i)
      val |= static_cast<mem_val_t>(_mem[addr + i]) << (i * 8);
    return val;
  }
  void write_mem(mem_ptr_t addr, mptr_diff_t data_len, mem_val_t val) {
    if(addr + data_len > RAMCap)
      throw std::runtime_error("MIU: Write memory access out of RAM bound");
    for(size_t i = 0; i < data_len; ++i)
      _mem[addr + i] = static_cast<uint8_t>((val >> (i * 8)) & 0xff);
  }
};

}


#endif // ISM_MIU_H