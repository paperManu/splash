#include "./userInput_dragndrop.h"

#include "./scene.h"

using namespace std;

namespace Splash
{
/*************/
DragNDrop::DragNDrop(weak_ptr<RootObject> root)
    : UserInput(root)
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
    auto paths = Window::getPathDropped();
    if (paths.size() != 0)
    {
        State substate("dragndrop");
        for (auto& p : paths)
            substate.value.push_back(p);
        _state.emplace_back(std::move(substate));
    }
}

} // end of namespace
