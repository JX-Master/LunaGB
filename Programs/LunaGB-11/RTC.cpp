#include "RTC.hpp"
#include <Luna/Runtime/Time.hpp>

void RTC::init()
{
    memzero(this);
    last_timestamp = get_utc_timestamp();
}
void RTC::update()
{
    if(!halted())
    {
        i64 current_timestamp = get_utc_timestamp();
        timestamp += current_timestamp - last_timestamp;
        last_timestamp = current_timestamp;
        if(!time_latched)
        {
            update_time_registers();
        }
    }
}
void RTC::update_time_registers()
{
    s = timestamp % 60;
    m = (timestamp / 60) % 60;
    h = (timestamp / 3600) % 24;
    u16 days = (u16)(timestamp / 86400);
    dl = (u8)(days & 0xFF);
    if(days & 0x100) bit_set(&dh, 0);
    else bit_reset(&dh, 0);
    if(days >= 512) bit_set(&dh, 7);
    else bit_reset(&dh, 7);
}
void RTC::update_timestamp()
{
    timestamp = s + ((u64)m) * 60 + ((u64)h) * 3600 + ((u64)days()) * 86400;
    if(day_overflow())
    {
        timestamp += 86400 * 512;
    }
}
void RTC::latch()
{
    if(!time_latched)
    {
        time_latched = true;
    }
    else
    {
        time_latched = false;
        update_time_registers();
    }
}
void RTC::resume()
{
    last_timestamp = get_utc_timestamp();
}