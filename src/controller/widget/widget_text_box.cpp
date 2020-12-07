#include "./controller/widget/widget_text_box.h"

#include <imgui.h>

#include "./core/scene.h"

namespace Splash
{

/*************/
void GuiTextBox::render()
{
    if (getText)
        ImGui::Text("%s", getText().c_str());
}

} // namespace Splash
