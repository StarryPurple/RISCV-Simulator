#ifndef ISM_REORDER_BUFFER_H
#define ISM_REORDER_BUFFER_H

#include "circular_queue.h"
#include "common.h"

namespace insomnia {

template <std::size_t BufSize>
class ReorderBuffer : public CPUModule {
public:
  bool update() override { return false; }
  void sync() override {}
};


}

#endif // ISM_REORDER_BUFFER_H