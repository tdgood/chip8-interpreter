#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define RAM_SIZE 4096
#define SCREEN_HEIGHT 32
#define SCREEN_WIDTH 64
#define STACK_SIZE 16
#define NUM_REGISTERS 16
#define FONT_SIZE 80

uint8_t ram[RAM_SIZE];
uint8_t display[SCREEN_HEIGHT][SCREEN_WIDTH];
uint8_t pc = 0;
uint16_t index_register = 0;
uint16_t stack[STACK_SIZE];
uint8_t sp = 0;
uint8_t delay_timer = 0;
uint8_t sound_timer = 0;
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

void clear_system() {
    clear_ram();
    clear_display();
    clear_registers();
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

        case 0x9000:
            if (v_registers[x_reg] != v_registers[y_reg]) {
                pc += 2;
            }
            break;

        default:
            printf("Unknown opcode\n");
            break;
    }

}

int main() {
    while (1) {
        uint16_t opcode = fetch_opcode();
        pc += 2;

        decode_and_execute(opcode);
    }
    
    return 0;
}
