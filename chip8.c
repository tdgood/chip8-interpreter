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
    delay_timer = 0;
    sound_timer = 0;

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

void load_program(char *filename) {    
    FILE *fd = fopen(filename, "rb");

    fseek(fd, 0, SEEK_END);
    int filesize = ftell(fd);
    rewind(fd);

    fread((ram + PC_START), sizeof(uint8_t), filesize, fd);

    fclose(fd);
}

void draw_to_terminal() { 
    // Clear terminal screen in Linux
    printf("\033[2J\033[1;1H");
    
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            if (display[i][j] == 1) {
                printf("█ ");
            } else {
                printf("  ");
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
                /*
                 * Opcode 00E0
                 * Clear display
                 */
                case 0x00E0:
                    clear_display();
                    draw_flag = 1;
                    break;

                /*
                 * Opcode 00EE
                 * Return from subroutine
                 */
                case 0x00EE:
                    pc = stack[--sp];
                    break;

                default:
                    printf("Unknown opcode: %#x\n", opcode);
                    break;
            }
            break;

        /*
         * Opcode 1NNN
         * Jump
         */
        case 0x1000:
            pc = nnn;
            break;

        /*
         * Opcode 2NNN
         * Call subroutine
         */
        case 0x2000:
            stack[sp++] = pc;
            pc = nnn;
            break;

        /*
         * Opcode 3XNN
         * Skip if X = NN
         */
        case 0x3000:
            if (v_registers[x_reg] == nn) {
                pc += 2;
            }
            break;

        /*
         * Opcode 4XNN
         * Skip if X != NN
         */
        case 0x4000:
            if (v_registers[x_reg] != nn) {
                pc += 2;
            }
            break;

        /*
         * Opcode 5XY0
         * Skip if X = Y
         */
        case 0x5000:
            if (v_registers[x_reg] == v_registers[y_reg]) {
                pc += 2;
            }
            break;

        /*
         * Opcode 6XNN
         * Set immediate
         */
        case 0x6000:
            v_registers[x_reg] = nn;
            break;

        /*
         * Opcode 7XNN
         * Add without overflow bit logic
         */
        case 0x7000:
            v_registers[x_reg] += nn;
            break;

        case 0x8000:
            switch (opcode & 0x000F) {
                /*
                 * Opcode 8XY0
                 * Set
                 */
                case 0x0000:
                    v_registers[x_reg] = v_registers[y_reg];
                    break;

                /*
                 * Opcode 8XY1
                 * Binary OR
                 */
                case 0x0001:
                    v_registers[x_reg] |= v_registers[y_reg];
                    break;

                /*
                 * Opcode 8XY2
                 * Binary AND
                 */
                case 0x0002:
                    v_registers[x_reg] &= v_registers[y_reg];
                    break;

                /*
                 * Opcode 8XY3
                 * Binary XOR
                 */
                case 0x0003:
                    v_registers[x_reg] ^= v_registers[y_reg];
                    break;

                /*
                 * Opcode 8XY4
                 * Add
                 */
                case 0x0004:
                    if (v_registers[x_reg] > UINT8_MAX - v_registers[y_reg]) {
                        v_registers[0xF] = 1;
                    } else {
                        v_registers[0xF] = 0;
                    }
                    v_registers[x_reg] += v_registers[y_reg];
                    break;

                /*
                 * Opcode 8XY5
                 * Subtract X - Y
                 */
                case 0x0005:
                    if (v_registers[x_reg] > v_registers[y_reg]) {
                        v_registers[0xF] = 1;
                    } else {
                        v_registers[0xF] = 0;
                    }
                    v_registers[x_reg] -= v_registers[y_reg];
                    break;

                /*
                 * Opcode 8XY6
                 * Shift right
                 */
                case 0x0006:
                    if ((v_registers[x_reg] & 0x1) == 1) {
                       v_registers[0xF] = 1; 
                    } else {
                        v_registers[0xF] = 0;
                    }
                    v_registers[x_reg] >>= 1;
                    break;

                /*
                 * Opcode 8XY7
                 * Subtract Y - X
                 */
                case 0x0007:
                    if (v_registers[y_reg] > v_registers[x_reg]) {
                        v_registers[0xF] = 1;
                    } else {
                        v_registers[0xF] = 0;
                    }
                    v_registers[x_reg] = v_registers[y_reg] - v_registers[x_reg];
                    break;

                /*
                 * Opcode 8XYE
                 * Shift left
                 */
                case 0x000E:
                    if ((v_registers[x_reg] & 0x80) == 0x80) {
                        v_registers[0xF] = 1;
                    } else {
                        v_registers[0xF] = 0;
                    }
                    v_registers[x_reg] <<= 1;
                    break;

                default:
                    printf("Unknown opcode: %#x\n", opcode);
                    break;

            }
            break;

        /*
         * Opcode 9XY0
         * Skip if X != Y
         */
        case 0x9000:
            if (v_registers[x_reg] != v_registers[y_reg]) {
                pc += 2;
            }
            break;

        /*
         * Opcode ANNN
         * Set index
         */
        case 0xA000:
            i_reg = nnn;
            break;

        /*
         * Opcode BNNN
         * Jump with offset
         */
        case 0xB000:
            pc = nnn + v_registers[0];
            break;

        /*
         * Opcode CNNN
         * Random number generator
         */
        case 0xC000:
            v_registers[x_reg] = (uint8_t)rand() & nn;
            break;
            
        /* 
         * Opcode DXYN
         * Set display/draw
         */
        case 0xD000:
            {
                uint8_t x_coord = v_registers[x_reg] % 64;
                uint8_t y_coord = v_registers[y_reg] % 32;
                v_registers[0xF] = 0;

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

                draw_flag = 1;
            }
            break;

        /*
         * Opcode EX00
         * Skip if key (not implemented)
         */
        case 0xE000:
            printf("Opcode %#x not implemented\n", opcode);
            break;

        case 0xF000:
            switch (opcode & 0x00FF) {
                /*
                 * Opcode FX07
                 * Set X = delay timer
                 */
                case 0x0007:
                    v_registers[x_reg] = delay_timer;
                    break;

                /*
                 * Opcode FX0A
                 * Get key (not implemented)
                 */
                case 0x000A:
                    printf("Opcode %#x not implemented\n", opcode);
                    break;

                /*
                 * Opcode FX15
                 * Set delay timer = X
                 */
                case 0x0015:
                    delay_timer = v_registers[x_reg];
                    break;

                /*
                 * Opcode FX18
                 * Set sound timer = X
                 */
                case 0x0018:
                    sound_timer = v_registers[x_reg];
                    break;

                /*
                 * Opcode FX1E
                 * Add to index
                 */
                case 0x001E:
                    i_reg += v_registers[x_reg];
                    if (i_reg > 0x0FFF) {
                        v_registers[0xF] = 1;
                    }
                    break;

                /*
                 * Opcode FX29
                 * Get font character
                 */
                case 0x0029:
                    /*
                     * v_registers[x_reg] should get us something in 0-F. Have to multiply by 5 
                     * because font is 5 wide.
                     */
                    i_reg = v_registers[x_reg] * 0x5;
                    break;

                /*
                 * Opcode FX33
                 * Binary-coded decimal conversion
                 */
                case 0x0033:
                    ram[i_reg] = v_registers[x_reg] / 100;
                    ram[i_reg + 1] = (v_registers[x_reg] / 10) % 10;
                    ram[i_reg + 2] = (v_registers[x_reg]) % 10;
                    break;
                
                /*
                 * Opcode FX55
                 * Store memory
                 */
                case 0x0055:
                    for (int i = 0; i <= x_reg; i++) {
                        ram[i_reg + i] = v_registers[i];
                    }
                    break;

                /*
                 * Opcode FX65
                 * Load memory
                 */
                case 0x0065:
                    for (int i = 0; i <= x_reg; i++) {
                        v_registers[i] = ram[i_reg + i];
                    }
                    break;


                default:
                    printf("Unknown opcode: %#x\n", opcode);
                    break;
            }
            break;



        default:
            printf("Unknown opcode: %#x\n", opcode);
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
    load_program(argv[1]);

    while (1) {
        uint16_t opcode = fetch_opcode();
        pc += 2;

        decode_and_execute(opcode);

        if (draw_flag)
            draw_to_terminal();
    }
    
    return 0;
}
