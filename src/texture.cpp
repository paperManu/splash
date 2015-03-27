#include "texture.h"

#include "image.h"
#include "log.h"
#include "threadpool.h"
#include "timer.h"

#include <string>

#define SPLASH_TEXTURE_COPY_THREADS 4

using namespace std;
using namespace OIIO_NAMESPACE;

namespace Splash {

/*************/
Texture::Texture()
{
    init();
}

/*************/
Texture::Texture(RootObjectWeakPtr root)
        : BaseObject(root)
{
    init();
}

/*************/
Texture::Texture(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                 GLint border, GLenum format, GLenum type, const GLvoid* data)
{
    init();
}

/*************/
Texture::Texture(RootObjectWeakPtr root, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                 GLint border, GLenum format, GLenum type, const GLvoid* data)
        : BaseObject(root)
{
    init();
}

/*************/
Texture::~Texture()
{
#ifdef DEBUG
    SLog::log << Log::DEBUGGING << "Texture::~Texture - Destructor" << Log::endl;
#endif
}

/*************/
void Texture::init()
{
    registerAttributes();

    _type = "texture";
    _timestamp = chrono::high_resolution_clock::now();
}

/*************/
bool Texture::linkTo(BaseObjectPtr obj)
{
    // Mandatory before trying to link
    return BaseObject::linkTo(obj);
}

/*************/
void Texture::registerAttributes()
{
    _attribFunctions["filtering"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        _filtering = args[0].asInt() > 0 ? true : false;
        return true;
    }, [&]() -> Values {
        return {_filtering};
    });

    _attribFunctions["resizable"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        _resizable = args[0].asInt() > 0 ? true : false;
        return true;
    }, [&]() -> Values {
        return {_resizable};
    });
}

} // end of namespace
