#include "./controller/widget/widget_tree.h"

#include <imgui.h>

#include "./utils/osutils.h"

using namespace std;

namespace Splash
{

/*************/
void GuiTree::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        auto tree = _root->getTree();

        std::function<void(const string&, const string&)> printBranch;
        printBranch = [&](const string& path, const string& nodeName) {
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
                    ImGui::Text("%s : %s", leaf.c_str(), leafValue.as<string>().c_str());
                }

                ImGui::TreePop();
            }
        };

        printBranch("/", "/");
    }
}

} // namespace Splash
