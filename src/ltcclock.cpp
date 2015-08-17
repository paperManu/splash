#include "ltcclock.h"

namespace Splash {

/*************/
LtcClock::LtcClock()
{
    _ltcDecoder = ltc_decoder_create(1920, 32);
}

/*************/
LtcClock::~LtcClock()
{
    if (_ltcDecoder)
        ltc_decoder_free(_ltcDecoder);
}

} // end of namespace
