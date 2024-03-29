#include "./sound/ltcclock.h"

#include <chrono>
#include <iostream>

#include "./utils/log.h"
#include "./utils/timer.h"

namespace chrono = std::chrono;

namespace Splash
{

/*************/
LtcClock::LtcClock(bool masterClock, const std::string& deviceName)
    : GraphObject(nullptr)
{
    registerAttributes();

    _listener = std::make_unique<Listener>();
    _listener->setParameters(1, 0, Sound_Engine::SAMPLE_FMT_U8, deviceName);
    if (!_listener)
    {
        _listener.reset();
        return;
    }

    _masterClock = masterClock;
    _continue = true;

    Log::get() << Log::MESSAGE << "LtcClock::" << __FUNCTION__ << " - Input clock enabled" << Log::endl;

    _ltcThread = std::thread([&]() {
        LTCDecoder* ltcDecoder = ltc_decoder_create(1920, 32);
        LTCFrameExt ltcFrame;

        std::vector<uint8_t> inputBuffer(256);
        long int total = 0;

        while (_continue)
        {
            if (!_listener->readFromQueue(inputBuffer))
            {
                std::this_thread::sleep_for(chrono::milliseconds(5));
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

            // Always set the pause status, even when no frame has been received
            _clock.paused = paused;
            Timer::get().setMasterClockPaused(paused);

            ltc_decoder_write(ltcDecoder, (ltcsnd_sample_t*)inputBuffer.data(), inputBuffer.size(), total);
            total += inputBuffer.size();

            // Try reading a new LTC frame
            while (ltc_decoder_read(ltcDecoder, &ltcFrame))
            {
                _ready = true;

                SMPTETimecode stime;
                ltc_frame_to_time(&stime, &ltcFrame.ltc, LTC_TC_CLOCK);

                Timer::Point clock;
                clock.paused = paused;
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

                if (_masterClock)
                    Timer::get().setMasterClock(_clock);
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
Timer::Point LtcClock::getClock()
{
    return _clock;
}

} // namespace Splash
