#include "./userInput_dragndrop.h"

using namespace std;

namespace Splash
{
/*************/
DragNDrop::DragNDrop(weak_ptr<RootObject> root) : UserInput(root)
{
    _type = "dragndrop";
    _updateRate = 10;
}

/*************/
DragNDrop::~DragNDrop()
{
}

/*************/
void DragNDrop::updateMethod()
{
    // Any file dropped onto the window? Then load it.
    auto paths = Window::getPathDropped();
    if (paths.size() != 0)
        setGlobal("loadConfig", {paths[0]});
}

} // end of namespace
