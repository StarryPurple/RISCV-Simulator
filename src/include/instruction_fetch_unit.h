#ifndef ISM_INSTRUCTION_FETCH_UNIT_H
#define ISM_INSTRUCTION_FETCH_UNIT_H

#include "common.h"
#include "instruction.h" // pre-decoding
#include "circular_queue.h"

namespace insomnia {

// No delay in communication with RAM Cache
template <std::size_t BufSize>
class InstructionFetchUnit : public CPUModule {
  enum class State {

  };
  struct Registers {
    circular_queue<raw_instr_t, IFUSize> _queue;
    mem_ptr_t _pc; // managed here
  };

private:
};

}

#endif // ISM_INSTRUCTION_FETCH_UNIT_H