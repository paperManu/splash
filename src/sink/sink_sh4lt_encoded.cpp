#include "./sink/sink_sh4lt_encoded.h"

#include <algorithm>
#include <regex>

#include <sh4lt/shtype/shtype-from-gst-caps.hpp>

#include "./utils/timer.h"

namespace Splash
{

/*************/
Sink_Sh4lt_Encoded::Sink_Sh4lt_Encoded(RootObject* root)
    : Sink(root)
{
    _type = "sink_sh4lt_encoded";
    registerAttributes();
}

/*************/
Sink_Sh4lt_Encoded::~Sink_Sh4lt_Encoded()
{
    freeFFmpegObjects();
}

/*************/
const AVCodec* Sink_Sh4lt_Encoded::findEncoderByName(const std::string& codecName)
{
    const AVCodec* codec{nullptr};

    if (codecName == "h264")
    {
        codec = avcodec_find_encoder_by_name("h264_nvenc");
        if (!codec)
            codec = avcodec_find_encoder_by_name("h264_vaapi");
        if (!codec)
            codec = avcodec_find_encoder_by_name("h264_amf");
        if (!codec)
            codec = avcodec_find_encoder_by_name("libx264");
    }
    else if (codecName == "hevc")
    {
        codec = avcodec_find_encoder_by_name("hvec_nvenc");
        if (!codec)
            codec = avcodec_find_encoder_by_name("hevc_vaapi");
        if (!codec)
            codec = avcodec_find_encoder_by_name("hevc_amf");
        if (!codec)
            codec = avcodec_find_encoder_by_name("libx265");
    }

    return codec;
}

/*************/
const std::unordered_map<std::string, std::string> Sink_Sh4lt_Encoded::parseOptions(const std::string& options)
{
    static const std::regex re("(([^=]+)=([^ ,]+))+");
    const auto sre_begin = std::sregex_iterator(options.begin(), options.end(), re);
    const auto sre_end = std::sregex_iterator();

    std::unordered_map<std::string, std::string> result;
    for (auto i = sre_begin; i != sre_end; ++i)
    {
        const std::smatch match = *i;
        const auto key = std::string(match[2]);
        const auto value = std::string(match[3]);
        result[key] = value;
    }

    return result;
}

/*************/
bool Sink_Sh4lt_Encoded::initFFmpegObjects(const ImageBufferSpec& spec)
{
    _codec = findEncoderByName(_codecName);
    if (!_codec)
    {
        Log::get() << Log::WARNING << "Sink_Sh4lt_Encoded::" << __FUNCTION__ << " - Unable to find encoder for codec " << _codecName << Log::endl;
        return false;
    }

    _context = avcodec_alloc_context3(_codec);
    if (!_context)
    {
        Log::get() << Log::WARNING << "Sink_Sh4lt_Encoded::" << __FUNCTION__ << " - Unable to allocate video codec context for codec " << _codecName << Log::endl;
        return false;
    }

    _context->bit_rate = _bitRate;
    _context->width = spec.width;
    _context->height = spec.height;
    _context->time_base = (AVRational){1, static_cast<int>(_framerate)};
    _context->sample_aspect_ratio = (AVRational){static_cast<int>(spec.width), static_cast<int>(spec.height)};
    _context->pix_fmt = AV_PIX_FMT_YUV420P;

    const auto options = parseOptions(_options);
    for (auto& option : options)
        av_opt_set(_context->priv_data, option.first.c_str(), option.second.c_str(), 0);

    if (avcodec_open2(_context, _codec, nullptr) < 0)
    {
        Log::get() << Log::WARNING << "Sink_Sh4lt_Encoded::" << __FUNCTION__ << " - Unable to open codec " << _codecName << Log::endl;
        return false;
    }

    _swsContext = sws_getContext(spec.width, spec.height, AV_PIX_FMT_RGB32, spec.width, spec.height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);

    _frame = av_frame_alloc();
    _yuvFrame = av_frame_alloc();
    if (!_frame || !_yuvFrame)
    {
        Log::get() << Log::WARNING << "Sink_Sh4lt_Encoded::" << __FUNCTION__ << " - Unable to allocate frame" << Log::endl;
        return false;
    }

    _frame->format = AV_PIX_FMT_RGB32;
    _frame->width = spec.width;
    _frame->height = spec.height;
    if (av_image_alloc(_frame->data, _frame->linesize, _context->width, _context->height, AV_PIX_FMT_RGB32, 32) < 0)
    {
        Log::get() << Log::WARNING << "Sink_Sh4lt_Encoded::" << __FUNCTION__ << " - Unable to allocate raw RGBA picture buffer" << Log::endl;
        return false;
    }

    _yuvFrame->format = AV_PIX_FMT_YUV420P;
    _yuvFrame->width = spec.width;
    _yuvFrame->height = spec.height;
    if (av_image_alloc(_yuvFrame->data, _yuvFrame->linesize, _context->width, _context->height, AV_PIX_FMT_YUV420P, 32) < 0)
    {
        Log::get() << Log::WARNING << "Sink_Sh4lt_Encoded::" << __FUNCTION__ << " - Unable to allocate raw YUV420 picture buffer" << Log::endl;
        return false;
    }

    _startTime = Timer::get().getTime();
    return true;
}

/*************/
void Sink_Sh4lt_Encoded::freeFFmpegObjects()
{
    if (_context)
    {
        av_free(_context);
        _context = nullptr;
    }

    if (_frame)
        av_frame_free(&_frame);
    if (_yuvFrame)
        av_frame_free(&_yuvFrame);

    if (_swsContext)
        sws_freeContext(_swsContext);
}

/*************/
std::string Sink_Sh4lt_Encoded::generateCaps(const ImageBufferSpec& spec, uint32_t framerate, const std::string& optionString, const std::string& codecName, AVCodecContext* ctx)
{
    if (!ctx)
        return "video/x-raw";

    auto codec = std::string(reinterpret_cast<char*>(&ctx->codec_tag), 4);
    if (ctx->codec_tag == 0)
        codec = "x-" + codecName;

    auto profile = std::string("baseline");

    const auto options = parseOptions(optionString);
    auto optionIt = options.find("profile");
    if (optionIt != options.end())
    {
        profile = optionIt->second;
    }
    else
    {
        uint8_t* profileStr{nullptr};
        if (av_opt_get(ctx, "profile", 0, &profileStr))
            profile = std::string(reinterpret_cast<char*>(profileStr));
    }

    auto caps = "video/" + codec + ",stream-format=(string)byte_stream,alignment=(string)au,profile=(string)" + profile + ",width=(int)" + std::to_string(spec.width) +
                ",height=(int)" + std::to_string(spec.height) + ",pixel-aspect-ratio=(fraction)1/1,framerate=(fraction)" + std::to_string(framerate) + "/1";

    return caps;
}

/*************/
void Sink_Sh4lt_Encoded::handlePixels(const char* pixels, const ImageBufferSpec& spec)
{
    auto size = spec.rawSize();
    if (!pixels || size == 0)
        return;

    if (_resetEncoding || !_context || !_writer || spec != _previousSpec || _previousFramerate != _framerate)
    {
        _resetEncoding = false;

        // Reset FFmpeg context and stuff
        freeFFmpegObjects();
        if (!initFFmpegObjects(spec))
            return;

        // Reset sh4lt writer
        _shtype = sh4lt::shtype::shtype_from_gst_caps(generateCaps(spec, _framerate, _options, _codecName, _context), _label, _group);
        _writer.reset();
        _writer = std::make_unique<sh4lt::Writer>(_shtype, size, std::make_shared<Utils::Sh4ltLogger>());

        _previousSpec = spec;
        _previousFramerate = _framerate;
    }

    // Encoding
    AVPacket* packet = av_packet_alloc();
    if (!packet)
    {
        Log::get() << Log::ERROR << "Sink_Sh4lt_Encoded::" << __FUNCTION__ << " - Can not allocate memory for encoding frame" << Log::endl;
    }

    packet->data = nullptr;
    packet->size = 0;

    av_image_fill_arrays(_frame->data, _frame->linesize, reinterpret_cast<const uint8_t*>(pixels), AV_PIX_FMT_RGB32, spec.width, spec.height, 1);
    sws_scale(_swsContext, _frame->data, _frame->linesize, 0, spec.height, _yuvFrame->data, _yuvFrame->linesize);

    _yuvFrame->pts = (static_cast<double>((Timer::get().getTime() - _startTime)) / 1e3) / _framerate;
    _yuvFrame->quality = _context->global_quality;
    _yuvFrame->pict_type = AV_PICTURE_TYPE_NONE;

    auto ret = avcodec_send_frame(_context, _yuvFrame);
    if (ret < 0)
    {
        Log::get() << Log::WARNING << "Sink_Sh4lt_Encoded::" << __FUNCTION__ << " - Error encoding frame" << Log::endl;
        return;
    }

    while (1)
    {
        ret = avcodec_receive_packet(_context, packet);
        if (ret == AVERROR(EAGAIN))
            break;
        else if (ret < 0)
            return;

        // Sending through sh4lt
        if (_writer && packet->size != 0)
        {
            // Sh4lt timestamp is expressed in nanoseconds, while Splash in microseconds
            _writer->copy_to_shm(packet->data, packet->size, 1000 * spec.timestamp, 1000000000 / _framerate);
        }
    }

    av_packet_unref(packet);
}

/*************/
void Sink_Sh4lt_Encoded::registerAttributes()
{
    Sink::registerAttributes();

    addAttribute(
        "group",
        [&](const Values& args) {
            _group = args[0].as<std::string>();
            _previousSpec = ImageBufferSpec();
            return true;
        },
        [&]() -> Values { return {_group}; },
        {'s'});
    setAttributeDescription("group", "Sh4lt group label");

    addAttribute(
        "label",
        [&](const Values& args) {
            _label = args[0].as<std::string>();
            _previousSpec = ImageBufferSpec();
            return true;
        },
        [&]() -> Values { return {_label}; },
        {'s'});
    setAttributeDescription("label", "Sh4lt group label");

    addAttribute(
        "bitrate",
        [&](const Values& args) {
            _bitRate = std::max(1000000, args[0].as<int>());
            _resetEncoding = true;
            return true;
        },
        [&]() -> Values { return {_bitRate}; },
        {'i'});
    setAttributeDescription("bitrate", "Output encoded video target bitrate");

    addAttribute("sh4lt type", [&]() -> Values { return {sh4lt::shtype::shtype_to_gst_caps(_shtype)}; });
    setAttributeDescription("sh4lt type", "format of the data sent to the Sh4lt");

    addAttribute(
        "codec",
        [&](const Values& args) {
            _codecName = args[0].as<std::string>();
            transform(_codecName.begin(), _codecName.end(), _codecName.begin(), ::tolower);
            _resetEncoding = true;
            return true;
        },
        [&]() -> Values { return {_codecName}; },
        {'s'});
    setAttributeDescription("codec", "Desired codec");

    addAttribute(
        "codecOptions",
        [&](const Values& args) {
            _options = args[0].as<std::string>();
            _resetEncoding = true;
            return true;
        },
        [&]() -> Values { return {_options}; },
        {'s'});
    setAttributeDescription("codecOptions",
        "Codec options as a string following the format: \"key1=value1, key2=value2, etc\".\n"
        "Options can be listed with the following terminal command:\n"
        "$ ffmpeg -h encoder=ENCODER_NAME");
}

} // namespace Splash
