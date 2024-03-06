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
    ch1_period_counter = ch1_period();
    ch1_sample_index = 0;
    ch1_length_timer = ch1_initial_length_timer();
    ch1_volume = ch1_initial_volume();
    ch1_envelope_iteration_increase = ch1_envelope_increase();
    ch1_envelope_iteration_pace = ch1_envelope_pace();
    ch1_envelope_iteration_counter = 0;
    ch1_sweep_iteration_counter = 0;
    ch1_sweep_iteration_pace = ch1_sweep_pace();
}
void APU::disable_ch1()
{
    bit_reset(&nr52_master_control, 0);
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
    ch2_period_counter = 0;
    ch2_sample_index = 0;
    ch2_length_timer = ch2_initial_length_timer();
    ch2_volume = ch2_initial_volume();
    ch2_envelope_iteration_increase = ch2_envelope_increase();
    ch2_envelope_iteration_pace = ch2_envelope_pace();
    ch2_envelope_iteration_counter = 0;
}
void APU::disable_ch2()
{
    bit_reset(&nr52_master_control, 1);
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
        // Mixer.
        // Output volume range in [-4, 4].
        f32 sample_l = 0.0f;
        f32 sample_r = 0.0f;
        if(ch1_l_enabled()) sample_l += ch1_output_sample;
        if(ch1_r_enabled()) sample_r += ch1_output_sample;
        if(ch2_l_enabled()) sample_l += ch2_output_sample;
        if(ch2_r_enabled()) sample_r += ch2_output_sample;
        // Volume control.
        // Scale output volume to [-1, 1].
        sample_l /= 4.0f;
        sample_r /= 4.0f;
        sample_l *= ((f32)left_volume()) / 15.0f;
        sample_r *= ((f32)right_volume()) / 15.0f;
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
    // Master control registers.
    if(addr >= 0xFF24 && addr <= 0xFF26)
    {
        return (&nr50_master_volume_vin_panning)[addr - 0xFF24];
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
    // Master control registers.
    if(addr >= 0xFF24 && addr <= 0xFF26)
    {
        if(addr == 0xFF26)
        {
            bool enabled_before = is_enabled();
            // Only bit 7 is writable.
            nr52_master_control = data & 0x80;
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
    log_error("LunaGB", "Unsupported bus write address: 0x%04X", (u32)addr);
}