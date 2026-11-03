#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#define MEM_SIZE 4096 // 4KB virtual memory
#define IVT_SIZE 256*4

#define FLAG_CF 0x0001
#define FLAG_ZF 0x0040
#define FLAG_SF 0x0080
#define FLAG_OF 0x0800
#define FLAG_IF 0x0200

uint8_t mem[MEM_SIZE] = {0};

uint16_t ax, bx, cx, dx;
uint16_t sp, bp, si, di;
uint16_t cs, ds, ss, es;
uint16_t ip;
uint16_t flags;

uint8_t has_interrupt, int_num;

void raise_interrupt(uint8_t num)
{
    if (flags & FLAG_IF)
    {
        has_interrupt = 1;
        int_num = num;
        printf("Interrupt %d raised\n", num);
    }
}

void interrupt_trigger()
{
    printf("Interrupt trigger thread started\n");
    while (1)
    {
        sleep(5);
        raise_interrupt(0x08);
    }
}

static void dump_regs(void)
{
    printf("AX=%04X BX=%04X CX=%04X DX=%04X SP=%04X IP=%04X CF=%d ZF=%d SF=%d OF=%d IF=%d\n",
        ax,
        bx,
        cx,
        dx,
        sp,
        ip,
        (flags & FLAG_CF) ? 1 : 0,
        (flags & FLAG_ZF) ? 1 : 0,
        (flags & FLAG_SF) ? 1 : 0,
        (flags & FLAG_OF) ? 1 : 0,
        (flags & FLAG_IF) ? 1 : 0
    );
}

#define PHY_IP ((cs << 4) + ip)
#define PHY_SP ((ss << 4) + sp)

static uint16_t read_imm16(uint16_t addr)
{
    return mem[addr] | (mem[addr + 1] << 8);
}

static void push16(uint16_t value)
{
    sp -= 2;
    mem[PHY_SP] = value & 0xFF;
    mem[PHY_SP + 1] = value >> 8;
}

static uint16_t pop16(void)
{
    uint16_t value = mem[PHY_SP] | (mem[PHY_SP + 1] << 8);
    sp += 2;
    return value;
}

