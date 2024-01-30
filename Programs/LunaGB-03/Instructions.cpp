#include "Instructions.hpp"
#include "Emulator.hpp"
//! Combines one 16-bit value from two 8-bit values.
inline constexpr u16 make_u16(u8 low, u8 high)
{
    return ((u16)low) | (((u16)high) << 8);
}
//! Reads 16-bit immediate data.
inline u16 read_d16(Emulator* emu)
{
    u16 r = make_u16(emu->bus_read(emu->cpu.pc), emu->bus_read(emu->cpu.pc + 1));
    emu->cpu.pc += 2;
    return r;
}
//! Reads 8-bit immediate data.
inline u8 read_d8(Emulator* emu)
{
    u8 r = emu->bus_read(emu->cpu.pc);
    ++emu->cpu.pc;
    return r;
}
//! Sets the zero flag if `v` is 0, otherwise resets the zero flag.
inline void set_zero_flag(Emulator* emu, u8 v)
{
    if(v)
    {
        emu->cpu.reset_fz();
    }
    else
    {
        emu->cpu.set_fz();
    }
}
//! Compares v1 with v2, and sets flags based on result.
inline void cp_8(Emulator* emu, u16 v1, u16 v2)
{
    u8 r = (u8)v1 - (u8)v2;
    set_zero_flag(emu, r);
    emu->cpu.set_fn();
    if((v1 & 0x0F) < (v2 & 0x0F)) emu->cpu.set_fh();
    else emu->cpu.reset_fh();
    if(v1 < v2) emu->cpu.set_fc();
    else emu->cpu.reset_fc();
}
//! Pushes 16-bit data into stack.
inline void push_16(Emulator* emu, u16 v)
{
    emu->cpu.sp -= 2;
    emu->bus_write(emu->cpu.sp + 1, (u8)((v >> 8) & 0xFF));
    emu->bus_write(emu->cpu.sp, (u8)(v & 0xFF));
}
//! Pops 16-bit data from stack.
inline u16 pop_16(Emulator* emu)
{
    u8 lo = emu->bus_read(emu->cpu.sp);
    u8 hi = emu->bus_read(emu->cpu.sp + 1);
    emu->cpu.sp += 2;
    return make_u16(lo, hi);
}

