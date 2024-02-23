#pragma once

struct Emulator;
using instruction_func_t = void(Emulator* emu);

//! A map of all instruction functions by their opcodes.
extern instruction_func_t* instructions_map[256];