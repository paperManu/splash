#include "./widget_text_box.h"

#include <imgui.h>

#include "./core/scene.h"

using namespace std;

namespace Splash
{

/*************/
void GuiTextBox::render()
{
    if (getText)
    {
        if (ImGui::CollapsingHeader(_name.c_str()))
            ImGui::Text("%s", getText().c_str());
    }
}

} // end of namespace
