#include "./image/image_shmv.h"

#ifdef HAVE_SYS_SHM_H
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

namespace Splash
{

/*************/
Image_ShmV::Image_ShmV(RootObject* root)
    : Image(root)
{
    _type = "image_shmv";
    registerAttributes();
}

/*************/
void Image_ShmV::registerAttributes()
{
    Image::registerAttributes();
}

} // namespace Splash
