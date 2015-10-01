#include "ltcclock.h"

#include <chrono>
#include <iostream>

#include "log.h"
#include "timer.h"

using namespace std;

namespace Splash {

/*************/
LtcClock::LtcClock(bool masterClock)
{
    registerAttributes();

    _listener = unique_ptr<Listener>(new Listener());
    _listener->setParameters(1, 44100, Listener::SAMPLE_FMT_U8);
    if (!_listener)
    {
        _listener.reset();
        return;
    }

    _masterClock = true;
    _continue = true;

    _ltcThread = thread([&]() {
        LTCDecoder* ltcDecoder = ltc_decoder_create(1920, 32);
        LTCFrameExt ltcFrame;

        vector<char> inputBuffer(512);
        long int total = 0;

        while (_continue)
        {
            if (!_listener->readFromQueue(inputBuffer))
            {
                this_thread::sleep_for(chrono::milliseconds(10));
                continue;
            }

            ltc_decoder_write(ltcDecoder, (ltcsnd_sample_t*)inputBuffer.data(), inputBuffer.size(), total);
            total += inputBuffer.size();

            while (ltc_decoder_read(ltcDecoder, &ltcFrame))
            {
                _ready = true;

                SMPTETimecode stime;
                ltc_frame_to_time(&stime, &ltcFrame.ltc, LTC_TC_CLOCK);

                Clock clock;
                clock.years = stime.years;
                clock.months = stime.months;
                clock.days = stime.days;
                clock.hours = stime.hours;
                clock.mins = stime.mins;
                clock.secs = stime.secs;
                clock.frame = stime.frame;
                _clock = clock;

                if (_masterClock)
                {
                    Values v;
                    getClock(v);
                    Timer::get().setMasterClock(v);
                }
            }
        }

        ltc_decoder_free(ltcDecoder);
    });
}

/*************/
LtcClock::~LtcClock()
{
    _continue = false;
    if (_ltcThread.joinable())
        _ltcThread.join();
}

/*************/
LtcClock::Clock LtcClock::getClock()
{
    return _clock;
}

/*************/
void LtcClock::getClock(Values& clockValues)
{
    if (!_ready)
        return;

    Clock clock = _clock;

    clockValues.clear();
    clockValues.push_back({(int)clock.years});
    clockValues.push_back({(int)clock.months});
    clockValues.push_back({(int)clock.days});
    clockValues.push_back({(int)clock.hours});
    clockValues.push_back({(int)clock.mins});
    clockValues.push_back({(int)clock.secs});
    clockValues.push_back({(int)clock.frame});
}

/*************/
void LtcClock::registerAttributes()
{
}

} // end of namespace
