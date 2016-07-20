#include "texture.h"

#include "image.h"
#include "log.h"
#include "threadpool.h"
#include "timer.h"

#include <string>

using namespace std;

namespace Splash {

/*************/
Texture::Texture(std::weak_ptr<RootObject> root)
        : BaseObject(root)
{
    init();
}

/*************/
Texture::~Texture()
{
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Texture::~Texture - Destructor" << Log::endl;
#endif
}

/*************/
void Texture::init()
{
    _type = "texture";
    registerAttributes();

    // If the root object weak_ptr is expired, this means that
    // this object has been created outside of a World or Scene.
    // This is used for getting documentation "offline"
    if (_root.expired())
        return;

    _timestamp = Timer::getTime();
}

/*************/
bool Texture::linkTo(shared_ptr<BaseObject> obj)
{
    // Mandatory before trying to link
    return BaseObject::linkTo(obj);
}

/*************/
void Texture::registerAttributes()
{
}

} // end of namespace
