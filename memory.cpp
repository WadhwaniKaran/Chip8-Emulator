//#pragma once

#include <cstdint>
#include <stdio.h>

class Mem {
  public:
  uint8_t mem[4096];
  
  void init_mem() {
    for(int i = 0; i < 4096; i++) {
      mem[i] = 0x00;
    }
  }

  void print_mem() {
    for(int i = 0; i < 4096; i++) {
      printf("Address: %03x, Value: %02x | ", i, mem[i]);
    }
    printf("\n");
  }

  uint8_t read_byte(uint16_t address) {
    return mem[address];
  }

  void write_byte(uint16_t address, unsigned char value) {
    mem[address] = value;
  }

  Mem() {
    init_mem();
  }
  ~Mem() {}
};
