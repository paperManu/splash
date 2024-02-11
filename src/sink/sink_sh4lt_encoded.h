/*
 * Copyright (C) 2024 Splash authors
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
 * @sink_sh4lt.h
 * The Sink_Sh4lt_Encoded class, sending the connected object to sh4lt
 */

#ifndef SPLASH_SINK_SH4LT_ENCODED_H
#define SPLASH_SINK_SH4LT_ENCODED_H

#include <unordered_map>

#include <sh4lt/writer.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include "./sink/sink.h"
#include "./utils/osutils.h"

namespace Splash
{

class Sink_Sh4lt_Encoded final : public Sink
{
  public:
    /**
     * Constructor
     */
    explicit Sink_Sh4lt_Encoded(RootObject* root);

    /**
     * Destructor
     */
    ~Sink_Sh4lt_Encoded() final;

    /**
     * Constructors/operators
     */
    Sink_Sh4lt_Encoded(const Sink_Sh4lt_Encoded&) = delete;
    Sink_Sh4lt_Encoded& operator=(const Sink_Sh4lt_Encoded&) = delete;
    Sink_Sh4lt_Encoded(Sink_Sh4lt_Encoded&&) = delete;
    Sink_Sh4lt_Encoded& operator=(Sink_Sh4lt_Encoded&&) = delete;

  private:
    std::string _group{sh4lt::ShType::default_group()};
    std::string _label{"splash encoded"};
    sh4lt::ShType _shtype{};
    Utils::Sh4ltLogger _logger;
    std::unique_ptr<sh4lt::Writer> _writer{nullptr};
    ImageBufferSpec _previousSpec{};
    uint32_t _previousFramerate{0};
    bool _resetEncoding{false};

    // FFmpeg objects
    const AVCodec* _codec{nullptr};
    AVCodecContext* _context{nullptr};
    AVFrame *_frame{nullptr}, *_yuvFrame{nullptr};
    SwsContext* _swsContext{nullptr};

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
    const AVCodec* findEncoderByName(const std::string& codecName);

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
     * \param framerate Expected framerate
     * \param optionString String of the options sent to the encoder
     * \param codecName Codec name
     * \param ctx FFmpeg codec context
     * \return Return a caps describing the output
     */
    std::string generateCaps(const ImageBufferSpec& spec, uint32_t framerate, const std::string& optionString, const std::string& codecName, AVCodecContext* ctx);

    /**
     * Class to be implemented to copy the _mappedPixels somewhere
     * \param pixels Input image
     * \param spec Input image specifications
     */
    void handlePixels(const char* pixels, const ImageBufferSpec& spec) final;

    /**
     * Parse the options from the given string, formatted as:
     * option1=value1, option2=value2, etc
     * \param options Encoding options
     */
    const std::unordered_map<std::string, std::string> parseOptions(const std::string& options);

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // namespace Splash

#endif
