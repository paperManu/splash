/*
 * Copyright (C) 2017 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @sink_shmdata.h
 * The Sink_Shmdata_Encoded class, sending the connected object to shmdata
 */

#ifndef SPLASH_SINK_SHMDATA_ENCODED_H
#define SPLASH_SINK_SHMDATA_ENCODED_H

#include <unordered_map>

#include <shmdata/writer.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include "./osUtils.h"
#include "./sink.h"

namespace Splash
{

class Sink_Shmdata_Encoded : public Sink
{
  public:
    /**
     * Constructor
     */
    Sink_Shmdata_Encoded(std::weak_ptr<RootObject> root);

    /**
     * Destructor
     */
    ~Sink_Shmdata_Encoded();

  private:
    std::string _path{"/tmp/splash_sink"};
    std::string _caps{""};
    Utils::ShmdataLogger _logger;
    std::unique_ptr<shmdata::Writer> _writer{nullptr};
    ImageBufferSpec _previousSpec{};
    bool _resetEncoding{false};

    // FFmpeg objects
    AVCodec* _codec{nullptr};
    AVCodecContext* _context{nullptr};
    AVFrame *_frame{nullptr}, *_yuvFrame{nullptr};
    SwsContext* _swsContext{nullptr};
    AVPacket _packet;

    // Codec parameters
    int64_t _startTime{0ll};
    std::string _codecName{"h264"};
    int _bitRate{4000000};
    double _framerate{30.0};
    std::string _options{"profile=baseline"};

    /**
     * Find an encoder base on its name
     * \param encoderName Codec name
     * \return Return a codec
     */
    AVCodec* findEncoderByName(const std::string& codecName);

    /**
     * Init FFmpeg objects
     * \param spec Input image specifications
     * \return Return true if all went well
     */
    bool initFFmpegObjects(const ImageBufferSpec& spec);

    /**
     * Free everything related to FFmpeg
     */
    void freeFFmpegObjects();

    /**
     * Generate the caps from the spec, the context and the options
     * \param spec Input image specifications
     * \param optionString String of the options sent to the encoder
     * \param codecName Codec name
     * \param ctx FFmpeg codec context
     * \return Return a caps describing the output
     */
    std::string generateCaps(const ImageBufferSpec& spec, const std::string& optionString, const std::string& codecName, AVCodecContext* ctx);

    /**
     * Class to be implemented to copy the _mappedPixels somewhere
     * \param pixels Input image
     * \param spec Input image specifications
     */
    void handlePixels(const char* pixels, const ImageBufferSpec& spec);

    /**
     * Parse the options from the given string, formatted as:
     * option1=value1, option2=value2, etc
     * \param options Encoding options
     */
    std::unordered_map<std::string, std::string> parseOptions(const std::string& options);

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif
