#include "./sound/ltcclock.h"

#include <chrono>
#include <iostream>

#include "./utils/log.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{

/*************/
LtcClock::LtcClock(bool masterClock, const string& deviceName)
{
    registerAttributes();

    _listener = unique_ptr<Listener>(new Listener());
    _listener->setParameters(1, 0, Sound_Engine::SAMPLE_FMT_U8, deviceName);
    if (!_listener)
    {
        _listener.reset();
        return;
    }

    _masterClock = masterClock;
    _continue = true;

    Log::get() << Log::MESSAGE << "LtcClock::" << __FUNCTION__ << " - Input clock enabled" << Log::endl;

    _ltcThread = thread([&]() {
        LTCDecoder* ltcDecoder = ltc_decoder_create(1920, 32);
        LTCFrameExt ltcFrame;

        vector<uint8_t> inputBuffer(256);
        long int total = 0;

        while (_continue)
        {
            if (!_listener->readFromQueue(inputBuffer))
            {
                this_thread::sleep_for(chrono::milliseconds(5));
                continue;
            }

            // Check all values to check whether the clock is paused or not
            bool paused = true;
            for (auto& v : inputBuffer)
            {
                if (v < 126 || v > 129) // This is for noise handling. There is not enough room for a clock in between.
                {
                    paused = false;
                    break;
                }
            }

            _clock.paused = paused;

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

                // This updates the maximum frames per second, to be able to handle any framerate
                if (stime.frame == 0)
                {
                    // Only accept some specific values
                    if (_previousFrame == 24 || _previousFrame == 25 || _previousFrame == 30 || _previousFrame == 60)
                    {
                        // Small trick to handle errors
                        if (_framerateChanged)
                        {
                            _maximumFramePerSec = _previousFrame + 1;
                            _framerateChanged = false;
                        }
                        else
                        {
                            _framerateChanged = true;
                        }
                    }
                }

                _previousFrame = stime.frame;
                clock.frame = stime.frame * 120 / _maximumFramePerSec;

                _clock = clock;
            }

            if (_masterClock)
            {
                Values v;
                getClock(v);
                Timer::get().setMasterClock(v);
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

    Log::get() << Log::MESSAGE << "LtcClock::" << __FUNCTION__ << " - Input clock disabled" << Log::endl;
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
    clockValues = Values({(int)clock.years, (int)clock.months, (int)clock.days, (int)clock.hours, (int)clock.mins, (int)clock.secs, (int)clock.frame, (int)clock.paused});
}

/*************/
void LtcClock::registerAttributes()
{
}

} // end of namespace
