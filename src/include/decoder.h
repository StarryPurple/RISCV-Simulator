#ifndef ISM_DECODER_H
#define ISM_DECODER_H

#include "common.h"

namespace insomnia {

class Decoder : public CPUModule {

public:
  bool update() override { return false; }
  void sync() override {}
};

}

#endif // ISM_DECODER_H