int main() {
    printf("Entered 8086 emulator\n");
    // create a thread to trigger interrupts
    pthread_t interrupt_thread;
    pthread_create(&interrupt_thread, NULL, (void *(*)(void *))interrupt_trigger, NULL);

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

    mem[IVT_SIZE+24] = 0x31; // XOR AX, AX

    mem[IVT_SIZE+25] = 0xB9; // MOV CX, 0x0003
    mem[IVT_SIZE+26] = 0x03;
    mem[IVT_SIZE+27] = 0x00;

    mem[IVT_SIZE+28] = 0x41; // INC CX
    mem[IVT_SIZE+29] = 0x49; // DEC CX

    mem[IVT_SIZE+30] = 0x92; // XCHG AX, DX

    mem[IVT_SIZE+31] = 0x04; // ADD AL, 0x10
    mem[IVT_SIZE+32] = 0x10;

    mem[IVT_SIZE+33] = 0x3D; // CMP AX, 0x0010
    mem[IVT_SIZE+34] = 0x10;
    mem[IVT_SIZE+35] = 0x00;

    mem[IVT_SIZE+36] = 0x74; // JZ +3
    mem[IVT_SIZE+37] = 0x03;

    mem[IVT_SIZE+38] = 0x90; // skipped NOP
    mem[IVT_SIZE+39] = 0x90; // skipped NOP
    mem[IVT_SIZE+40] = 0x90; // skipped NOP

    mem[IVT_SIZE+41] = 0xE8; // CALL +2
    mem[IVT_SIZE+42] = 0x02;
    mem[IVT_SIZE+43] = 0x00;

    mem[IVT_SIZE+44] = 0x90; // NOP
    mem[IVT_SIZE+45] = 0x90; // NOP
    mem[IVT_SIZE+46] = 0xFA; // CLI
    mem[IVT_SIZE+47] = 0xFB; // STI

    mem[IVT_SIZE+48] = 0xF4; // HLT
    mem[IVT_SIZE+49] = 0x40; // INC AX
    mem[IVT_SIZE+50] = 0xEB; // JMP -4
    mem[IVT_SIZE+51] = -4;

    // interrupt vector 0x08 -> 0000:0200
    mem[0x08 * 4] = 0x00;
    mem[0x08 * 4 + 1] = 0x02;
    mem[0x08 * 4 + 2] = 0x00;
    mem[0x08 * 4 + 3] = 0x00;

    // handler set ax to 0x2333
    mem[0x0200] = 0xB8; // MOV AX, 0x2333
    mem[0x0201] = 0x33;
    mem[0x0202] = 0x23;
    mem[0x0203] = 0xCF; // IRET

    ip = IVT_SIZE;
    printf("Starting execution at IP: 0x%04X\n", ip);
    while (ip + 6 < MEM_SIZE) 
    {
        // check for interrupts first
        if (has_interrupt)
        {
            has_interrupt = 0;
            push16(flags);
            flags &= ~(FLAG_IF);
            push16(cs);
            push16(ip);
            // set CS:IP to interrupt vector
            ip = read_imm16(int_num * 4);
            cs = read_imm16(int_num * 4 + 2);
            printf("Jumping to interrupt handler at CS:IP 0x%04X:0x%04X\n", cs, ip);
            continue;
        }
        switch (mem[PHY_IP])
        {
            case 0x90: // NOP
                printf("NOP at CS:IP 0x%04X:0x%04X\n", cs, ip);
                ip++;
                break;
            case 0xF4: // HLT
                sleep(1);
                printf("HLT at CS:IP 0x%04X:0x%04X\n", cs, ip);
                ip++;
                break;
            case 0x40: // INC AX
                ax++;
                printf("INC AX -> 0x%04X\n", ax);
                ip++;
                break;
            case 0x41: // INC CX
                cx++;
                printf("INC CX -> 0x%04X\n", cx);
                ip++;
                break;
            case 0x49: // DEC CX
                cx--;
                printf("DEC CX -> 0x%04X\n", cx);
                ip++;
                break;
            case 0x4B: // DEC BX
                bx--;
                printf("DEC BX -> 0x%04X\n", bx);
                ip++;
                break;
            case 0x05: // ADD AX, imm16
                ax += read_imm16(PHY_IP + 1);
                flags = (ax == 0) ? (flags | FLAG_ZF) : (flags & ~FLAG_ZF);
                printf("ADD AX -> 0x%04X\n", ax);
                ip += 3;
                break;
            case 0x04: // ADD AL, imm8
                ax = (ax & 0xFF00) | ((ax + mem[PHY_IP + 1]) & 0x00FF);
                printf("ADD AL -> 0x%02X\n", ax & 0xFF);
                ip += 2;
                break;
            case 0x2D: // SUB AX, imm16
                ax -= read_imm16(PHY_IP + 1);
                flags = (ax == 0) ? (flags | FLAG_ZF) : (flags & ~FLAG_ZF);
                printf("SUB AX -> 0x%04X\n", ax);
                ip += 3;
                break;
            case 0x3D: // CMP AX, imm16
            {
                flags &= ~(FLAG_CF | FLAG_ZF | FLAG_SF);
                uint16_t value = read_imm16(PHY_IP + 1);
                if (ax == value)
                    flags |= FLAG_ZF;
                if (ax < value)
                    flags |= FLAG_CF;
                if ((ax - value) & 0x8000)
                    flags |= FLAG_SF;
                printf("CMP AX, 0x%04X\n", value);
                ip += 3;
                break;
            }
            case 0x31: // XOR AX, AX
                ax = 0;
                printf("XOR AX, AX\n");
                ip++;
                break;
            case 0x92: // XCHG AX, DX
            {
                uint16_t tmp = ax;
                ax = dx;
                dx = tmp;
                printf("XCHG AX, DX\n");
                ip++;
                break;
            }
            case 0xB8: // MOV AX, imm16
                ax = read_imm16(PHY_IP + 1);
                printf("MOV AX, 0x%04X\n", ax);
                ip += 3;
                break;
            case 0xB9: // MOV CX, imm16
                cx = read_imm16(PHY_IP + 1);
                printf("MOV CX, 0x%04X\n", cx);
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
                push16(ax);
                printf("PUSH AX\n");
                ip++;
                break;
            case 0x58: // POP AX
                ax = pop16();
                printf("POP AX -> 0x%04X\n", ax);
                ip++;
                break;
            case 0x74: // JZ short
                if (flags & FLAG_ZF)
                {
                    printf("JZ taken\n");
                    ip += 2 + (int8_t)mem[PHY_IP + 1];
                }
                else
                {
                    printf("JZ not taken\n");
                    ip += 2;
                }
                break;
            case 0xC3: // RET
                ip = pop16();
                printf("RET\n");
                break;
            case 0xCF: // IRET
                ip = pop16();
                cs = pop16();
                flags = pop16();
                printf("IRET -> CS:IP 0x%04X:0x%04X\n", cs, ip);
                break;
            case 0xE8: // CALL near
            {
                uint16_t return_ip = ip + 3;

                push16(return_ip);

                ip += 3 + (int16_t)read_imm16(PHY_IP + 1);
                printf("CALL near\n");
                break;
            }
            case 0xFA: // CLI
                flags &= ~FLAG_IF;
                printf("CLI -> IF=0\n");
                ip++;
                break;
            case 0xFB: // STI
                flags |= FLAG_IF;
                printf("STI -> IF=1\n");
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