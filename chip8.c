#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define RAM_SIZE 4096
#define SCREEN_HEIGHT 32
#define SCREEN_WIDTH 64
#define STACK_SIZE 16
#define NUM_REGISTERS 16
#define FONT_SIZE 80
#define PC_START 0x200
#define FONT_START 0x50

uint8_t ram[RAM_SIZE];
uint8_t display[SCREEN_HEIGHT][SCREEN_WIDTH];
uint16_t pc;
uint16_t i_reg;
uint16_t stack[STACK_SIZE];
uint8_t sp;
uint8_t delay_timer;
uint8_t sound_timer;
uint8_t draw_flag;
uint8_t v_registers[NUM_REGISTERS];
uint8_t font[FONT_SIZE] = {0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0
                           0x20, 0x60, 0x20, 0x20, 0x70,   // 1
                           0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2
                           0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
                           0x90, 0x90, 0xF0, 0x10, 0x10,   // 4
                           0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
                           0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
                           0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
                           0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
                           0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
                           0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
                           0xE0, 0x90, 0xE0, 0x90, 0xE0,   // B
                           0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
                           0xE0, 0x90, 0x90, 0x90, 0xE0,   // D
                           0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
                           0xF0, 0x80, 0xF0, 0x80, 0x80};  // F


void clear_ram() {
    memset(ram, 0, sizeof(ram));
}

void clear_display() {
    memset(display, 0, sizeof(display));
}

void clear_registers() {
    memset(v_registers, 0, sizeof(v_registers));
}

void clear_stack() {
    memset(stack, 0, sizeof(stack));
}

void clear_system() {
    pc = PC_START;
    i_reg = 0;
    sp = 0;
    draw_flag = 0;

    clear_ram();
    clear_display();
    clear_registers();
    clear_stack();
}

void load_font() {
    for (int i = FONT_START; i < FONT_START + FONT_SIZE; i++) {
        ram[i] = font[i - FONT_START];
    }
}

int get_file_size(FILE *fd) {
    int size = 0;

    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    rewind(fd);

    if (size > RAM_SIZE - PC_START) {
        return -1;
    }

    return size;
}

void draw_to_terminal() {
    printf("\033[2J\033[1;1H"); // Clear screen
    
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            if (display[i][j] == 1) {
                printf("X ");
            } else {
                printf(". ");
            }
        }
        printf("\n");
    }
   
    draw_flag = 0;
}

uint16_t fetch_opcode() {
    return ram[pc] << 8 | ram[pc + 1];
}

