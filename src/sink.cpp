#include "./sink.h"

#include <fstream>

using namespace std;

namespace Splash
{

/*************/
Sink::Sink(weak_ptr<RootObject> root)
    : BaseObject(root)
{
    _type = "sink";
    _renderingPriority = Priority::POST_CAMERA;
    registerAttributes();

    if (_root.expired())
        return;

    glGenBuffers(2, _pbos);
}

/*************/
Sink::~Sink()
{
    lock_guard<mutex> lock(_lockPixels);
    if (_root.expired())
        return;

    if (_handlePixelsThread.joinable())
        _handlePixelsThread.join();

    glDeleteBuffers(2, _pbos);
}

/*************/
bool Sink::linkTo(shared_ptr<BaseObject> obj)
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
void Sink::unlinkFrom(shared_ptr<BaseObject> obj)
{
    auto objAsTexture = dynamic_pointer_cast<Texture>(obj);
    if (objAsTexture)
        _inputTexture.reset();

    BaseObject::unlinkFrom(obj);
}

/*************/
void Sink::render()
{
    if (_handlePixelsThread.joinable())
        _handlePixelsThread.join();

    _handlePixelsThread = thread([&]() {
        lock_guard<mutex> lock(_lockPixels);
        handlePixels(reinterpret_cast<char*>(_mappedPixels), _spec);
    });
}

/*************/
void Sink::update()
{
    if (!_inputTexture)
        return;

    auto textureSpec = _inputTexture->getSpec();
    if (textureSpec.rawSize() == 0)
        return;

    if (_spec != textureSpec)
    {
        updatePbos(textureSpec.width, textureSpec.height, textureSpec.pixelBytes());
        _spec = textureSpec;
        _image = ImageBuffer(_spec);
    }

    _inputTexture->bind();
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[_pboWriteIndex]);
    if (_mappedPixels)
    {
        lock_guard<mutex> lock(_lockPixels); // Ensure that copy tasks are finished
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        _mappedPixels = nullptr;
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

    _pboWriteIndex = (_pboWriteIndex + 1) % 2;

    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[_pboWriteIndex]);
    _mappedPixels = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, _spec.rawSize(), GL_MAP_READ_BIT);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

/*************/
void Sink::updatePbos(int width, int height, int bytes)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[0]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * bytes, 0, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[1]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * bytes, 0, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

/*************/
void Sink::registerAttributes()
{
    BaseObject::registerAttributes();
}

} // end of namespace
