#pragma once
#include <Luna/Runtime/MemoryUtils.hpp>
using namespace Luna;

struct Emulator;
struct APU
{
    // Registers.

    // CH1 registers.

    //! 0xFF10
    u8 nr10_ch1_sweep;
    //! 0xFF11
    u8 nr11_ch1_length_timer_duty_cycle;
    //! 0xFF12
    u8 nr12_ch1_volume_envelope;
    //! 0xFF13
    u8 nr13_ch1_period_low;
    //! 0xFF14
    u8 nr14_ch1_period_high_control;

    // CH2 registers.

    //! 0xFF16
    u8 nr21_ch2_length_timer_duty_cycle;
    //! 0xFF17
    u8 nr22_ch2_volume_envelope;
    //! 0xFF18
    u8 nr23_ch2_period_low;
    //! 0xFF19
    u8 nr24_ch2_period_high_control;

    // Master control registers.

    //! 0xFF24
    u8 nr50_master_volume_vin_panning;
    //! 0xFF25
    u8 nr51_master_panning;
    //! 0xFF26
    u8 nr52_master_control;

    // APU internal state.
    
    // Stores the timer DIV value in last tick to detect div value change.
    u8 last_div;
    // The DIV-APU counter, increases every time DIV’s bit 4 goes from 1 to 0.
    u8 div_apu;
    void tick_div_apu(Emulator* emu);

    // Master control states.

    //! Whether APU is enabled.
    bool is_enabled() const { return bit_test(&nr52_master_control, 7); }
    //! Disables APU.
    void disable();
    //! Whether CH1 is enabled.
    bool ch1_enabled() const { return bit_test(&nr52_master_control, 0); }
    //! Whether CH2 is enabled.
    bool ch2_enabled() const { return bit_test(&nr52_master_control, 1); }
    //! Whether CH1 is outputted to right channel.
    bool ch1_r_enabled() const { return bit_test(&nr51_master_panning, 0); }
    //! Whether CH2 is outputted to right channel.
    bool ch2_r_enabled() const { return bit_test(&nr51_master_panning, 1); }
    //! Whether CH1 is outputted to left channel.
    bool ch1_l_enabled() const { return bit_test(&nr51_master_panning, 4); }
    //! Whether CH2 is outputted to left channel.
    bool ch2_l_enabled() const { return bit_test(&nr51_master_panning, 5); }
    //! Right channel master volume (0~15).
    u8 right_volume() const { return nr50_master_volume_vin_panning & 0x07; }
    //! Left channel master volume (0~15).
    u8 left_volume() const { return (nr50_master_volume_vin_panning & 0x70) >> 4; }

    // CH1 states.
    // Audio generation states.
    u8 ch1_sample_index;
    u8 ch1_volume;
    u16 ch1_period_counter;
    f32 ch1_output_sample;
    // Sweep states.
    u8 ch1_sweep_iteration_counter;
    u8 ch1_sweep_iteration_pace;
    // Envelope states.
    bool ch1_envelope_iteration_increase;
    u8 ch1_envelope_iteration_pace;
    u8 ch1_envelope_iteration_counter;
    // Length timer states.
    u8 ch1_length_timer;

    //! Whether CH1 DAC is powered on.
    bool ch1_dac_on() const { return (nr12_ch1_volume_envelope & 0xF8) != 0; }
    void enable_ch1();
    void disable_ch1();
    u8 ch1_sweep_pace() const { return (nr10_ch1_sweep & 0x70) >> 4; }
    bool ch1_sweep_subtraction() const { return bit_test(&nr10_ch1_sweep, 3); }
    u8 ch1_sweep_individual_step() const { return nr10_ch1_sweep & 0x07; }
    u8 ch1_initial_length_timer() const { return (nr11_ch1_length_timer_duty_cycle & 0x3F); }
    u8 ch1_wave_type() const { return (nr11_ch1_length_timer_duty_cycle & 0xC0) >> 6; }
    u8 ch1_envelope_pace() const { return nr12_ch1_volume_envelope & 0x07; }
    bool ch1_envelope_increase() const { return bit_test(&nr12_ch1_volume_envelope, 3); }
    u8 ch1_initial_volume() const { return (nr12_ch1_volume_envelope & 0xF0) >> 4; }
    u16 ch1_period() const { return (u16)nr13_ch1_period_low + (((u16)(nr14_ch1_period_high_control & 0x07)) << 8); }
    void set_ch1_period(u16 period)
    {
        nr13_ch1_period_low = (u8)(period & 0xFF);
        nr14_ch1_period_high_control = (nr14_ch1_period_high_control & 0xF8) + (u8)((period >> 8) & 0x07);
    }
    bool ch1_length_enabled() const { return bit_test(&nr14_ch1_period_high_control, 6); }

    void tick_ch1_sweep();
    void tick_ch1_envelope();
    void tick_ch1_length();
    void tick_ch1(Emulator* emu);

    // CH2 states.
    // Audio generation states.
    u8 ch2_sample_index;
    u8 ch2_volume;
    u16 ch2_period_counter;
    f32 ch2_output_sample;
    // Envelope states.
    bool ch2_envelope_iteration_increase;
    u8 ch2_envelope_iteration_pace;
    u8 ch2_envelope_iteration_counter;
    // Length timer states.
    u8 ch2_length_timer;

    //! Whether CH2 DAC is powered on.
    bool ch2_dac_on() const { return (nr22_ch2_volume_envelope & 0xF8) != 0; }
    void enable_ch2();
    void disable_ch2();
    u8 ch2_initial_length_timer() const { return (nr21_ch2_length_timer_duty_cycle & 0x3F); }
    u8 ch2_wave_type() const { return (nr21_ch2_length_timer_duty_cycle & 0xC0) >> 6; }
    u8 ch2_envelope_pace() const { return nr22_ch2_volume_envelope & 0x07; }
    bool ch2_envelope_increase() const { return bit_test(&nr22_ch2_volume_envelope, 3); }
    u8 ch2_initial_volume() const { return (nr22_ch2_volume_envelope & 0xF0) >> 4; }
    u16 ch2_period() const { return (u16)nr23_ch2_period_low + (((u16)(nr24_ch2_period_high_control & 0x07)) << 8); }
    bool ch2_length_enabled() const { return bit_test(&nr24_ch2_period_high_control, 6); }

    void tick_ch2_envelope();
    void tick_ch2_length();
    void tick_ch2(Emulator* emu);

    void init();
    void tick(Emulator* emu);
    u8 bus_read(u16 addr);
    void bus_write(u16 addr, u8 data);
};