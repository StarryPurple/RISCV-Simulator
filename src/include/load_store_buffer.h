#ifndef ISM_LOAD_STORE_BUFFER_H
#define ISM_LOAD_STORE_BUFFER_H

#include <unordered_map>

#include "common.h"
#include "circular_queue.h"

namespace insomnia {

// Must only be used in insomnia::CPU.
template <std::size_t BufSize>
class LoadStoreBuffer : public CPUModule {
public:
  bool update() override { return false; }
  void sync() override {}
};

}


#endif // ISM_LOAD_STORE_BUFFER_H