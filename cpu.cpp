//#pragma once
#include <SDL2/SDL.h>
#include <cstdint>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <time.h>
#include "memory.cpp"

class CPU {
  public:
  Mem ram;
  uint16_t IR;
  uint16_t PC;
  uint8_t DT;      // Delay Timer
  uint8_t ST;      // Sound Timer
  uint16_t I;
  const uint16_t offset = 0x200;
  uint8_t graphics[64*32];
  uint8_t keys[16];

  uint16_t stack[16]; 
  int8_t SP;          // Stack Pointer

  uint8_t GR[16]; // 16 General Registers GR[x] == Vx

  CPU() {
    PC = 0x200;
    set_font();
    SP = -1;
    IR = 0x0;
    I = 0x0;
    DT = 0x0;
    ST = 0x0;
    for(int i = 0; i < 64*32; i++) {
      graphics[i] = 0;
    }
    for(int i = 0; i < 16; i++) {
      stack[i] = 0x0;
    }
    for(int i = 0; i < 16; i++) {
      keys[i] = 0;
    }
    for(int i = 0; i < 16; i++) {
      GR[i] = 0;
    }
  }
  ~CPU() {}
  
  void increment_PC() {
    if(PC < 0xFFF && PC >= 0x200) {
      PC = PC + 2;
    } else {
      PC = 0x200;
    }
  }

 
  void load_ROM(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if(file.is_open()) {
      std::streampos size = file.tellg();
      char *buffer = new char[size];

      file.seekg(0, std::ios::beg);
      file.read(buffer, size);
      file.close();

      for(uint16_t i = 0; i < size; i++) {
        ram.write_byte(offset + i, buffer[i]);
      }

    delete[] buffer;
    }
} 
  
  void push(uint16_t address) {
    if(SP < 15) {
      stack[++SP] = address;
    } else {
      printf("Stack Overflow. SP=%d\n", SP);
    }
  }

  uint16_t pop() {
    if(SP > -1) {
      return stack[SP--];
    } else {
      printf("Stack Underflow.\n");
    }
    return 0x200;
  }
  
  void reset_keys() {
    for(int i = 0; i < 16; i++) {
      keys[i] = 0;
    }
  }

  void fetch() {
    IR = (ram.read_byte(PC) << 8) | ram.read_byte(PC + 1);
    increment_PC();
  }

