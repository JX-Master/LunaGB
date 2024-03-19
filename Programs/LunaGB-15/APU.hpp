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

    // CH3 registers.

    //! 0xFF1A
    u8 nr30_ch3_dac_enable;
    //! 0xFF1B
    u8 nr31_ch3_length_timer;
    //! 0xFF1C
    u8 nr32_ch3_output_level;
    //! 0xFF1D
    u8 nr33_ch3_period_low;
    //! 0xFF1E
    u8 nr34_ch3_period_high_control;

    // CH4 registers.

    //! 0xFF20
    u8 nr41_ch4_length_timer;
    //! 0xFF21
    u8 nr42_ch4_volume_envelope;
    //! 0xFF22
    u8 nr43_ch4_freq_randomness;
    //! 0xFF23
    u8 nr44_ch4_control;

    // Master control registers.

    //! 0xFF24
    u8 nr50_master_volume_vin_panning;
    //! 0xFF25
    u8 nr51_master_panning;
    //! 0xFF26
    u8 nr52_master_control;

    //! 0xFF30-0xFF3F.
    u8 wave_pattern_ram[16];

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
    //! Whether CH3 is enabled.
    bool ch3_enabled() const { return bit_test(&nr52_master_control, 2); }
    //! Whether CH4 is enabled.
    bool ch4_enabled() const { return bit_test(&nr52_master_control, 3); }
    //! Whether CH1 is outputted to right channel.
    bool ch1_r_enabled() const { return bit_test(&nr51_master_panning, 0); }
    //! Whether CH2 is outputted to right channel.
    bool ch2_r_enabled() const { return bit_test(&nr51_master_panning, 1); }
    //! Whether CH3 is outputted to right channel.
    bool ch3_r_enabled() const { return bit_test(&nr51_master_panning, 2); }
    //! Whether CH4 is outputted to right channel.
    bool ch4_r_enabled() const { return bit_test(&nr51_master_panning, 3); }
    //! Whether CH1 is outputted to left channel.
    bool ch1_l_enabled() const { return bit_test(&nr51_master_panning, 4); }
    //! Whether CH2 is outputted to left channel.
    bool ch2_l_enabled() const { return bit_test(&nr51_master_panning, 5); }
    //! Whether CH3 is outputted to left channel.
    bool ch3_l_enabled() const { return bit_test(&nr51_master_panning, 6); }
    //! Whether CH4 is outputted to left channel.
    bool ch4_l_enabled() const { return bit_test(&nr51_master_panning, 7); }
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

    // CH3 states.
    // Audio generation states.
    u8 ch3_sample_index;
    u16 ch3_period_counter;
    f32 ch3_output_sample;
    // Length timer states.
    u8 ch3_length_timer;

    //! Whether CH3 DAC is powered on.
    bool ch3_dac_on() const { return bit_test(&nr30_ch3_dac_enable, 7); }
    void enable_ch3();
    void disable_ch3();
    u8 ch3_initial_length_timer() const { return nr31_ch3_length_timer; }
    u8 ch3_output_level() const { return (nr32_ch3_output_level & 0x60) >> 5; }
    u16 ch3_period() const { return (u16)nr33_ch3_period_low + (((u16)(nr34_ch3_period_high_control & 0x07)) << 8); }
    bool ch3_length_enabled() const { return bit_test(&nr34_ch3_period_high_control, 6); }
    u8 ch3_wave_pattern(u8 index) const;

    void tick_ch3_length();
    void tick_ch3(Emulator* emu);

    // CH4 states.
    // Audio generation states.
    u16 ch4_lfsr;
    u8 ch4_volume;
    u32 ch4_period_counter; // max value is 917504(2^15 * 7 * 4)
    f32 ch4_output_sample;
    // Envelope states.
    bool ch4_envelope_iteration_increase;
    u8 ch4_envelope_iteration_pace;
    u8 ch4_envelope_iteration_counter;
    // Length timer states.
    u8 ch4_length_timer;

    void enable_ch4();
    void disable_ch4();
    u8 ch4_initial_length_timer() const { return nr41_ch4_length_timer & 0x3F; }
    u8 ch4_initial_volume() const { return (nr42_ch4_volume_envelope & 0xF0) >> 4; }
    u32 ch4_period() const { return (u32)ch4_clock_divider() * (1 << ch4_clock_shift()) * 4; }
    bool ch4_length_enabled() const { return bit_test(&nr44_ch4_control, 6); }
    u8 ch4_clock_divider() const { return nr43_ch4_freq_randomness & 0x07; }
    u8 ch4_lfsr_width() const { return (nr43_ch4_freq_randomness & 0x08) >> 3; }
    u8 ch4_clock_shift() const { return (nr43_ch4_freq_randomness & 0xF0) >> 4; }

    void update_lfsr(bool short_mode);
    void tick_ch4_envelope();
    void tick_ch4_length();
    void tick_ch4(Emulator* emu);

    //! Used for high-pass filtering.
    u8 history_samples_l[65536];
    u8 history_samples_r[65536];
    u16 history_sample_cursor;
    u32 sample_sum_l;
    u32 sample_sum_r;

    void init();
    void tick(Emulator* emu);
    u8 bus_read(u16 addr);
    void bus_write(u16 addr, u8 data);
};