void decode_and_execute(uint16_t opcode) {
    /*
     * Instructions can show up in 3 forms:
     * INNN - Instruction followed by 12 bit memory address
     * IXNN - Instruction, v_register index, 8 bit immediate
     * IXYN - Instruction, v_register x index, v_register y index, 4 bit number
     */
    
    uint8_t x_reg = (opcode & 0x0F00) >> 8;
    uint8_t y_reg = (opcode & 0x00F0) >> 4;
    uint16_t nnn = (opcode & 0x0FFF);
    uint8_t nn = (opcode & 0x00FF);
    uint8_t n = (opcode & 0x000F);

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0:
                    clear_display();
                    draw_flag = 1;
                    break;

                case 0x00EE:
                    pc = stack[sp--];
                    break;

                default:
                    printf("Unknown opcode\n");
                    break;
            }
            break;

        case 0x1000:
            pc = nnn;
            break;

        case 0x2000:
            stack[sp++] = pc;
            pc = nnn;
            break;

        case 0x3000:
            if (v_registers[x_reg] == nn) {
                pc += 2;
            }
            break;

        case 0x4000:
            if (v_registers[x_reg] != nn) {
                pc += 2;
            }
            break;

        case 0x5000:
            if (v_registers[x_reg] == v_registers[y_reg]) {
                pc += 2;
            }
            break;

        case 0x6000:
            v_registers[x_reg] = nn;
            break;

        case 0x7000:
            v_registers[x_reg] += nn;
            break;

        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000:
                    v_registers[x_reg] = v_registers[y_reg];
                    break;

                case 0x0001:
                    v_registers[x_reg] |= v_registers[y_reg];
                    break;

                case 0x0002:
                    v_registers[x_reg] &= v_registers[y_reg];
                    break;

                case 0x0003:
                    v_registers[x_reg] ^= v_registers[y_reg];
                    break;

                case 0x0004:
                    if (v_registers[x_reg] > UINT8_MAX - v_registers[y_reg]) {
                        v_registers[0xF] = 1;
                    } else {
                        v_registers[0xF] = 0;
                    }
                    v_registers[x_reg] += v_registers[y_reg];
                    break;

                case 0x0005:
                    if (v_registers[x_reg] > v_registers[y_reg]) {
                        v_registers[0xF] = 1;
                    } else {
                        v_registers[0xF] = 0;
                    }
                    v_registers[x_reg] -= v_registers[y_reg];
                    break;

                case 0x0006:
                    if ((v_registers[x_reg] & 0x1) == 1) {
                       v_registers[0xF] = 1; 
                    } else {
                        v_registers[0xF] = 0;
                    }
                    v_registers[x_reg] >>= 1;
                    break;

                case 0x0007:
                    if (v_registers[y_reg] > v_registers[x_reg]) {
                        v_registers[0xF] = 1;
                    } else {
                        v_registers[0xF] = 0;
                    }
                    v_registers[x_reg] = v_registers[y_reg] - v_registers[x_reg];
                    break;

                case 0x000E:
                    if ((v_registers[x_reg] & 0x80) == 0x80) {
                        v_registers[0xF] = 1;
                    } else {
                        v_registers[0xF] = 0;
                    }
                    v_registers[x_reg] <<= 1;
                    break;

                default:
                    printf("Unknown opcode\n");
                    break;

            }
            break;

        case 0x9000:
            if (v_registers[x_reg] != v_registers[y_reg]) {
                pc += 2;
            }
            break;

        case 0xA000:
            i_reg = nnn;
            break;

        case 0xB000:
            pc = nnn + v_registers[0];
            break;

        case 0xC000:
            v_registers[x_reg] = (uint8_t)rand() & nn;
            break;

        case 0xD000:
            {
                uint8_t x_coord = v_registers[x_reg] % 64;
                uint8_t y_coord = v_registers[y_reg] % 32;
                v_registers[0xF] = 0;
                draw_flag = 1;

                for (int i = 0; i < n; i++) {
                    uint8_t curr_sprite_data = ram[i_reg + i];
                    
                    for (int j = 0; j < 8; j++) {
                        uint8_t curr_pixel = (curr_sprite_data & (0x80 >> j)) >> (8 - j - 1);
                        
                        if (curr_pixel == 1 && display[y_coord + i][x_coord + j] == 1) {
                            display[y_coord + i][x_coord + j] = 0;
                            v_registers[0xF] = 1;
                        } else if (curr_pixel == 1 && display[y_coord + i][x_coord + j] == 0) {
                            display[y_coord + i][x_coord + j] = 1;
                        }
                        
                        if (x_coord + j >= SCREEN_WIDTH) {
                            break;
                        }
                    }

                    if (y_coord + i >= SCREEN_HEIGHT) {
                        break;
                    }
                }
            }
            break;

        default:
            printf("Unknown opcode\n");
            break;
    }

}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Incorrect number of arguments. ./chip8 <filename>\n");
        return -1;
    }

    srand(time(NULL));
    clear_system();
    load_font();

    FILE *fd = fopen(argv[1], "rb");

    int file_size = get_file_size(fd);
    if (file_size == -1) {
        printf("File too big\n");
        return -1;
    }

    fread((ram + PC_START), sizeof(uint8_t), file_size, fd);

    fclose(fd);

    while (1) {
        uint16_t opcode = fetch_opcode();
        pc += 2;

        decode_and_execute(opcode);

        if (draw_flag)
            draw_to_terminal();
    }
    
    return 0;
}