  void decode_execute() {
    uint16_t opcode = (IR & 0xF000) >> 12, nnn = IR & 0x0FFF, n = IR & 0x000F, 
             x = (IR & 0x0F00) >> 8, y = (IR & 0x00F0) >> 4, kk = IR & 0x00FF;
    uint8_t X, Y, nth_byte, cX, cY; int pixel, coord;
    switch(opcode) {
      case 0x0:
        if(nnn == 0x0E0) { // Clear Display, CLS
          for(int i = 0; i < 64*32; i++) { graphics[i] = 0; } 
        } else if(nnn == 0x0EE) { // Return from subroutine, RET
          PC = pop();
          //printf("Pop operation->SP: %d\n", SP);
        } else {
          printf("Unknown Instruction: %04x\n", IR);
       	  abort();
        }
        break;

      case 0x1: // JP to addr, JP addr
        PC = nnn; 
        break;

      case 0x2: // CALL at addr, CALL addr
        push(PC);
        //printf("Push operation->SP: %d\n", SP);
        //printf("SP(inCPU): %d, IR: %04x\n", SP, IR);
        PC = nnn;
        break;

      case 0x3: // if Vx == kk, skip next instruction, SE Vx, byte
        if(GR[x] == kk) {
          increment_PC();
        }
        break;

      case 0x4: // if Vx != kk, skip next instruction, SNE Vx, byte
        if(GR[x] != kk) {
          increment_PC();
        }
        break;

      case 0x5: // if Vx == Vy, skip next instruction, SE Vx, Vy
        if(n == 0x0 && GR[x] == GR[y]) {
          increment_PC();
        }
        break;

      case 0x6: // load kk into Vx, LD Vx, byte
        GR[x] = kk;
        break;

      case 0x7: // add Vx and kk then store in Vx, ADD Vx, byte
        GR[x] = GR[x] + kk;
        break;

      case 0x8:                   // ALU instructions
        if(n == 0x0) {              
          GR[x] = GR[y];
        } 
        else if(n == 0x1) {       
          GR[x] = GR[x] | GR[y];
        } 
        else if(n == 0x2) {       
          GR[x] = GR[x] & GR[y];
        } 
        else if(n == 0x3) {       
          GR[x] = GR[x] ^ GR[y];
        } 
        else if(n == 0x4) {
          uint16_t sum = GR[x];
          sum = sum + GR[y];
          if(sum > 255) {
            GR[0xF] = 0x1; }
          else {
            GR[0xF] = 0x0; }
          GR[x] = sum & 0x00FF;
        } 
        else if(n == 0x5) {
          if(GR[x] > GR[y]) {
            GR[0xF] = 0x1;
          } else {
            GR[0xF] = 0x0;
          }
          GR[x] = GR[x] - GR[y];
        }
        else if(n == 0x6) {
          if((GR[x] & 0x01) == 0x1) {
            GR[0xF] = 0x1;
          } else {
            GR[0xF] = 0x0;
          }
          GR[x] = GR[x] >> 1;
        }
        else if(n == 0x7) {
          if(GR[y] > GR[x]) {
            GR[0xF] = 0x1;
          } else {
            GR[0xF] = 0x0;
          }
          GR[x] = GR[y] - GR[x];
        }
        else if(n == 0xE) {
          if((GR[x] & 0x80) >> 7 == 0x01) {
            GR[0xF] = 0x1;
          } else {
            GR[0xF] = 0x0;
          }
          GR[x] = GR[x] << 1;
        }
        break;
      
      case 0x9:
        if(n == 0x0 && GR[x] != GR[y]) {
          increment_PC();
        }
        break;

      case 0xA:
        I = nnn;
        printf("nnn: %04x, I: %04x\n", nnn, I);
        break;

      case 0xB:
        PC = nnn + GR[0]; // V0 -> Vx
        break;

      case 0xC:
        srand(time(NULL)); 
        GR[x] = kk & (rand() % 256);
        break;

      case 0xD:
        X = GR[x] % 64, Y = GR[y] % 32; GR[0xF] = 0;
        for(uint16_t row = 0; row < n; row++) {
          nth_byte = ram.read_byte(I + row);
          for(uint16_t col = 0; col < 8; col++) {
            pixel = (nth_byte >> (7 - col)) & 0x01;
            coord = (Y + row) * 64 + (X + col);

            if (pixel == 1) {
                if (coord < 64 * 32) {
                    if (graphics[coord] == 1) {
                        GR[0xF] = 1; // Collision detected
                    }
                    graphics[coord] ^= 1; // XOR to toggle the pixel
                }
            }
          }
        }
        break;

      case 0xE:
        if(kk == 0x9E && keys[GR[x]] == 1) {
          increment_PC();
        }
        else if(kk == 0xA1 && keys[GR[x]] == 0) {
          increment_PC();
        }
        break;

      case 0xF:
        if(kk == 0x07) {
          GR[x] = DT;
        }
        else if(kk == 0x0A) { // In main 
          
        }
        else if(kk == 0x15) {
          DT = GR[x];
        }
        else if(kk == 0x18) {
          ST = GR[x];
        }
        else if(kk == 0x1E) {
          I += GR[x];
        }
        else if(kk == 0x29) {
          if(GR[x] < 16) {
            I = GR[x] * 0x5; // edit here if font change
          } 
        }
        else if(kk == 0x33) {
          ram.write_byte(I, GR[x] / 100);
          ram.write_byte(I + 1, (GR[x] / 10) % 10);
          ram.write_byte(I + 2, GR[x] % 10);
        }
        else if(kk == 0x55) {
          for(int i = 0; i <= x; i++) {
            ram.write_byte(I + i, GR[i]);
          }
        } 
        else if(kk == 0x65) {
          for(int i = 0; i <= x; i++) {
            GR[i] = ram.read_byte(I + i);
          }
        }
        break;
       
       default:
       	printf("Unknown Instruction\n");
       	abort();
    };

    reset_keys();
  }

