#include "./graphics/texture.h"

#include "./image/image.h"
#include "./utils/log.h"
#include "./utils/timer.h"

#include <string>

using namespace std;

namespace Splash
{

/*************/
Texture::Texture(RootObject* root)
    : GraphObject(root)
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
bool Texture::linkTo(const shared_ptr<GraphObject>& obj)
{
    // Mandatory before trying to link
    return GraphObject::linkTo(obj);
}

/*************/
void Texture::registerAttributes()
{
    GraphObject::registerAttributes();
}

} // namespace Splash
