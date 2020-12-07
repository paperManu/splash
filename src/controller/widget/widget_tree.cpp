#include "./controller/widget/widget_tree.h"

#include <imgui.h>

#include "./utils/osutils.h"

namespace Splash
{

/*************/
void GuiTree::render()
{
    auto tree = _root->getTree();

    std::function<void(const std::string&, const std::string&)> printBranch;
    printBranch = [&](const std::string& path, const std::string& nodeName) {
        if (ImGui::TreeNode(nodeName.c_str()))
        {
            for (const auto& branch : tree->getBranchListAt(path))
            {
                auto childPath = Utils::cleanPath(path + "/" + branch);
                printBranch(childPath, branch);
            }

            for (const auto& leaf : tree->getLeafListAt(path))
            {
                Value leafValue;
                tree->getValueForLeafAt(Utils::cleanPath(path + "/" + leaf), leafValue);
                ImGui::Text("%s : %s", leaf.c_str(), leafValue.as<std::string>().c_str());
            }

            ImGui::TreePop();
        }
    };

    printBranch("/", "/");
}

} // namespace Splash
