#include "APU.hpp"
#include "Emulator.hpp"
#include <Luna/Runtime/Log.hpp>
#include <Luna/Runtime/Math/Math.hpp>
#include "App.hpp"
void APU::tick_div_apu(Emulator* emu)
{
    u8 div = emu->timer.read_div();
    // When DIV bit 4 goes from 1 to 0...
    if(bit_test(&(last_div), 4) && !bit_test(&div, 4))
    {
        // 512Hz.
        ++div_apu;
        if((div_apu % 2) == 0)
        {
            // Length is ticked at 256Hz.
            tick_ch1_length();
            tick_ch2_length();
            tick_ch3_length();
            tick_ch4_length();
        }
        if((div_apu % 4) == 0)
        {
            // Sweep is ticked at 128Hz.
            tick_ch1_sweep();
        }
        if((div_apu % 8) == 0)
        {
            // Envelope is ticked at 64Hz.
            tick_ch1_envelope();
            tick_ch2_envelope();
            tick_ch4_envelope();
        }
    }
    last_div = div;
}
void APU::disable()
{
    // Clears all APU registers.
    memzero(this);
}
void APU::enable_ch1()
{
    bit_set(&nr52_master_control, 0);
    // Save NR1x states to registers.
    ch1_sample_index = 0;
    ch1_volume = ch1_initial_volume();
    ch1_period_counter = ch1_period();
    ch1_sweep_iteration_counter = 0;
    ch1_sweep_iteration_pace = ch1_sweep_pace();
    ch1_envelope_iteration_increase = ch1_envelope_increase();
    ch1_envelope_iteration_pace = ch1_envelope_pace();
    ch1_envelope_iteration_counter = 0;
    ch1_length_timer = ch1_initial_length_timer();
}
void APU::disable_ch1()
{
    bit_reset(&nr52_master_control, 0);
}
void APU::tick_ch1_sweep()
{
    if(ch1_enabled() && ch1_sweep_pace())
    {
        ++ch1_sweep_iteration_counter;
        if(ch1_sweep_iteration_counter == ch1_sweep_iteration_pace)
        {
            // Computes period after modification.
            i32 period = (i32)ch1_period();
            u8 step = ch1_sweep_individual_step();
            if(ch1_sweep_subtraction())
            {
                period -= period / (1 << step);
            }
            else
            {
                period += period / (1 << step);
            }
            // If period is out of valid range after sweep, the channel is 
            // disabled.
            if(period > 0x07FF || period <= 0)
            {
                disable_ch1();
            }
            else
            {
                // Write period back.
                set_ch1_period((u16)period);
            }
            ch1_sweep_iteration_counter = 0;
            // Reload iteration pace when one iteration completes.
            ch1_sweep_iteration_pace = ch1_sweep_pace();
        }
    }
}
void APU::tick_ch1_envelope()
{
    if(ch1_enabled() && ch1_envelope_iteration_pace)
    {
        ++ch1_envelope_iteration_counter;
        if(ch1_envelope_iteration_counter >= ch1_envelope_iteration_pace)
        {
            if(ch1_envelope_iteration_increase)
            {
                if(ch1_volume < 15)
                {
                    ++ch1_volume;
                }
            }
            else
            {
                if(ch1_volume > 0)
                {
                    --ch1_volume;
                }
            }
            ch1_envelope_iteration_counter = 0;
        }
    }
}
void APU::tick_ch1_length()
{
    if(ch1_enabled() && ch1_length_enabled())
    {
        ++ch1_length_timer;
        if(ch1_length_timer >= 64)
        {
            disable_ch1();
        }
    }
}
constexpr u8 pulse_wave_0[8] = {1, 1, 1, 1, 1, 1, 1, 0};
constexpr u8 pulse_wave_1[8] = {0, 1, 1, 1, 1, 1, 1, 0};
constexpr u8 pulse_wave_2[8] = {0, 1, 1, 1, 1, 0, 0, 0};
constexpr u8 pulse_wave_3[8] = {1, 0, 0, 0, 0, 0, 0, 1};
inline f32 dac(u8 sample)
{
    return lerp(-1.0f, 1.0f, ((f32)(15 - sample)) / 15.0f);
}
void APU::tick_ch1(Emulator* emu)
{
    if(!ch1_dac_on())
    {
        disable_ch1();
        return;
    }
    ++ch1_period_counter;
    if(ch1_period_counter >= 0x800)
    {
        // advance to next sample.
        ch1_sample_index = (ch1_sample_index + 1) % 8;
        ch1_period_counter = ch1_period();
    }
    u8 sample = 0;
    switch(ch1_wave_type())
    {
        case 0: sample = pulse_wave_0[ch1_sample_index]; break;
        case 1: sample = pulse_wave_1[ch1_sample_index]; break;
        case 2: sample = pulse_wave_2[ch1_sample_index]; break;
        case 3: sample = pulse_wave_3[ch1_sample_index]; break;
        default: break;
    }
    ch1_output_sample = dac(sample * ch1_volume);
}
void APU::enable_ch2()
{
    bit_set(&nr52_master_control, 1);
    ch2_sample_index = 0;
    ch2_volume = ch2_initial_volume();
    ch2_period_counter = 0;
    ch2_envelope_iteration_increase = ch2_envelope_increase();
    ch2_envelope_iteration_pace = ch2_envelope_pace();
    ch2_envelope_iteration_counter = 0;
    ch2_length_timer = ch2_initial_length_timer();
}
void APU::disable_ch2()
{
    bit_reset(&nr52_master_control, 1);
}
void APU::tick_ch2_envelope()
{
    if(ch2_enabled() && ch2_envelope_iteration_pace)
    {
        ++ch2_envelope_iteration_counter;
        if(ch2_envelope_iteration_counter >= ch2_envelope_iteration_pace)
        {
            if(ch2_envelope_iteration_increase)
            {
                if(ch2_volume < 15)
                {
                    ++ch2_volume;
                }
            }
            else
            {
                if(ch2_volume > 0)
                {
                    --ch2_volume;
                }
            }
            ch2_envelope_iteration_counter = 0;
        }
    }
}
void APU::tick_ch2_length()
{
    if(ch2_enabled() && ch2_length_enabled())
    {
        ++ch2_length_timer;
        if(ch2_length_timer >= 64)
        {
            disable_ch2();
        }
    }
}
void APU::tick_ch2(Emulator* emu)
{
    if(!ch2_dac_on())
    {
        disable_ch2();
        return;
    }
    ++ch2_period_counter;
    if(ch2_period_counter >= 0x800)
    {
        // advance to next sample.
        ch2_sample_index = (ch2_sample_index + 1) % 8;
        ch2_period_counter = ch2_period();
    }
    u8 sample = 0;
    switch(ch2_wave_type())
    {
        case 0: sample = pulse_wave_0[ch2_sample_index]; break;
        case 1: sample = pulse_wave_1[ch2_sample_index]; break;
        case 2: sample = pulse_wave_2[ch2_sample_index]; break;
        case 3: sample = pulse_wave_3[ch2_sample_index]; break;
        default: break;
    }
    ch2_output_sample = dac(sample * ch2_volume);
}
void APU::enable_ch3()
{
    bit_set(&nr52_master_control, 2);
    ch3_period_counter = 0;
    ch3_sample_index = 1;
    ch3_length_timer = ch3_initial_length_timer();
}
void APU::disable_ch3()
{
    bit_reset(&nr52_master_control, 2);
}
u8 APU::ch3_wave_pattern(u8 index) const
{
    luassert(index < 32);
    u8 base = index / 2;
    u8 wave = wave_pattern_ram[base];
    // Read upper then lower.
    wave = (index % 2) ? (wave & 0x0F) : ((wave >> 4) & 0x0F);
    return wave;
}
void APU::tick_ch3_length()
{
    if(ch3_enabled() && ch3_length_enabled())
    {
        ++ch3_length_timer;
        if(ch3_length_timer >= 256)
        {
            disable_ch3();
        }
    }
}
void APU::tick_ch3(Emulator* emu)
{
    if(!ch3_dac_on())
    {
        disable_ch3();
    }
    // The CH3 of APU is ticked 2 times per M-cycle.
    for(u32 i = 0; i < 2; ++i)
    {
        ++ch3_period_counter;
        if(ch3_period_counter >= 0x800)
        {
            // advance to next sample.
            ch3_sample_index = (ch3_sample_index + 1) % 32;
            ch3_period_counter = ch3_period();
        }
    }
    u8 wave = ch3_wave_pattern(ch3_sample_index);
    u8 level = ch3_output_level();
    switch(level)
    {
        case 0: wave = 0; break;
        case 1: break;
        case 2: wave >>= 1; break;
        case 3: wave >>= 2; break;
        default: lupanic(); break;
    }
    ch3_output_sample = dac(wave);
}
void APU::enable_ch4()
{
    bit_set(&nr52_master_control, 3);
    ch4_period_counter = 0;
    ch4_length_timer = ch4_initial_length_timer();
    ch4_volume = ch4_initial_volume();
    ch4_envelope_iteration_increase = bit_test(&nr42_ch4_volume_envelope, 3);
    ch4_envelope_iteration_pace = nr42_ch4_volume_envelope & 0x07;
    ch4_envelope_iteration_counter = 0;
    ch4_lfsr = 0;
}
void APU::disable_ch4()
{
    bit_reset(&nr52_master_control, 3);
}
void APU::update_lfsr(bool short_mode)
{
    u8 b0 = ch4_lfsr & 0x01;
    u8 b1 = (ch4_lfsr & 0x02) >> 1;
    bool v = (b0 == b1);
    ch4_lfsr = (ch4_lfsr & 0x7FFF) + (v ? 0x8000 : 0);
    if(short_mode)
    {
        ch4_lfsr = (ch4_lfsr & 0xFF7F) + (v ? 0x80 : 0);
    }
    ch4_lfsr >>= 1;
}
void APU::tick_ch4_envelope()
{
    if(ch4_enabled() && ch4_envelope_iteration_pace)
    {
        ++ch4_envelope_iteration_counter;
        if(ch4_envelope_iteration_counter >= ch4_envelope_iteration_pace)
        {
            if(ch4_envelope_iteration_increase)
            {
                if(ch4_volume < 15)
                {
                    ++ch4_volume;
                }
            }
            else
            {
                if(ch4_volume > 0)
                {
                    --ch4_volume;
                }
            }
            ch4_envelope_iteration_counter = 0;
        }
    }
}
void APU::tick_ch4_length()
{
    if(ch4_enabled() && ch4_length_enabled())
    {
        ++ch4_length_timer;
        if(ch4_length_timer >= 64)
        {
            disable_ch4();
        }
    }
}
void APU::tick_ch4(Emulator* emu)
{
    ++ch4_period_counter;
    if(ch4_period_counter >= ch4_period())
    {
        // advance to next sample.
        update_lfsr(!!ch4_lfsr_width());
        ch4_period_counter = 0;
    }
    u8 wave = ch4_lfsr & 0x01;
    wave *= ch4_volume;
    ch4_output_sample = dac(wave);
}
void APU::init()
{
    memzero(this);
}
void APU::tick(Emulator* emu)
{
    if(!is_enabled()) return;
    // DIV-APU is ticked at 4194304 Hz.
    tick_div_apu(emu);
    // APU is ticked at 1048576 Hz, trus has 1048576 sample rate.
    if((emu->clock_cycles % 4) == 0)
    {
        // Tick CH1.
        if(ch1_enabled())
        {
            tick_ch1(emu);
        }
        // Tick CH2.
        if(ch2_enabled())
        {
            tick_ch2(emu);
        }
        // Tick CH3.
        if(ch3_enabled())
        {
            tick_ch3(emu);
        }
        // Tick CH4.
        if(ch4_enabled())
        {
            tick_ch4(emu);
        }
        // Mixer.
        // Output volume range in [-4, 4].
        f32 sample_l = 0.0f;
        f32 sample_r = 0.0f;
        if(ch1_l_enabled()) sample_l += ch1_output_sample;
        if(ch1_r_enabled()) sample_r += ch1_output_sample;
        if(ch2_l_enabled()) sample_l += ch2_output_sample;
        if(ch2_r_enabled()) sample_r += ch2_output_sample;
        if(ch3_l_enabled()) sample_l += ch3_output_sample;
        if(ch3_r_enabled()) sample_r += ch3_output_sample;
        if(ch4_l_enabled()) sample_l += ch4_output_sample;
        if(ch4_r_enabled()) sample_r += ch4_output_sample;
        // Volume control.
        // Scale output volume to [-1, 1].
        sample_l /= 4.0f;
        sample_r /= 4.0f;
        sample_l *= ((f32)left_volume()) / 7.0f;
        sample_r *= ((f32)right_volume()) / 7.0f;
        // Write to histroy buffer.
        sample_sum_l -= history_samples_l[history_sample_cursor];
        sample_sum_r -= history_samples_r[history_sample_cursor];
        history_samples_l[history_sample_cursor] = (u16)((sample_l + 1.0f) * 30.0f); // [0, F] * 4.
        history_samples_r[history_sample_cursor] = (u16)((sample_r + 1.0f) * 30.0f);
        sample_sum_l += history_samples_l[history_sample_cursor];
        sample_sum_r += history_samples_r[history_sample_cursor];
        history_sample_cursor = (history_sample_cursor + 1) % 65536;
        // High-pass filter.
        f32 average_level_l = ((((f32)sample_sum_l) / 65536.0f) / 30.0f) - 1.0f;
        f32 average_level_r = ((((f32)sample_sum_r) / 65536.0f) / 30.0f) - 1.0f;
        sample_l -= average_level_l;
        sample_r -= average_level_r;
        // Prevent sample value over [-1, 1] limit.
        sample_l = clamp(sample_l, -1.0f, 1.0f);
        sample_r = clamp(sample_r, -1.0f, 1.0f);
        // Output samples.
        LockGuard guard(g_app->audio_buffer_lock);
        // Restrict audio buffer size to store at most 65536 samples
        // (about 1/16 second of audio data).
        if(g_app->audio_buffer_l.size() >= AUDIO_BUFFER_MAX_SIZE) g_app->audio_buffer_l.pop_front();
        if(g_app->audio_buffer_r.size() >= AUDIO_BUFFER_MAX_SIZE) g_app->audio_buffer_r.pop_front();
        g_app->audio_buffer_l.push_back(sample_l);
        g_app->audio_buffer_r.push_back(sample_r);
    }
}
u8 APU::bus_read(u16 addr)
{
    // CH1 registers.
    if(addr >= 0xFF10 && addr <= 0xFF14)
    {
        if(addr == 0xFF11)
        {
            // lower 6 bits of NR11 is write-only.
            return nr11_ch1_length_timer_duty_cycle & 0xC0;
        }
        if(addr == 0xFF14)
        {
            // only bit 6 is readable.
            return nr14_ch1_period_high_control & 0x40;
        }
        return (&nr10_ch1_sweep)[addr - 0xFF10];
    }
    // CH2 registers.
    if(addr >= 0xFF16 && addr <= 0xFF19)
    {
        if(addr == 0xFF16)
        {
            // lower 6 bits of NR21 is write-only.
            return nr21_ch2_length_timer_duty_cycle & 0xC0;
        }
        if(addr == 0xFF19)
        {
            // only bit 6 is readable.
            return nr24_ch2_period_high_control & 0x40;
        }
        return (&nr21_ch2_length_timer_duty_cycle)[addr - 0xFF16];
    }
    // CH3 registers.
    if(addr >= 0xFF1A && addr <= 0xFF1E)
    {
        if(addr == 0xFF1B)
        {
            // NR31 is write-only.
            return 0;
        }
        if(addr == 0xFF1E)
        {
            // only bit 6 is readable.
            return nr34_ch3_period_high_control & 0x40;
        }
        return (&nr30_ch3_dac_enable)[addr - 0xFF1A];
    }
    // CH4 registers.
    if(addr >= 0xFF20 && addr <= 0xFF23)
    {
        if(addr == 0xFF20)
        {
            // NR41 is write-only.
            return 0;
        }
        if(addr == 0xFF23)
        {
            // only bit 6 is readable.
            return nr44_ch4_control & 0x40;
        }
        return (&nr41_ch4_length_timer)[addr - 0xFF20];
    }
    // Master control registers.
    if(addr >= 0xFF24 && addr <= 0xFF26)
    {
        return (&nr50_master_volume_vin_panning)[addr - 0xFF24];
    }
    // Wave RAM
    if(addr >= 0xFF30 && addr <= 0xFF3F)
    {
        return wave_pattern_ram[addr - 0xFF30];
    }
    log_error("LunaGB", "Unsupported bus read address: 0x%04X", (u32)addr);
    return 0xFF;
}
void APU::bus_write(u16 addr, u8 data)
{
    // CH1 registers.
    if(addr >= 0xFF10 && addr <= 0xFF14)
    {
        if(!is_enabled())
        {
            // Only NRx1 is writable.
            if(addr == 0xFF11)
            {
                nr11_ch1_length_timer_duty_cycle = data;
            }
        }
        else
        {
            if(addr == 0xFF10)
            {
                if((nr10_ch1_sweep & 0x70) == 0 && ((data & 0x70) != 0))
                {
                    // Restart sweep iteration.
                    ch1_sweep_iteration_counter = 0;
                    ch1_sweep_iteration_pace = (data & 0x70) >> 4;
                }
            }
            if(addr == 0xFF14 && bit_test(&data, 7))
            {
                // CH1 trigger.
                enable_ch1();
                data &= 0x7F;
            }
            (&nr10_ch1_sweep)[addr - 0xFF10] = data;
        }
        return;
    }
    // CH2 registers.
    if(addr >= 0xFF16 && addr <= 0xFF19)
    {
        if(!is_enabled())
        {
            // Only NRx1 is writable.
            if(addr == 0xFF16)
            {
                nr21_ch2_length_timer_duty_cycle = data;
            }
        }
        else
        {
            if(addr == 0xFF19 && bit_test(&data, 7))
            {
                // CH2 trigger.
                enable_ch2();
                data &= 0x7F;
            }
            (&nr21_ch2_length_timer_duty_cycle)[addr - 0xFF16] = data;
        }
        return;
    }
    // CH3 registers.
    if(addr >= 0xFF1A && addr <= 0xFF1E)
    {
        if(!is_enabled())
        {
            // Only NRx1 is writable.
            if(addr == 0xFF1B)
            {
                nr31_ch3_length_timer = data;
            }
        }
        else
        {
            if(addr == 0xFF1E && bit_test(&data, 7))
            {
                // CH3 trigger.
                enable_ch3();
                data &= 0x7F;
            }
            (&nr30_ch3_dac_enable)[addr - 0xFF1A] = data;
        }
        return;
    }
    // CH4 registers.
    if(addr >= 0xFF20 && addr <= 0xFF23)
    {
        if(!is_enabled())
        {
            // Only NRx1 is writable.
            if(addr == 0xFF20)
            {
                nr41_ch4_length_timer = data;
            }
        }
        else
        {
            if(addr == 0xFF23 && bit_test(&data, 7))
            {
                // CH4 trigger.
                enable_ch4();
                data &= 0x7F;
            }
            (&nr41_ch4_length_timer)[addr - 0xFF20] = data;
        }
        return;
    }
    // Master control registers.
    if(addr >= 0xFF24 && addr <= 0xFF26)
    {
        if(addr == 0xFF26)
        {
            bool enabled_before = is_enabled();
            // Only bit 7 is writable.
            nr52_master_control = (data & 0x80) | (nr52_master_control & 0x7F);
            if(enabled_before && !is_enabled())
            {
                disable();
            }
            return;
        }
        // All registers except NR52 is read-only if APU is not enabled.
        if(!is_enabled()) return;
        (&nr50_master_volume_vin_panning)[addr - 0xFF24] = data;
        return;
    }
    // Wave RAM
    if(addr >= 0xFF30 && addr <= 0xFF3F)
    {
        wave_pattern_ram[addr - 0xFF30] = data;
        return;
    }
    log_error("LunaGB", "Unsupported bus write address: 0x%04X", (u32)addr);
}