# chip8-interpreter
My attempt at writing a CHIP-8 interpreter. Following https://tobiasvl.github.io/blog/write-a-chip-8-emulator/ and https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/

All instructions (excluding keyboard input) have been implemented. Not currently using a graphics library, just keeping it simple and drawing to the terminal. CHIP-8 programs that draw stuff to the screen and test instructions that don't rely on keyboard input do work successfully. These programs are [corax89's chip8 test rom](https://github.com/corax89/chip8-test-rom), [cj1128's BC_test](https://github.com/cj1128/chip8-emulator/blob/master/rom/BC_test.ch8), and [loktar00's IBM Logo](https://github.com/loktar00/chip8/blob/master/roms/IBM%20Logo.ch8). 

I may come back and try to implement this fully in the future with a graphics library. 