  void set_font() { // 80 -> 159
    ram.mem[0x000] = 0xF0;
    ram.mem[0x001] = 0x90;
    ram.mem[0x002] = 0x90; // 0
    ram.mem[0x003] = 0x90;
    ram.mem[0x004] = 0xF0;

    ram.mem[0x005] = 0x20;
    ram.mem[0x006] = 0x60;
    ram.mem[0x007] = 0x20; // 1
    ram.mem[0x008] = 0x20;
    ram.mem[0x009] = 0x70;

    ram.mem[0x00A] = 0xF0;
    ram.mem[0x00B] = 0x10;
    ram.mem[0x00C] = 0xF0; // 2
    ram.mem[0x00D] = 0x80;
    ram.mem[0x00E] = 0xF0;

    ram.mem[0x00F] = 0xF0;
    ram.mem[0x010] = 0x10;
    ram.mem[0x011] = 0xF0; // 3
    ram.mem[0x012] = 0x10;
    ram.mem[0x013] = 0xF0;

    ram.mem[0x014] = 0x90;
    ram.mem[0x015] = 0x90;
    ram.mem[0x016] = 0xF0; // 4
    ram.mem[0x017] = 0x10;
    ram.mem[0x018] = 0x10;

    ram.mem[0x019] = 0xF0;
    ram.mem[0x01A] = 0x80;
    ram.mem[0x01B] = 0xF0; // 5
    ram.mem[0x01C] = 0x10;
    ram.mem[0x01D] = 0xF0;

    ram.mem[0x01E] = 0xF0;
    ram.mem[0x01F] = 0x80;
    ram.mem[0x020] = 0xF0; // 6
    ram.mem[0x021] = 0x90;
    ram.mem[0x022] = 0xF0;

    ram.mem[0x023] = 0xF0;
    ram.mem[0x024] = 0x10;
    ram.mem[0x025] = 0x20; // 7
    ram.mem[0x026] = 0x40;
    ram.mem[0x027] = 0x40;

    ram.mem[0x028] = 0xF0;
    ram.mem[0x029] = 0x90;
    ram.mem[0x02A] = 0xF0; // 8
    ram.mem[0x02B] = 0x90;
    ram.mem[0x02C] = 0xF0;

    ram.mem[0x02D] = 0xF0;
    ram.mem[0x02E] = 0x90;
    ram.mem[0x02F] = 0xF0; // 9
    ram.mem[0x030] = 0x10;
    ram.mem[0x031] = 0xF0;

    ram.mem[0x032] = 0xF0;
    ram.mem[0x033] = 0x90;
    ram.mem[0x034] = 0xF0; // A
    ram.mem[0x035] = 0x90;
    ram.mem[0x036] = 0x90;

    ram.mem[0x037] = 0xE0;
    ram.mem[0x038] = 0x90;
    ram.mem[0x039] = 0xE0; // B
    ram.mem[0x03A] = 0x90;
    ram.mem[0x03B] = 0xE0;

    ram.mem[0x03C] = 0xF0;
    ram.mem[0x03D] = 0x80;
    ram.mem[0x03E] = 0x80; // C
    ram.mem[0x03F] = 0x80;
    ram.mem[0x040] = 0xF0;

    ram.mem[0x041] = 0xE0;
    ram.mem[0x042] = 0x90;
    ram.mem[0x043] = 0x90; // D
    ram.mem[0x044] = 0x90;
    ram.mem[0x045] = 0xE0;

    ram.mem[0x046] = 0xF0;
    ram.mem[0x047] = 0x80;
    ram.mem[0x048] = 0xF0; // E
    ram.mem[0x049] = 0x80;
    ram.mem[0x04A] = 0xF0;

    ram.mem[0x04B] = 0xF0;
    ram.mem[0x04C] = 0x80;
    ram.mem[0x04D] = 0xF0; // F
    ram.mem[0x04E] = 0x80;
    ram.mem[0x04F] = 0x80;
  }

};
