#include "./sink.h"

#include <fstream>

#include "./timer.h"

using namespace std;

namespace Splash
{

/*************/
Sink::Sink(RootObject* root)
    : BaseObject(root)
{
    _type = "sink";
    _renderingPriority = Priority::POST_CAMERA;
    registerAttributes();

    if (!_root)
        return;
}

/*************/
Sink::~Sink()
{
    if (!_root)
        return;

    if (_mappedPixels)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[_pboWriteIndex]);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        _mappedPixels = nullptr;
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    glDeleteBuffers(_pbos.size(), _pbos.data());
}

/*************/
bool Sink::linkTo(const shared_ptr<BaseObject>& obj)
{
    // Mandatory before trying to link
    if (!BaseObject::linkTo(obj))
        return false;

    auto objAsTexture = dynamic_pointer_cast<Texture>(obj);
    if (objAsTexture)
    {
        _inputTexture = objAsTexture;
        return true;
    }

    return false;
}

/*************/
void Sink::unlinkFrom(const shared_ptr<BaseObject>& obj)
{
    auto objAsTexture = dynamic_pointer_cast<Texture>(obj);
    if (objAsTexture)
        _inputTexture.reset();

    BaseObject::unlinkFrom(obj);
}

/*************/
void Sink::render()
{
    if (!_inputTexture || !_mappedPixels)
        return;

    handlePixels(reinterpret_cast<char*>(_mappedPixels), _spec);
}

/*************/
void Sink::update()
{
    if (!_inputTexture)
        return;

    auto textureSpec = _inputTexture->getSpec();
    if (textureSpec.rawSize() == 0)
        return;

    _inputTexture->bind();
    if (!_pbos.empty())
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[_pboWriteIndex]);
        if (_mappedPixels)
        {
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            _mappedPixels = nullptr;
        }
    }

    if (!_opened)
        return;

    uint64_t currentTime = Timer::get().getTime();
    uint64_t period = static_cast<uint64_t>(1e6 / (double)_framerate);
    if (period != 0 && _lastFrameTiming != 0 && currentTime - _lastFrameTiming < period)
        return;
    _lastFrameTiming = currentTime;

    if (_spec != textureSpec || _pbos.size() != _pboCount)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        updatePbos(textureSpec.width, textureSpec.height, textureSpec.pixelBytes());
        _spec = textureSpec;
        _image = ImageBuffer(_spec);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[_pboWriteIndex]);
    }

    if (_spec.bpp == 32)
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
    else if (_spec.bpp == 24)
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    else if (_spec.bpp == 16 && _spec.channels != 1)
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_UNSIGNED_SHORT, 0);
    else if (_spec.bpp == 16 && _spec.channels == 1)
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_SHORT, 0);
    else if (_spec.bpp == 8)
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    _inputTexture->unbind();

    _pboWriteIndex = (_pboWriteIndex + 1) % _pbos.size();

    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[_pboWriteIndex]);
    _mappedPixels = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, _spec.rawSize(), GL_MAP_READ_BIT);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

/*************/
void Sink::updatePbos(int width, int height, int bytes)
{
    if (!_pbos.empty())
        glDeleteBuffers(_pbos.size(), _pbos.data());

    _pbos.resize(_pboCount);
    glGenBuffers(_pbos.size(), _pbos.data());

    for (int i = 0; i < _pbos.size(); ++i)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[i]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * bytes, 0, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    _pboWriteIndex = 0;
}

/*************/
void Sink::registerAttributes()
{
    BaseObject::registerAttributes();

    addAttribute("bufferCount",
        [&](const Values& args) {
            _pboCount = max(args[0].as<int>(), 2);
            return true;
        },
        [&]() -> Values { return {(int)_pboCount}; },
        {'n'});
    setAttributeDescription("bufferCount", "Number of GPU buffers to use for data download to CPU memory");

    addAttribute("framerate",
        [&](const Values& args) {
            _framerate = max(1, args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {(int)_framerate}; },
        {'n'});
    setAttributeDescription("framerate", "Maximum framerate, additional frames are dropped");

    addAttribute("opened",
        [&](const Values& args) {
            _opened = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {static_cast<int>(_opened)}; },
        {'n'});
    setAttributeDescription("opened", "If true, the sink lets frames through");
}

} // end of namespace
