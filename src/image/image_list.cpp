#include "./image_list.h"

#include <algorithm>
#include <filesystem>

#include "./utils/osutils.h"

namespace Splash
{

/*************/
Image_List::Image_List(RootObject* root)
    : Image_Sequence(root)
{
    init();
}

/*************/
void Image_List::init()
{
    _type = "image_list";
    registerAttributes();
}

/*************/
bool Image_List::read(const std::string& dirname)
{
    if (!std::filesystem::is_directory(dirname))
        return false;

    // read the files from a directory
    for (const auto& it : std::filesystem::directory_iterator(dirname))
    {
        auto path = it.path();
        if (!path.has_extension())
            return false;

        std::string extension = path.extension();
        Utils::toLower(extension);
        if (extension == ".jpg" || extension == ".png")
            _filenameSeq.push_back(std::filesystem::absolute(path));
    }

    // sort in descending order
    std::sort(_filenameSeq.begin(), _filenameSeq.end(), std::greater<std::string>());
    return true;
}

/*************/
bool Image_List::capture()
{
    if (_filenameSeq.empty())
        return false;

    readFile(_filenameSeq.back());
    _filenameSeq.pop_back();
    return true;
}

void Image_List::registerAttributes()
{
    Image_Sequence::registerAttributes();

    // Fake shutterspeed attribute
    addAttribute(
        "shutterspeed",
        [&](const Values& args) {
            _shutterspeed = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_shutterspeed}; },
        {'r'});
    setAttributeDescription("shutterspeed", "Set the fake shutter speed");

    // Status
    addAttribute("ready", [&]() -> Values {
        if (_filenameSeq.size() == 0)
            return {false};
        else
            return {true};
    });
    setAttributeDescription("ready", "Ask whether the image sequence is ready to capture");
}

} // namespace Splash
