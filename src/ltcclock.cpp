#include "ltcclock.h"

#include <chrono>
#include <iostream>

using namespace std;

namespace Splash {

/*************/
LtcClock::LtcClock()
{
    registerAttributes();

    _listener = unique_ptr<Listener>(new Listener());
    _listener->setParameters(1, 44100, Listener::SAMPLE_FMT_FLT);
    if (!_listener)
    {
        _listener.reset();
        return;
    }

    _ready = true;
    _continue = true;

    _ltcThread = thread([&]() {
        LTCDecoder* ltcDecoder = ltc_decoder_create(1920, 32);
        LTCFrameExt ltcFrame;

        vector<float> inputBuffer(8192);
        long int total = 0;

        while (_continue)
        {
            if (!_listener->readFromQueue(inputBuffer))
                this_thread::sleep_for(chrono::milliseconds(10));

            ltc_decoder_write_float(ltcDecoder, inputBuffer.data(), inputBuffer.size(), total);
            total += inputBuffer.size();

            while (ltc_decoder_read(ltcDecoder, &ltcFrame))
            {
                SMPTETimecode stime;
                ltc_frame_to_time(&stime, &ltcFrame.ltc, LTC_TC_CLOCK);
                cout << stime.hours << ":" << stime.mins << ":" << stime.secs << ":" << stime.frame << endl;

                unique_lock<mutex> lock(_ltcMutex);
                _clock.hours = stime.hours;
                _clock.mins = stime.mins;
                _clock.secs = stime.secs;
                _clock.frame = stime.frame;
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
    unique_lock<mutex> lock(_ltcMutex);
    return _clock;
}

/*************/
void LtcClock::registerAttributes()
{
}

} // end of namespace
