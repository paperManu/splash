#include "./widget_template.h"

#include <array>
#include <fstream>

#include <imgui.h>

#include "./core/scene.h"

using namespace std;

namespace Splash
{

/*************/
void GuiTemplate::render()
{
    if (!_templatesLoaded)
    {
        loadTemplates();
        _templatesLoaded = true;
    }

    if (_textures.size() == 0)
        return;

    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        bool firstTemplate = true;
        for (auto& name : _names)
        {
            if (!firstTemplate)
                ImGui::SameLine(0.f, 2.f);
            firstTemplate = false;

            if (ImGui::ImageButton((void*)(intptr_t)_textures[name]->getTexId(), ImVec2(128, 128)))
            {
                string configPath = string(DATADIR) + "templates/" + name + ".json";
#if HAVE_OSX
                if (!ifstream(configPath, ios::in | ios::binary))
                    configPath = "../Resources/templates/" + name + ".json";
#endif
                setWorldAttribute("loadConfig", {configPath});
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", _descriptions[name].data());
        }
    }
}

/*************/
void GuiTemplate::loadTemplates()
{
    auto templatePath = string(DATADIR);
    auto examples = vector<string>();
    auto descriptions = vector<string>();

    // Try to read the template file
    ifstream in(templatePath + "templates.txt", ios::in | ios::binary);
#if HAVE_OSX
    if (!in)
    {
        templatePath = "../Resources/";
        in = ifstream(templatePath + "templates.txt", ios::in | ios::binary);
    }
#endif
    if (in)
    {
        auto newTemplate = false;
        auto templateName = string();
        auto templateDescription = string();
        auto endTemplate = false;

        for (array<char, 256> line; in.getline(&line[0], 256);)
        {
            auto strLine = string(line.data());
            if (!newTemplate)
            {
                if (strLine == "{")
                {
                    newTemplate = true;
                    endTemplate = false;
                    templateName = "";
                    templateDescription = "";
                }
            }
            else
            {
                if (strLine == "}")
                    endTemplate = true;
                else if (templateName == "")
                    templateName = strLine;
                else
                    templateDescription = strLine;
            }

            if (endTemplate)
            {
                examples.push_back(templateName);
                descriptions.push_back(templateDescription);
                templateName = "";
                templateDescription = "";
                newTemplate = false;
            }
        }
    }
    else
    {
        Log::get() << Log::WARNING << "GuiTemplate::" << __FUNCTION__ << " - Could not load the templates file list in " << templatePath << "templates.txt" << Log::endl;
        return;
    }

    _textures.clear();
    _descriptions.clear();

    for (unsigned int i = 0; i < examples.size(); ++i)
    {
        auto& example = examples[i];
        auto& description = descriptions[i];

        glGetError();
        auto image = make_shared<Image>(_scene);
        image->setName("template_" + example);
        if (!image->read(templatePath + "templates/" + example + ".png"))
            continue;

        auto texture = make_shared<Texture_Image>(_scene);
        texture->linkTo(image);
        texture->update();
        texture->flushPbo();

        _names.push_back(example);
        _textures[example] = texture;
        _descriptions[example] = description;
    }
}

} // end of namespace
