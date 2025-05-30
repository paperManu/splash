#include "./image_sh4lt.h"

#include <regex>

#include <hap.h>

// All existing 64bits x86 CPUs have SSE2
#if __x86_64__
#define GLM_FORCE_SSE2
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#else
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#endif

#include "./utils/cgutils.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

#define MEDIA_BASE_CAPS "video/"

namespace Splash
{

/*************/
Image_Sh4lt::Image_Sh4lt(RootObject* root)
    : Image(root)
{
    _type = "image_sh4lt";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    _monitor = std::make_unique<sh4lt::monitor::Monitor>(std::make_shared<Utils::Sh4ltLogger>(), sh4lt::ShType::get_socket_dir());

    addPeriodicTask(
        "update-monitor",
        [this]() {
            _monitor->update();
            const auto stats = _monitor->get_stats();
            std::set<std::string> groups = {sh4lt::ShType::default_group()};
            std::set<std::string> labels;
            std::map<std::string, std::string> medias;
            for (const auto& key : stats->get_child_keys("infos"))
            {
                const auto group = sh4lt::Any::to_string(stats->branch_get_value("infos." + key + ".shtype.group"));
                const auto label = sh4lt::Any::to_string(stats->branch_get_value("infos." + key + ".shtype.label"));
                const auto media = sh4lt::Any::to_string(stats->branch_get_value("infos." + key + ".shtype.media"));

                if (media.find(MEDIA_BASE_CAPS) == std::string::npos)
                    continue;

                groups.insert(group);
                if (group != _group)
                    continue;

                labels.insert(label);
                medias.insert_or_assign(label, media);
            }

            std::unique_lock<std::mutex> lock(_monitorMutex);
            _groups = std::move(groups);
            _labels = std::move(labels);
            _medias = std::move(medias);
        },
        2000);
}

/*************/
Image_Sh4lt::~Image_Sh4lt()
{
    _reader.reset();
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Image_Sh4lt::~Image_Sh4lt - Destructor" << Log::endl;
#endif
}

/*************/
bool Image_Sh4lt::read(const std::string& /*filename*/)
{
    // We don't do anything with the mere filename,
    // reading is done by setting the group and label
    // attributes
    return true;
}

/*************/
bool Image_Sh4lt::readByLabel()
{
    _reader = std::make_unique<sh4lt::Follower>(
        sh4lt::ShType::get_path(_label, _group),
        [&](void* data, size_t size, const sh4lt::Time::info_t*) { onData(data, size); },
        [&](const sh4lt::ShType& caps) { onShType(caps); },
        [&]() {},
        std::make_shared<Utils::Sh4ltLogger>());

    return true;
}

/*************/
// Small function to work around a bug in GCC's libstdc++
void Image_Sh4lt::removeExtraParenthesis(std::string& str)
{
    if (str.find(")") == 0)
        str = str.substr(1);
}

/*************/
void Image_Sh4lt::onShType(const sh4lt::ShType& shtype)
{
    Log::get() << Log::MESSAGE << "Image_Sh4lt::" << __FUNCTION__ << " - Trying to connect with the following ShType: " << sh4lt::ShType::serialize(shtype) << Log::endl;

    _bpp = 0;
    _width = 0;
    _height = 0;
    _red = 0;
    _green = 0;
    _blue = 0;
    _channels = 0;
    _isVideo = false;
    _isDepth = false;
    _isHap = false;
    _isYUV = false;
    _is420 = false;
    _is422 = false;

    if (shtype.media() == "video/x-raw")
    {
        const auto format = shtype.get_prop("format").as<std::string>();
        if ("BGR" == format)
        {
            _bpp = 24;
            _channels = 3;
            _red = 2;
            _green = 1;
            _blue = 0;
        }
        else if ("RGB" == format)
        {
            _bpp = 24;
            _channels = 3;
            _red = 0;
            _green = 1;
            _blue = 2;
        }
        else if ("RGBA" == format)
        {
            _bpp = 32;
            _channels = 4;
            _red = 2;
            _green = 1;
            _blue = 0;
        }
        else if ("BGRA" == format)
        {
            _bpp = 32;
            _channels = 4;
            _red = 0;
            _green = 1;
            _blue = 2;
        }
        else if ("Y16" == format)
        {
            _bpp = 16;
            _channels = 1;
            _isDepth = true;
        }
        else if ("I420" == format)
        {
            _bpp = 12;
            _channels = 3;
            _isYUV = true;
            _is420 = true;
        }
        else if ("UYVY" == format)
        {
            _bpp = 12;
            _channels = 3;
            _isYUV = true;
            _is422 = true;
        }
    }
    else if (shtype.media() == "video/x-gst-fourcc-HapY")
    {
        _isHap = true;
    }
    else
    {
        Log::get() << Log::WARNING << "Image_Sh4lt::" << __FUNCTION__ << " - Incoming sh4lt seems not to be of a supported video format" << Log::endl;
        return;
    }

    _width = shtype.get_prop("width").as<int>();
    _height = shtype.get_prop("height").as<int>();
    _isVideo = true;

    Log::get() << Log::MESSAGE << "Image_Sh4lt::" << __FUNCTION__ << " - Connection successful" << Log::endl;
}

/*************/
void Image_Sh4lt::onData(void* data, int data_size)
{
    if (Timer::get().isDebug())
        Timer::get() << "image_sh4lt " + _name;

    if (!_isVideo)
        return;

    // Standard images, RGB or YUV
    if (_width != 0 && _height != 0 && _bpp != 0 && _channels != 0)
        readUncompressedFrame(data, data_size);
    // Hap compressed images
    else if (_isHap == true)
        readHapFrame(data, data_size);

    if (Timer::get().isDebug())
        Timer::get() >> ("image_sh4lt " + _name);
}

/*************/
void Image_Sh4lt::readHapFrame(void* data, int data_size)
{
    // We are using kind of a hack to store a DXT compressed image in an ImageBuffer
    // First, we check the texture format type
    auto textureFormat = std::string("");
    if (!hapDecodeFrame(data, data_size, nullptr, 0, textureFormat))
        return;

    // Check if we need to resize the reader buffer
    // We set the size so as to have just enough place for the given texture format
    auto bufSpec = _readerBuffer.getSpec();
    if (bufSpec.width != _width || (bufSpec.height != _height && textureFormat != _textureFormat))
    {
        _textureFormat = textureFormat;

        ImageBufferSpec spec;
        if (textureFormat == "RGB_DXT1")
            spec = ImageBufferSpec(_width, (int)(ceil((float)_height / 2.f)), 1, 8, ImageBufferSpec::Type::UINT8);
        else if (textureFormat == "RGBA_DXT5")
            spec = ImageBufferSpec(_width, _height, 1, 8, ImageBufferSpec::Type::UINT8);
        else if (textureFormat == "YCoCg_DXT5")
            spec = ImageBufferSpec(_width, _height, 1, 8, ImageBufferSpec::Type::UINT8);
        else
            return;

        spec.format = textureFormat;
        _readerBuffer = ImageBuffer(spec);
    }

    unsigned long outputBufferBytes = bufSpec.width * bufSpec.height * bufSpec.channels;
    if (!hapDecodeFrame(data, data_size, _readerBuffer.data(), outputBufferBytes, textureFormat))
        return;

    {
        std::lock_guard<Spinlock> updateLock(_updateMutex);
        if (!_bufferImage)
            _bufferImage = std::make_unique<ImageBuffer>();
        std::swap(*(_bufferImage), _readerBuffer);
        _bufferImageUpdated = true;
    }
    updateTimestamp();
}

/*************/
void Image_Sh4lt::readUncompressedFrame(void* data, int /*data_size*/)
{
    // Check if we need to resize the reader buffer
    auto bufSpec = _readerBuffer.getSpec();
    if (bufSpec.width != _width || bufSpec.height != _height || bufSpec.channels != _channels)
    {
        ImageBufferSpec spec(_width, _height, _channels, 8 * _channels, ImageBufferSpec::Type::UINT8);
        if (_green < _blue)
            spec.format = "BGR";
        else
            spec.format = "RGB";
        if (_channels == 4)
            spec.format.push_back('A');

        if (_isDepth)
        {
            spec.format = "R";
            spec.bpp = 16;
            spec.channels = 1;
        }
        else if (_is420 || _is422)
        {
            spec.format = "UYVY";
            spec.bpp = 16;
            spec.channels = 2;
        }

        _readerBuffer = ImageBuffer(spec);
    }

    if (!_isYUV && (_channels == 3 || _channels == 4))
    {
        char* pixels = (char*)(_readerBuffer).data();
        std::vector<std::future<void>> threads;
        for (uint32_t block = 0; block < _sh4ltCopyThreads; ++block)
        {
            int size = _width * _height * _channels * sizeof(char);
            threads.push_back(std::async(std::launch::async, [=]() {
                int sizeOfBlock; // We compute the size of the block, to handle image size non divisible by _sh4ltCopyThreads
                if (size - size / _sh4ltCopyThreads * block < 2 * size / _sh4ltCopyThreads)
                    sizeOfBlock = size - size / _sh4ltCopyThreads * block;
                else
                    sizeOfBlock = size / _sh4ltCopyThreads;

                memcpy(pixels + size / _sh4ltCopyThreads * block, (const char*)data + size / _sh4ltCopyThreads * block, sizeOfBlock);
            }));
        }
    }
    else if (_isDepth)
    {
        auto pixels = reinterpret_cast<char*>(_readerBuffer.data());
        memcpy(pixels, data, _readerBuffer.getSpec().rawSize());
    }
    else if (_is420)
    {
        const auto input = std::span(reinterpret_cast<uint8_t*>(data), _width * _height * 3 / 2);
        auto output = std::span(_readerBuffer.data(), _readerBuffer.getSize());
        cvtI420toUYVY(input, output, _width, _height);
    }
    else if (_is422)
    {
        const unsigned char* YUV = static_cast<const unsigned char*>(data);
        char* pixels = (char*)(_readerBuffer).data();
        std::copy(YUV, YUV + _width * _height * 2, pixels);
    }
    else
        return;

    {
        std::lock_guard<Spinlock> updateLock(_updateMutex);
        if (!_bufferImage)
            _bufferImage = std::make_unique<ImageBuffer>();
        std::swap(*(_bufferImage), _readerBuffer);
        _bufferImageUpdated = true;
    }
    updateTimestamp();
}

/*************/
void Image_Sh4lt::registerAttributes()
{
    addAttribute(
        "label",
        [&](const Values& args) {
            _label = args[0].as<std::string>();
            return readByLabel();
        },
        [&]() -> Values {
            Values retValues = {_label};
            std::unique_lock<std::mutex> lock(_monitorMutex);
            for (const auto& label : _labels)
                retValues.push_back(label);

            return retValues;
        },
        {'s'},
        true);
    setAttributeDescription("label", "Label for the Sh4lt (applied if filename attribute is empty)");

    addAttribute(
        "group",
        [&](const Values& args) {
            _group = args[0].as<std::string>();
            return readByLabel();
        },
        [&]() -> Values {
            Values retValues = {_group};
            std::unique_lock<std::mutex> lock(_monitorMutex);
            for (const auto& group : _groups)
                retValues.push_back(group);

            return retValues;
        },
        {'s'},
        true);
    setAttributeDescription("group", "Group for the Sh4lt (applied if filename attribute is empty)");

    addAttribute("sh4ltInfo", [&]() -> Values {
        std::unique_lock<std::mutex> lock(_monitorMutex);
        if (_medias.find(_label) != _medias.end())
            return {_medias[_label]};
        return {};
    });
    setAttributeDescription("sh4ltInfo", "Media information for the Sh4lt input");

    Image::registerAttributes();
}
} // namespace Splash
