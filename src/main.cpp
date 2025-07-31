#include "cpu.h"

int main() {
  insomnia::CPU cpu;
  cpu.preload_program();
  while(true)
    cpu.tick();
  return 0;
}
