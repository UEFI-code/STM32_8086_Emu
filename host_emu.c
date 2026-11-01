#include <stdio.h>
#include <stdint.h>

#define MEM_SIZE 4096 // 4KB virtual memory
#define IVT_SIZE 256*4

uint8_t mem[MEM_SIZE] = {0};

uint16_t ax, bx, cx, dx;
uint16_t sp, bp, si, di;
uint16_t cs, ds, ss, es;
uint16_t ip;
uint16_t flags;

int main() {
    printf("Entered 8086 emulator\n");
    mem[IVT_SIZE] = 0x90; // NOP instruction at the start of the code memory
    ip = IVT_SIZE; // Set instruction pointer to the start of the code memory
    printf("Starting execution at IP: 0x%04X\n", ip);
    while (ip + 6 < MEM_SIZE) 
    {
        switch (mem[ip])
        {
            case 0x90: // NOP
                printf("NOP executed at IP: 0x%04X\n", ip);
                ip++;
                break;
            default:
                printf("Unknown opcode: 0x%02X at IP: 0x%04X\n", mem[ip], ip);
                // should we goto exception handler?
                goto fin;
        }
    }
    fin:
    printf("Quitting emulator\n");
}