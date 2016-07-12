#include "./widget_text_box.h"

#include <imgui.h>

#include "./scene.h"

using namespace std;

namespace Splash
{

/*************/
void GuiTextBox::render()
{
    if (getText)
    {
        if (ImGui::CollapsingHeader(_name.c_str()))
            ImGui::Text(getText().c_str());
    }
}

} // end of namespace
