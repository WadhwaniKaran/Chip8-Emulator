#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <cstdint>
#include <iostream>
#include "cpu.cpp"

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 512;
const int PIXEL_SIZE = 16;
const int CHIP8_WIDTH = 64;
const int CHIP8_HEIGHT = 32;

void render(SDL_Renderer* renderer, uint8_t graphics[CHIP8_WIDTH * CHIP8_HEIGHT]);
void key_presses(SDL_Event e, CPU *chip);
void inst_fx0a(SDL_Event e, CPU *chip);
void beep();

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }
    
    CPU chip8;
    chip8.load_ROM(argv[1]);
    render(renderer, chip8.graphics);
    
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if(e.type == SDL_KEYDOWN) {
              key_presses(e, &chip8);
            }
        }
      
        chip8.fetch();
        chip8.decode_execute();
        printf("IR: %04x\n", chip8.IR);
        if((chip8.IR & 0xF0FF) == 0xF00A) {
          inst_fx0a(e, &chip8);
        }

        if(((chip8.IR) >> 12) == 0xD) {
          render(renderer, chip8.graphics);
        }
        if(chip8.DT > 0) { chip8.DT--; }
        SDL_Delay(48);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_CloseAudio();
    SDL_Quit();

    return 0;
}

void render(SDL_Renderer* renderer, uint8_t graphics[CHIP8_WIDTH * CHIP8_HEIGHT]) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int y = 0; y < CHIP8_HEIGHT; ++y) {
        for (int x = 0; x < CHIP8_WIDTH; ++x) {
            if (graphics[y * CHIP8_WIDTH + x] == 1) {
                SDL_Rect pixelRect = {x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE};
                SDL_RenderFillRect(renderer, &pixelRect);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void key_presses(SDL_Event e, CPU *chip) {
    switch(e.key.keysym.scancode) {
      case SDL_SCANCODE_X:
        chip->keys[0x0] = 1;
        break;

      case SDL_SCANCODE_1:
        chip->keys[0x1] = 1;
        break;

      case SDL_SCANCODE_2:
        chip->keys[0x2] = 1;
        break;

      case SDL_SCANCODE_3:
        chip->keys[0x3] = 1;
        break;

      case SDL_SCANCODE_Q:
        chip->keys[0x4] = 1;
        break;

      case SDL_SCANCODE_W:
        chip->keys[0x5] = 1;
        break;

      case SDL_SCANCODE_E:
        chip->keys[0x6] = 1;
        break;

      case SDL_SCANCODE_A:
        chip->keys[0x7] = 1;
        break;

      case SDL_SCANCODE_S:
        chip->keys[0x8] = 1;
        break;

      case SDL_SCANCODE_D:
        chip->keys[0x9] = 1;
        break;

      case SDL_SCANCODE_Z:
        chip->keys[0xA] = 1;
        break;

      case SDL_SCANCODE_C:
        chip->keys[0xB] = 1;
        break;

      case SDL_SCANCODE_4:
        chip->keys[0xC] = 1;
        break;

      case SDL_SCANCODE_R:
        chip->keys[0xD] = 1;
        break;

      case SDL_SCANCODE_F:
        chip->keys[0xE] = 1;
        break;

      case SDL_SCANCODE_V:
        chip->keys[0xF] = 1;
        break;

      default:
        NULL;
    }
}

void inst_fx0a(SDL_Event e, CPU *chip) {
  bool key_pressed = false;
  while(!key_pressed) {
    SDL_PollEvent(&e);
    if(e.type == SDL_QUIT) {
      SDL_Quit();
      exit(0);
    }
    else if(e.type == SDL_KEYDOWN) {
      chip->reset_keys();
      key_presses(e, chip);
      for(int i = 0; i < 16; i++) {
        if(chip->keys[i] == 1) {
          chip->GR[(chip->IR & 0x0F00) >> 8] = i;
          key_pressed = true;
          printf("Key pressed: %d\n", i);
          return;
        }
      }
    }
  }
}
