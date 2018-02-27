#include "texture.h"

#include "image.h"
#include "log.h"
#include "timer.h"

#include <string>

using namespace std;

namespace Splash
{

/*************/
Texture::Texture(RootObject* root)
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

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    _timestamp = Timer::getTime();
}

/*************/
bool Texture::linkTo(const shared_ptr<BaseObject>& obj)
{
    // Mandatory before trying to link
    return BaseObject::linkTo(obj);
}

/*************/
void Texture::registerAttributes()
{
    BaseObject::registerAttributes();
}

} // end of namespace
