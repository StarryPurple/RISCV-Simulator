#ifndef ISM_REGISTER_FILE_H
#define ISM_REGISTER_FILE_H

namespace insomnia {

class RegisterFile : public CPUModule {
  enum class State {

  };
  struct Registers {

  };
public:
  bool update() override { return false; }
  void sync() override {}

private:
};

}

#endif // ISM_REGISTER_FILE_H