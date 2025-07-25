#ifndef ISM_PREDICTOR_H
#define ISM_PREDICTOR_H

namespace insomnia {

class Predictor : public CPUModule {
public:
  bool update() override { return false; }
  void sync() override {}
};

}

#endif // ISM_PREDICTOR_H