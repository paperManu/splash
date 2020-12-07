#include "./graphics/texture.h"

#include "./image/image.h"
#include "./utils/log.h"
#include "./utils/timer.h"

#include <string>

namespace Splash
{

/*************/
Texture::Texture(RootObject* root)
    : GraphObject(root)
{
    _type = "texture";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;
}

/*************/
Texture::~Texture()
{
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Texture::~Texture - Destructor" << Log::endl;
#endif
}

/*************/
void Texture::registerAttributes()
{
    GraphObject::registerAttributes();

    addAttribute("timestamp", [](const Values&) { return true; }, [&]() -> Values { return {_spec.timestamp}; });
    setAttributeDescription("timestamp", "Timestamp (in µs) for the current texture, which mimicks the timestamp of the input image (if any)");
}

} // namespace Splash