//! NOP : Do nothing.
void x00_nop(Emulator* emu)
{
    // Do nothing.
    emu->tick(1);
}
//! LD BC, d16 : Loads 16-bit immediate data to BC.
void x01_ld_bc_d16(Emulator* emu)
{
    emu->cpu.bc(read_d16(emu));
    emu->tick(3);
}
//! LD (BC), A : Stores A to the memory pointed by BC.
void x02_ld_mbc_a(Emulator* emu)
{
    emu->bus_write(emu->cpu.bc(), emu->cpu.a);
    emu->tick(2);
}
//! LD B, d8 : Loads 8-bit immediate data to B.
void x06_ld_b_d8(Emulator* emu)
{
    emu->cpu.b = read_d8(emu);
    emu->tick(2);
}
//! LD (a16), SP : Stores SP to the specified address.
void x08_ld_a16_sp(Emulator* emu)
{
    u16 addr = read_d16(emu);
    emu->tick(2);
    emu->bus_write(addr, (u8)(emu->cpu.sp & 0xFF));
    emu->tick(1);
    emu->bus_write(addr + 1, (u8)(emu->cpu.sp >> 8));
    emu->tick(2);
}
//! LD A (BC) : Loads value from memory pointed by BC to A.
void x0a_ld_a_mbc(Emulator* emu)
{
    emu->cpu.a = emu->bus_read(emu->cpu.bc());
    emu->tick(2);
}
//! LD C, d8 : Loads 8-bit immediate data to C.
void x0e_ld_c_d8(Emulator* emu)
{
    emu->cpu.c = read_d8(emu);
    emu->tick(2);
}
//! LD DE, d16 : Loads 16-bit immediate data to DE.
void x11_ld_de_d16(Emulator* emu)
{
    emu->cpu.de(read_d16(emu));
    emu->tick(3);
}
//! LD (DE), A : Stores A to the memory pointed by DE.
void x12_ld_mde_a(Emulator* emu)
{
    emu->bus_write(emu->cpu.de(), emu->cpu.a);
    emu->tick(2);
}
//! LD D, d8 : Loads 8-bit immediate data to D.
void x16_ld_d_d8(Emulator* emu)
{
    emu->cpu.d = read_d8(emu);
    emu->tick(2);
}
//! JR r8 : Jumps to PC + r8.
void x18_jr_r8(Emulator* emu)
{
    i8 offset = (i8)read_d8(emu);
    emu->cpu.pc += (i16)offset;
    emu->tick(3);
}
//! LD A (DE) : Loads value from memory pointed by DE to A.
void x1a_ld_a_mde(Emulator* emu)
{
    emu->cpu.a = emu->bus_read(emu->cpu.de());
    emu->tick(2);
}
//! LD E, d8 : Loads 8-bit immediate data to E.
void x1e_ld_e_d8(Emulator* emu)
{
    emu->cpu.e = read_d8(emu);
    emu->tick(2);
}
//! JR NZ, r8 : Jumps to PC + r8 if Z is 0.
void x20_jr_nz_r8(Emulator* emu)
{
    i8 offset = (i8)read_d8(emu);
    if(!emu->cpu.fz())
    {
        emu->cpu.pc += (i16)offset;
        emu->tick(3);
    }
    else
    {
        emu->tick(2);
    }
}
//! LD HL, d16 : Loads 16-bit immediate data to HL.
void x21_ld_hl_d16(Emulator* emu)
{
    emu->cpu.hl(read_d16(emu));
    emu->tick(3);
}
//! LD (HL+), A : Stores A to the memory pointed by HL, then increases HL.
void x22_ldi_mhl_a(Emulator* emu)
{
    emu->bus_write(emu->cpu.hl(), emu->cpu.a);
    emu->cpu.hl(emu->cpu.hl() + 1);
    emu->tick(2);
}
//! LD H, d8 : Loads 8-bit immediate data to H.
void x26_ld_h_d8(Emulator* emu)
{
    emu->cpu.h = read_d8(emu);
    emu->tick(2);
}
//! JR Z, r8 : Jumps to PC + r8 if Z is 1.
void x28_jr_z_r8(Emulator* emu)
{
    i8 offset = (i8)read_d8(emu);
    if(emu->cpu.fz())
    {
        emu->cpu.pc += (i16)offset;
        emu->tick(3);
    }
    else
    {
        emu->tick(2);
    }
}
//! LD A (HL+) : Loads value from memory pointed by HL to A, then increases HL.
void x2a_ldi_a_mhl(Emulator* emu)
{
    emu->cpu.a = emu->bus_read(emu->cpu.hl());
    emu->cpu.hl(emu->cpu.hl() + 1);
    emu->tick(2);
}
//! LD L, d8 : Loads 8-bit immediate data to L.
void x2e_ld_l_d8(Emulator* emu)
{
    emu->cpu.l = read_d8(emu);
    emu->tick(2);
}
//! JR NC, r8 : Jumps to PC + r8 if C is 0.
void x30_jr_nc_r8(Emulator* emu)
{
    i8 offset = (i8)read_d8(emu);
    if(!emu->cpu.fc())
    {
        emu->cpu.pc += (i16)offset;
        emu->tick(3);
    }
    else
    {
        emu->tick(2);
    }
}
//! LD SP, d16 : Loads 16-bit immediate data to SP.
void x31_ld_sp_d16(Emulator* emu)
{
    emu->cpu.sp = read_d16(emu);
    emu->tick(3);
}
//! LD (HL-) A : Stores A to the memory pointed by HL, then decreases HL.
void x32_ldd_mhl_a(Emulator* emu)
{
    emu->bus_write(emu->cpu.hl(), emu->cpu.a);
    emu->cpu.hl(emu->cpu.hl() - 1);
    emu->tick(2);
}
//! LD (HL), d8 : Stores 8-bit immediate data to memory pointed by HL.
void x36_ld_mhl_d8(Emulator* emu)
{
    u8 data = read_d8(emu);
    emu->tick(1);
    emu->bus_write(emu->cpu.hl(), data);
    emu->tick(2);
}
//! JR C, r8 : Jumps to PC + r8 if C is 1.
void x38_jr_c_r8(Emulator* emu)
{
    i8 offset = (i8)read_d8(emu);
    if(emu->cpu.fc())
    {
        emu->cpu.pc += (i16)offset;
        emu->tick(3);
    }
    else
    {
        emu->tick(2);
    }
}
//! LD A (HL-) : Loads value from memory pointed by HL to A, then decreases HL.
void x3a_ldd_a_mhl(Emulator* emu)
{
    emu->cpu.a = emu->bus_read(emu->cpu.hl());
    emu->cpu.hl(emu->cpu.hl() - 1);
    emu->tick(2);
}
//! LD A, d8 : Loads 8-bit immediate data to A.
void x3e_ld_a_d8(Emulator* emu)
{
    emu->cpu.a = read_d8(emu);
    emu->tick(2);
}
//! LD B, B : Loads B to B.
void x40_ld_b_b(Emulator* emu)
{
    emu->cpu.b = emu->cpu.b;
    emu->tick(1);
}
//! LD B, C : Loads C to B.
void x41_ld_b_c(Emulator* emu)
{
    emu->cpu.b = emu->cpu.c;
    emu->tick(1);
}
//! LD B, D : Loads D to B.
void x42_ld_b_d(Emulator* emu)
{
    emu->cpu.b = emu->cpu.d;
    emu->tick(1);
}
//! LD B, E : Loads E to B.
void x43_ld_b_e(Emulator* emu)
{
    emu->cpu.b = emu->cpu.e;
    emu->tick(1);
}
//! LD B, H : Loads H to B.
void x44_ld_b_h(Emulator* emu)
{
    emu->cpu.b = emu->cpu.h;
    emu->tick(1);
}
//! LD B, L : Loads L to B.
void x45_ld_b_l(Emulator* emu)
{
    emu->cpu.b = emu->cpu.l;
    emu->tick(1);
}
//! LD B, (HL) : Loads 8-bit data pointed by HL to B.
void x46_ld_b_mhl(Emulator* emu)
{
    emu->cpu.b = emu->bus_read(emu->cpu.hl());
    emu->tick(2);
}
//! LD B, A : Loads A to B.
void x47_ld_b_a(Emulator* emu)
{
    emu->cpu.b = emu->cpu.a;
    emu->tick(1);
}
//! LD C, B : Loads B to C.
void x48_ld_c_b(Emulator* emu)
{
    emu->cpu.c = emu->cpu.b;
    emu->tick(1);
}
//! LD C, C : Loads C to C.
void x49_ld_c_c(Emulator* emu)
{
    emu->cpu.c = emu->cpu.c;
    emu->tick(1);
}
//! LD C, D : Loads D to C.
void x4a_ld_c_d(Emulator* emu)
{
    emu->cpu.c = emu->cpu.d;
    emu->tick(1);
}
//! LD C, E : Loads E to C.
void x4b_ld_c_e(Emulator* emu)
{
    emu->cpu.c = emu->cpu.e;
    emu->tick(1);
}
//! LD C, H : Loads H to C.
void x4c_ld_c_h(Emulator* emu)
{
    emu->cpu.c = emu->cpu.h;
    emu->tick(1);
}
//! LD C, L : Loads L to C.
void x4d_ld_c_l(Emulator* emu)
{
    emu->cpu.c = emu->cpu.l;
    emu->tick(1);
}
//! LD C, (HL) : Loads 8-bit data pointed by HL to C.
void x4e_ld_c_mhl(Emulator* emu)
{
    emu->cpu.c = emu->bus_read(emu->cpu.hl());
    emu->tick(2);
}
//! LD C, A : Loads A to C.
void x4f_ld_c_a(Emulator* emu)
{
    emu->cpu.c = emu->cpu.a;
    emu->tick(1);
}
//! LD D, B : Loads B to D.
void x50_ld_d_b(Emulator* emu)
{
    emu->cpu.d = emu->cpu.b;
    emu->tick(1);
}
//! LD D, C : Loads C to D.
void x51_ld_d_c(Emulator* emu)
{
    emu->cpu.d = emu->cpu.c;
    emu->tick(1);
}
//! LD D, D : Loads D to D.
void x52_ld_d_d(Emulator* emu)
{
    emu->cpu.d = emu->cpu.d;
    emu->tick(1);
}
//! LD D, E : Loads E to D.
void x53_ld_d_e(Emulator* emu)
{
    emu->cpu.d = emu->cpu.e;
    emu->tick(1);
}
//! LD D, H : Loads H to D.
void x54_ld_d_h(Emulator* emu)
{
    emu->cpu.d = emu->cpu.h;
    emu->tick(1);
}
//! LD D, L : Loads L to D.
void x55_ld_d_l(Emulator* emu)
{
    emu->cpu.d = emu->cpu.l;
    emu->tick(1);
}
//! LD D, (HL) : Loads 8-bit data pointed by HL to D.
void x56_ld_d_mhl(Emulator* emu)
{
    emu->cpu.d = emu->bus_read(emu->cpu.hl());
    emu->tick(2);
}
//! LD D, A : Loads A to D.
void x57_ld_d_a(Emulator* emu)
{
    emu->cpu.d = emu->cpu.a;
    emu->tick(1);
}
//! LD E, B : Loads B to E.
void x58_ld_e_b(Emulator* emu)
{
    emu->cpu.e = emu->cpu.b;
    emu->tick(1);
}
//! LD E, C : Loads C to E.
void x59_ld_e_c(Emulator* emu)
{
    emu->cpu.e = emu->cpu.c;
    emu->tick(1);
}
//! LD E, D : Loads D to E.
void x5a_ld_e_d(Emulator* emu)
{
    emu->cpu.e = emu->cpu.d;
    emu->tick(1);
}
//! LD E, E : Loads E to E.
void x5b_ld_e_e(Emulator* emu)
{
    emu->cpu.e = emu->cpu.e;
    emu->tick(1);
}
//! LD E, H : Loads H to E.
void x5c_ld_e_h(Emulator* emu)
{
    emu->cpu.e = emu->cpu.h;
    emu->tick(1);
}
//! LD E, L : Loads L to E.
void x5d_ld_e_l(Emulator* emu)
{
    emu->cpu.e = emu->cpu.l;
    emu->tick(1);
}
//! LD E, (HL) : Loads 8-bit data pointed by HL to E.
void x5e_ld_e_mhl(Emulator* emu)
{
    emu->cpu.e = emu->bus_read(emu->cpu.hl());
    emu->tick(2);
}
//! LD E, A : Loads A to E.
void x5f_ld_e_a(Emulator* emu)
{
    emu->cpu.e = emu->cpu.a;
    emu->tick(1);
}
//! LD H, B : Loads B to H.
void x60_ld_h_b(Emulator* emu)
{
    emu->cpu.h = emu->cpu.b;
    emu->tick(1);
}
//! LD H, C : Loads C to H.
void x61_ld_h_c(Emulator* emu)
{
    emu->cpu.h = emu->cpu.c;
    emu->tick(1);
}
//! LD H, D : Loads D to H.
void x62_ld_h_d(Emulator* emu)
{
    emu->cpu.h = emu->cpu.d;
    emu->tick(1);
}
//! LD H, E : Loads E to H.
void x63_ld_h_e(Emulator* emu)
{
    emu->cpu.h = emu->cpu.e;
    emu->tick(1);
}
//! LD H, H : Loads H to H.
void x64_ld_h_h(Emulator* emu)
{
    emu->cpu.h = emu->cpu.h;
    emu->tick(1);
}
//! LD H, L : Loads L to H.
void x65_ld_h_l(Emulator* emu)
{
    emu->cpu.h = emu->cpu.l;
    emu->tick(1);
}
//! LD H, (HL) : Loads 8-bit data pointed by HL to H.
void x66_ld_h_mhl(Emulator* emu)
{
    emu->cpu.h = emu->bus_read(emu->cpu.hl());
    emu->tick(2);
}
//! LD H, A : Loads A to H.
void x67_ld_h_a(Emulator* emu)
{
    emu->cpu.h = emu->cpu.a;
    emu->tick(1);
}
//! LD L, B : Loads B to L.
void x68_ld_l_b(Emulator* emu)
{
    emu->cpu.l = emu->cpu.b;
    emu->tick(1);
}
//! LD L, C : Loads C to L.
void x69_ld_l_c(Emulator* emu)
{
    emu->cpu.l = emu->cpu.c;
    emu->tick(1);
}
//! LD L, D : Loads D to L.
void x6a_ld_l_d(Emulator* emu)
{
    emu->cpu.l = emu->cpu.d;
    emu->tick(1);
}
//! LD L, E : Loads E to L.
void x6b_ld_l_e(Emulator* emu)
{
    emu->cpu.l = emu->cpu.e;
    emu->tick(1);
}
//! LD L, H : Loads H to L.
void x6c_ld_l_h(Emulator* emu)
{
    emu->cpu.l = emu->cpu.h;
    emu->tick(1);
}
//! LD L, L : Loads L to L.
void x6d_ld_l_l(Emulator* emu)
{
    emu->cpu.l = emu->cpu.l;
    emu->tick(1);
}
//! LD L, (HL) : Loads 8-bit data pointed by HL to L.
void x6e_ld_l_mhl(Emulator* emu)
{
    emu->cpu.l = emu->bus_read(emu->cpu.hl());
    emu->tick(2);
}
//! LD L, A : Loads A to L.
void x6f_ld_l_a(Emulator* emu)
{
    emu->cpu.l = emu->cpu.a;
    emu->tick(1);
}
//! LD (HL), B : Stores B to the memory pointed by HL.
void x70_ld_mhl_b(Emulator* emu)
{
    emu->bus_write(emu->cpu.hl(), emu->cpu.b);
    emu->tick(2);
}
//! LD (HL), C : Stores C to the memory pointed by HL.
void x71_ld_mhl_c(Emulator* emu)
{
    emu->bus_write(emu->cpu.hl(), emu->cpu.c);
    emu->tick(2);
}
//! LD (HL), D : Stores D to the memory pointed by HL.
void x72_ld_mhl_d(Emulator* emu)
{
    emu->bus_write(emu->cpu.hl(), emu->cpu.d);
    emu->tick(2);
}
//! LD (HL), E : Stores E to the memory pointed by HL.
void x73_ld_mhl_e(Emulator* emu)
{
    emu->bus_write(emu->cpu.hl(), emu->cpu.e);
    emu->tick(2);
}
//! LD (HL), H : Stores H to the memory pointed by HL.
void x74_ld_mhl_h(Emulator* emu)
{
    emu->bus_write(emu->cpu.hl(), emu->cpu.h);
    emu->tick(2);
}
//! LD (HL), L : Stores L to the memory pointed by HL.
void x75_ld_mhl_l(Emulator* emu)
{
    emu->bus_write(emu->cpu.hl(), emu->cpu.l);
    emu->tick(2);
}
//! LD (HL), A : Stores A to the memory pointed by HL.
void x77_ld_mhl_a(Emulator* emu)
{
    emu->bus_write(emu->cpu.hl(), emu->cpu.a);
    emu->tick(2);
}
//! LD A, B : Loads B to A.
void x78_ld_a_b(Emulator* emu)
{
    emu->cpu.a = emu->cpu.b;
    emu->tick(1);
}
//! LD A, C : Loads C to A.
void x79_ld_a_c(Emulator* emu)
{
    emu->cpu.a = emu->cpu.c;
    emu->tick(1);
}
//! LD A, D : Loads D to A.
void x7a_ld_a_d(Emulator* emu)
{
    emu->cpu.a = emu->cpu.d;
    emu->tick(1);
}
//! LD A, E : Loads E to A.
void x7b_ld_a_e(Emulator* emu)
{
    emu->cpu.a = emu->cpu.e;
    emu->tick(1);
}
//! LD A, H : Loads H to A.
void x7c_ld_a_h(Emulator* emu)
{
    emu->cpu.a = emu->cpu.h;
    emu->tick(1);
}
//! LD A, L : Loads L to A.
void x7d_ld_a_l(Emulator* emu)
{
    emu->cpu.a = emu->cpu.l;
    emu->tick(1);
}
//! LD A, (HL) : Loads 8-bit data pointed by HL to A.
void x7e_ld_a_mhl(Emulator* emu)
{
    emu->cpu.a = emu->bus_read(emu->cpu.hl());
    emu->tick(2);
}
//! LD A, A : Loads A to A.
void x7f_ld_a_a(Emulator* emu)
{
    emu->cpu.a = emu->cpu.a;
    emu->tick(1);
}
//! CP B : Compares B with A.
void xb8_cp_b(Emulator* emu)
{
    cp_8(emu, emu->cpu.a, emu->cpu.b);
    emu->tick(1);
}
//! CP C : Compares C with A.
void xb9_cp_c(Emulator* emu)
{
    cp_8(emu, emu->cpu.a, emu->cpu.c);
    emu->tick(1);
}
//! CP D : Compares D with A.
void xba_cp_d(Emulator* emu)
{
    cp_8(emu, emu->cpu.a, emu->cpu.d);
    emu->tick(1);
}
//! CP E : Compares E with A.
void xbb_cp_e(Emulator* emu)
{
    cp_8(emu, emu->cpu.a, emu->cpu.e);
    emu->tick(1);
}
//! CP H : Compares H with A.
void xbc_cp_h(Emulator* emu)
{
    cp_8(emu, emu->cpu.a, emu->cpu.h);
    emu->tick(1);
}
//! CP L : Compares L with A.
void xbd_cp_l(Emulator* emu)
{
    cp_8(emu, emu->cpu.a, emu->cpu.l);
    emu->tick(1);
}
//! CP (HL) : Compares data pointed by HL with A.
void xbe_cp_mhl(Emulator* emu)
{
    cp_8(emu, emu->cpu.a, emu->bus_read(emu->cpu.hl()));
    emu->tick(2);
}
//! CP A : Compares A with A.
void xbf_cp_a(Emulator* emu)
{
    cp_8(emu, emu->cpu.a, emu->cpu.a);
    emu->tick(1);
}
//! RET NZ : Returns the function if Z is 0.
void xc0_ret_nz(Emulator* emu)
{
    if(!emu->cpu.fz())
    {
        emu->cpu.pc = pop_16(emu);
        emu->tick(5);
    }
    else
    {
        emu->tick(2);
    }
}
//! POP BC : Pops 16-bit data from stack to BC.
void xc1_pop_bc(Emulator* emu)
{
    emu->cpu.bc(pop_16(emu));
    emu->tick(3);
}
//! JP NZ, a16 : Jumps to the 16-bit address if Z is 0.
void xc2_jp_nz_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    if(!emu->cpu.fz())
    {
        emu->cpu.pc = addr;
        emu->tick(4);
    }
    else
    {
        emu->tick(3);
    }
}
//! JP a16 : Jumps to the 16-bit address.
void xc3_jp_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    emu->cpu.pc = addr;
    emu->tick(4);
}
//! CALL NZ, a16 : Calls the function if Z is 0.
void xc4_call_nz_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    emu->tick(2);
    if(!emu->cpu.fz())
    {
        push_16(emu, emu->cpu.pc);
        emu->cpu.pc = addr;
        emu->tick(4);
    }
    else
    {
        emu->tick(1);
    }
}
//! PUSH BC : Pushes BC to the stack.
void xc5_push_bc(Emulator* emu)
{
    push_16(emu, emu->cpu.bc());
    emu->tick(4);
}
//! RST 00H : Pushes PC to stack and resets PC to 0x00.
void xc7_rst_00h(Emulator* emu)
{
    push_16(emu, emu->cpu.pc);
    emu->cpu.pc = 0x0000;
    emu->tick(4);
}
//! RET Z : Returns the function if Z is 1.
void xc8_ret_z(Emulator* emu)
{
    if(emu->cpu.fz())
    {
        emu->cpu.pc = pop_16(emu);
        emu->tick(5);
    }
    else
    {
        emu->tick(2);
    }
}
//! RET : Returns the function.
void xc9_ret(Emulator* emu)
{
    emu->cpu.pc = pop_16(emu);
    emu->tick(4);
}
//! JP Z, a16 : Jumps to the 16-bit address if Z is 1.
void xca_jp_z_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    if(emu->cpu.fz())
    {
        emu->cpu.pc = addr;
        emu->tick(4);
    }
    else
    {
        emu->tick(3);
    }
}
//! CALL Z, a16 : Calls the function if Z is 1.
void xcc_call_z_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    emu->tick(2);
    if(emu->cpu.fz())
    {
        push_16(emu, emu->cpu.pc);
        emu->cpu.pc = addr;
        emu->tick(4);
    }
    else
    {
        emu->tick(1);
    }
}
//! CALL a16 : Calls the function.
void xcd_call_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    emu->tick(2);
    push_16(emu, emu->cpu.pc);
    emu->cpu.pc = addr;
    emu->tick(4);
}
//! RST 08H : Pushes PC to stack and resets PC to 0x08.
void xcf_rst_08h(Emulator* emu)
{
    push_16(emu, emu->cpu.pc);
    emu->cpu.pc = 0x0008;
    emu->tick(4);
}
//! RET NC : Returns the function if C is 0.
void xd0_ret_nc(Emulator* emu)
{
    if(!emu->cpu.fc())
    {
        emu->cpu.pc = pop_16(emu);
        emu->tick(5);
    }
    else
    {
        emu->tick(2);
    }
}
//! POP DE : Pops 16-bit data from stack to DE.
void xd1_pop_de(Emulator* emu)
{
    emu->cpu.de(pop_16(emu));
    emu->tick(3);
}
//! JP NC, a16 : Jumps to the 16-bit address if C is 0.
void xd2_jp_nc_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    if(!emu->cpu.fc())
    {
        emu->cpu.pc = addr;
        emu->tick(4);
    }
    else
    {
        emu->tick(3);
    }
}
//! CALL NC, a16 : Calls the function if C is 0.
void xd4_call_nc_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    emu->tick(2);
    if(!emu->cpu.fc())
    {
        push_16(emu, emu->cpu.pc);
        emu->cpu.pc = addr;
        emu->tick(4);
    }
    else
    {
        emu->tick(1);
    }
}
//! PUSH DE : Pushes DE to the stack.
void xd5_push_de(Emulator* emu)
{
    push_16(emu, emu->cpu.de());
    emu->tick(4);
}
//! RST 10H : Pushes PC to stack and resets PC to 0x10.
void xd7_rst_10h(Emulator* emu)
{
    push_16(emu, emu->cpu.pc);
    emu->cpu.pc = 0x0010;
    emu->tick(4);
}
//! RET C : Returns the function if C is 1.
void xd8_ret_c(Emulator* emu)
{
    if(emu->cpu.fc())
    {
        emu->cpu.pc = pop_16(emu);
        emu->tick(5);
    }
    else
    {
        emu->tick(2);
    }
}
//! RETI : Returns and enables interruption.
void xd9_reti(Emulator* emu)
{
    emu->cpu.enable_interrupt_master();
    xc9_ret(emu);
}
//! JP C, a16 : Jumps to the 16-bit address if C is 1.
void xda_jp_c_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    if(emu->cpu.fc())
    {
        emu->cpu.pc = addr;
        emu->tick(4);
    }
    else
    {
        emu->tick(3);
    }
}
//! CALL C, a16 : Calls the function if C is 1.
void xdc_call_c_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    emu->tick(2);
    if(emu->cpu.fc())
    {
        push_16(emu, emu->cpu.pc);
        emu->cpu.pc = addr;
        emu->tick(4);
    }
    else
    {
        emu->tick(1);
    }
}
//! RST 18H : Pushes PC to stack and resets PC to 0x18.
void xdf_rst_18h(Emulator* emu)
{
    push_16(emu, emu->cpu.pc);
    emu->cpu.pc = 0x0018;
    emu->tick(4);
}
//! LDH (a8) A : Stores A to high memory address (0xFF00 + a8).
void xe0_ldh_m8_a(Emulator* emu)
{
    u8 addr = read_d8(emu);
    emu->tick(1);
    emu->bus_write(0xFF00 + (u16)addr, emu->cpu.a);
    emu->tick(2);
}
//! POP HL : Pops 16-bit data from stack to HL.
void xe1_pop_hl(Emulator* emu)
{
    emu->cpu.hl(pop_16(emu));
    emu->tick(3);
}
//! LD (C), A : Stores A to memory at 0xFF00 + C.
void xe2_ld_mc_a(Emulator* emu)
{
    emu->bus_write(0xFF00 + (u16)emu->cpu.c, emu->cpu.a);
    emu->tick(2);
}
//! PUSH HL : Pushes HL to the stack.
void xe5_push_hl(Emulator* emu)
{
    push_16(emu, emu->cpu.hl());
    emu->tick(4);
}
//! RST 20H : Pushes PC to stack and resets PC to 0x20.
void xe7_rst_20h(Emulator* emu)
{
    push_16(emu, emu->cpu.pc);
    emu->cpu.pc = 0x0020;
    emu->tick(4);
}
//! JP HL : Jumps to HL.
void xe9_jp_hl(Emulator* emu)
{
    emu->cpu.pc = emu->cpu.hl();
    emu->tick(1);
}
//! LD (a16), A : Stores A to the memory address.
void xea_ld_a16_a(Emulator* emu)
{
    u16 addr = read_d16(emu);
    emu->tick(2);
    emu->bus_write(addr, emu->cpu.a);
    emu->tick(2);
}
//! RST 28H : Pushes PC to stack and resets PC to 0x28.
void xef_rst_28h(Emulator* emu)
{
    push_16(emu, emu->cpu.pc);
    emu->cpu.pc = 0x0028;
    emu->tick(4);
}
//! LDH A, (a8) : Loads data in high memory address (0xFF00 + a8) to A.
void xf0_ldh_a_m8(Emulator* emu)
{
    u8 addr = read_d8(emu);
    emu->tick(1);
    emu->cpu.a = emu->bus_read(0xFF00 + (u16)addr);
    emu->tick(2);
}
//! POP AF : Pops 16-bit data from stack to AF.
void xf1_pop_af(Emulator* emu)
{
    emu->cpu.af(pop_16(emu));
    emu->tick(3);
}
//! LD A, (C) : Loads data at memory 0xFF00 + C to A.
void xf2_ld_a_mc(Emulator* emu)
{
    emu->cpu.a = emu->bus_read(0xFF00 + (u16)emu->cpu.c);
    emu->tick(2);
}
//! PUSH AF : Pushes AF to the stack.
void xf5_push_af(Emulator* emu)
{
    push_16(emu, emu->cpu.af());
    emu->tick(4);
}
//! RST 30H : Pushes PC to stack and resets PC to 0x30.
void xf7_rst_30h(Emulator* emu)
{
    push_16(emu, emu->cpu.pc);
    emu->cpu.pc = 0x0030;
    emu->tick(4);
}
//! LD HL, SP + r8 : Loads SP + r8 to HL.
void xf8_ld_hl_sp_r8(Emulator* emu)
{
    emu->cpu.reset_fz();
    emu->cpu.reset_fn();
    u16 v1 = emu->cpu.sp;
    i16 v2 = (i16)((i8)read_d8(emu));
    emu->tick(1);
    u16 r = v1 + v2;
    u16 check = v1 ^ v2 ^ r;
    if(check & 0x10) emu->cpu.set_fh();
    else emu->cpu.reset_fh();
    if(check & 0x100) emu->cpu.set_fc();
    else emu->cpu.reset_fc();
    emu->cpu.hl(r);
    emu->tick(2);
}
//! LD SP, HL : Loads HL to SP.
void xf9_ld_sp_hl(Emulator* emu)
{
    emu->cpu.sp = emu->cpu.hl();
    emu->tick(2);
}
//! LD A, (a16) : Loads data at memory address to A.
void xfa_ld_a_a16(Emulator* emu)
{
    u16 addr = read_d16(emu);
    emu->tick(2);
    emu->cpu.a = emu->bus_read(addr);
    emu->tick(2);
}
//! CP d8 : Compares A with 8-bit immediate data.
void xfe_cp_d8(Emulator* emu)
{
    cp_8(emu, emu->cpu.a, read_d8(emu));
    emu->tick(2);
}
//! RST 38H : Pushes PC to stack and resets PC to 0x38.
void xff_rst_38h(Emulator* emu)
{
    push_16(emu, emu->cpu.pc);
    emu->cpu.pc = 0x0038;
    emu->tick(4);
}
instruction_func_t* instructions_map[256] = 
{
    x00_nop,      x01_ld_bc_d16, x02_ld_mbc_a,  nullptr, nullptr, nullptr, x06_ld_b_d8,   nullptr, x08_ld_a16_sp, nullptr, x0a_ld_a_mbc,  nullptr, nullptr, nullptr, x0e_ld_c_d8, nullptr, 
    nullptr,      x11_ld_de_d16, x12_ld_mde_a,  nullptr, nullptr, nullptr, x16_ld_d_d8,   nullptr, x18_jr_r8,     nullptr, x1a_ld_a_mde,  nullptr, nullptr, nullptr, x1e_ld_e_d8, nullptr, 
    x20_jr_nz_r8, x21_ld_hl_d16, x22_ldi_mhl_a, nullptr, nullptr, nullptr, x26_ld_h_d8,   nullptr, x28_jr_z_r8,   nullptr, x2a_ldi_a_mhl, nullptr, nullptr, nullptr, x2e_ld_l_d8, nullptr, 
    x30_jr_nc_r8, x31_ld_sp_d16, x32_ldd_mhl_a, nullptr, nullptr, nullptr, x36_ld_mhl_d8, nullptr, x38_jr_c_r8,   nullptr, x3a_ldd_a_mhl, nullptr, nullptr, nullptr, x3e_ld_a_d8, nullptr, 
    x40_ld_b_b,   x41_ld_b_c,   x42_ld_b_d,   x43_ld_b_e,   x44_ld_b_h,   x45_ld_b_l,   x46_ld_b_mhl, x47_ld_b_a,   x48_ld_c_b, x49_ld_c_c, x4a_ld_c_d, x4b_ld_c_e, x4c_ld_c_h, x4d_ld_c_l, x4e_ld_c_mhl, x4f_ld_c_a,
    x50_ld_d_b,   x51_ld_d_c,   x52_ld_d_d,   x53_ld_d_e,   x54_ld_d_h,   x55_ld_d_l,   x56_ld_d_mhl, x57_ld_d_a,   x58_ld_e_b, x59_ld_e_c, x5a_ld_e_d, x5b_ld_e_e, x5c_ld_e_h, x5d_ld_e_l, x5e_ld_e_mhl, x5f_ld_e_a,
    x60_ld_h_b,   x61_ld_h_c,   x62_ld_h_d,   x63_ld_h_e,   x64_ld_h_h,   x65_ld_h_l,   x66_ld_h_mhl, x67_ld_h_a,   x68_ld_l_b, x69_ld_l_c, x6a_ld_l_d, x6b_ld_l_e, x6c_ld_l_h, x6d_ld_l_l, x6e_ld_l_mhl, x6f_ld_l_a,
    x70_ld_mhl_b, x71_ld_mhl_c, x72_ld_mhl_d, x73_ld_mhl_e, x74_ld_mhl_h, x75_ld_mhl_l, nullptr,      x77_ld_mhl_a, x78_ld_a_b, x79_ld_a_c, x7a_ld_a_d, x7b_ld_a_e, x7c_ld_a_h, x7d_ld_a_l, x7e_ld_a_mhl, x7f_ld_a_a,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, xb8_cp_b, xb9_cp_c, xba_cp_d, xbb_cp_e, xbc_cp_h, xbd_cp_l, xbe_cp_mhl, xbf_cp_a, 
    xc0_ret_nz,   xc1_pop_bc, xc2_jp_nz_a16, xc3_jp_a16, xc4_call_nz_a16, xc5_push_bc, nullptr, xc7_rst_00h, xc8_ret_z,       xc9_ret,      xca_jp_z_a16, nullptr, xcc_call_z_a16, xcd_call_a16, nullptr,   xcf_rst_08h, 
    xd0_ret_nc,   xd1_pop_de, xd2_jp_nc_a16, nullptr,    xd4_call_nc_a16, xd5_push_de, nullptr, xd7_rst_10h, xd8_ret_c,       xd9_reti,     xda_jp_c_a16, nullptr, xdc_call_c_a16, nullptr,      nullptr,   xdf_rst_18h, 
    xe0_ldh_m8_a, xe1_pop_hl, xe2_ld_mc_a,   nullptr,    nullptr,         xe5_push_hl, nullptr, xe7_rst_20h, nullptr,         xe9_jp_hl,    xea_ld_a16_a, nullptr, nullptr,        nullptr,      nullptr,   xef_rst_28h, 
    xf0_ldh_a_m8, xf1_pop_af, xf2_ld_a_mc,   nullptr,    nullptr,         xf5_push_af, nullptr, xf7_rst_30h, xf8_ld_hl_sp_r8, xf9_ld_sp_hl, xfa_ld_a_a16, nullptr, nullptr,        nullptr,      xfe_cp_d8, xff_rst_38h
};