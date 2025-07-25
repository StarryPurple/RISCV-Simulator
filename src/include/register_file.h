#ifndef ISM_REGISTER_FILE_H
#define ISM_REGISTER_FILE_H

namespace insomnia {

class RegisterFile : public CPUModule {
public:
  bool update() override { return false; }
  void sync() override {}
};

}

#endif // ISM_REGISTER_FILE_H