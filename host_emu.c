#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define MEM_SIZE 4096 // 4KB virtual memory
#define IVT_SIZE 256*4

uint8_t mem[MEM_SIZE] = {0};

uint16_t ax, bx, cx, dx;
uint16_t sp, bp, si, di;
uint16_t cs, ds, ss, es;
uint16_t ip;
uint16_t flags;

static uint16_t read_imm16(uint16_t addr)
{
    return mem[addr] | (mem[addr + 1] << 8);
}

static void dump_regs(void)
{
    printf("AX=%04X BX=%04X CX=%04X DX=%04X SP=%04X IP=%04X\n", ax, bx, cx, dx, sp, ip);
}

#define PHY_IP ((cs << 4) + ip)

int main() {
    printf("Entered 8086 emulator\n");

    mem[IVT_SIZE] = 0x90; // NOP

    mem[IVT_SIZE+1] = 0xB8; // MOV AX, 0x1234
    mem[IVT_SIZE+2] = 0x34;
    mem[IVT_SIZE+3] = 0x12;

    mem[IVT_SIZE+4] = 0xBB; // MOV BX, 0x5678
    mem[IVT_SIZE+5] = 0x78;
    mem[IVT_SIZE+6] = 0x56;

    mem[IVT_SIZE+7] = 0x40; // INC AX
    mem[IVT_SIZE+8] = 0x4B; // DEC BX

    mem[IVT_SIZE+9] = 0x05; // ADD AX, 0x0002
    mem[IVT_SIZE+10] = 0x02;
    mem[IVT_SIZE+11] = 0x00;

    mem[IVT_SIZE+12] = 0x2D; // SUB AX, 0x0001
    mem[IVT_SIZE+13] = 0x01;
    mem[IVT_SIZE+14] = 0x00;

    mem[IVT_SIZE+15] = 0xBC; // MOV SP, 0x1000
    mem[IVT_SIZE+16] = 0x00;
    mem[IVT_SIZE+17] = 0x10;

    mem[IVT_SIZE+18] = 0x50; // PUSH AX
    mem[IVT_SIZE+19] = 0x58; // POP AX

    mem[IVT_SIZE+20] = 0xEB; // JMP short +2
    mem[IVT_SIZE+21] = 0x02;

    mem[IVT_SIZE+22] = 0x90; // skipped NOP
    mem[IVT_SIZE+23] = 0x90; // skipped NOP

    mem[IVT_SIZE+24] = 0x90; // NOP
    mem[IVT_SIZE+25] = 0xF4; // HLT

    ip = IVT_SIZE;
    printf("Starting execution at IP: 0x%04X\n", ip);
    while (ip + 6 < MEM_SIZE) 
    {
        switch (mem[PHY_IP])
        {
            case 0x90: // NOP
                printf("NOP at CS:IP 0x%04X:0x%04X\n", cs, ip);
                ip++;
                break;
            case 0xF4: // HLT
                sleep(1);
                printf("HLT at CS:IP 0x%04X:0x%04X\n", cs, ip);
                goto fin;
            case 0x40: // INC AX
                ax++;
                printf("INC AX -> 0x%04X\n", ax);
                ip++;
                break;
            case 0x4B: // DEC BX
                bx--;
                printf("DEC BX -> 0x%04X\n", bx);
                ip++;
                break;
            case 0x05: // ADD AX, imm16
                ax += read_imm16(PHY_IP + 1);
                printf("ADD AX -> 0x%04X\n", ax);
                ip += 3;
                break;
            case 0x2D: // SUB AX, imm16
                ax -= read_imm16(PHY_IP + 1);
                printf("SUB AX -> 0x%04X\n", ax);
                ip += 3;
                break;
            case 0xB8: // MOV AX, imm16
                ax = read_imm16(PHY_IP + 1);
                printf("MOV AX, 0x%04X\n", ax);
                ip += 3;
                break;
            case 0xBB: // MOV BX, imm16
                bx = read_imm16(PHY_IP + 1);
                printf("MOV BX, 0x%04X\n", bx);
                ip += 3;
                break;
            case 0xBC: // MOV SP, imm16
                sp = read_imm16(PHY_IP + 1);
                printf("MOV SP, 0x%04X\n", sp);
                ip += 3;
                break;
            case 0x50: // PUSH AX
                sp -= 2;
                mem[(ss << 4) + sp] = ax & 0xFF;
                mem[(ss << 4) + sp + 1] = ax >> 8;
                printf("PUSH AX\n");
                ip++;
                break;
            case 0x58: // POP AX
                ax = mem[(ss << 4) + sp] | (mem[(ss << 4) + sp + 1] << 8);
                sp += 2;
                printf("POP AX -> 0x%04X\n", ax);
                ip++;
                break;
            case 0xEB: // JMP short
                printf("JMP short %d\n", (int8_t)mem[PHY_IP + 1]);
                ip += 2 + (int8_t)mem[PHY_IP + 1];
                break;
            default:
                printf("Unknown opcode: 0x%02X at CS:IP 0x%04X:0x%04X\n", mem[PHY_IP], cs, ip);
                // should we goto exception handler?
                goto fin;
        }
        dump_regs();
    }
fin:
    printf("Quitting emulator\n");
    return 0;
}