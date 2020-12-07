#include "./graphics/filter_black_level.h"

namespace Splash
{

/*************/
FilterBlackLevel::FilterBlackLevel(RootObject* root)
    : Filter(root)
{
    _type = "filter_black_level";

    registerDefaultShaderAttributes();
}

/*************/
void FilterBlackLevel::render()
{
    _screen->setAttribute("fill", {"blacklevel_filter"});
    Filter::render();

    // Automatic black level stuff
    if (_autoBlackLevelTargetValue != 0.f)
    {
        auto luminance = _fbo->getColorTexture()->getMeanValue().luminance();
        auto deltaLuminance = _autoBlackLevelTargetValue - luminance;
        auto newBlackLevel = _autoBlackLevel + deltaLuminance / 2.f;

        auto currentTime = Timer::getTime() / 1000;
        auto deltaT = _previousTime == 0 ? 0.f : static_cast<float>((currentTime - _previousTime) / 1e3);
        _previousTime = currentTime;

        if (deltaT != 0.f)
        {
            auto blackLevelProgress = std::min(1.f, deltaT / _autoBlackLevelSpeed); // Limit to 1.f, otherwise the black level resonates
            newBlackLevel = std::min(_autoBlackLevelTargetValue, std::max(0.f, newBlackLevel));
            _autoBlackLevel = newBlackLevel * blackLevelProgress + _autoBlackLevel * (1.f - blackLevelProgress);
            _filterUniforms["_blackLevel"] = {_autoBlackLevel / 255.0};
        }
    }
}

/*************/
void FilterBlackLevel::registerDefaultShaderAttributes()
{
    addAttribute(
        "blackLevel",
        [&](const Values& args) {
            auto blackLevel = std::max(0.f, std::min(255.f, args[0].as<float>()));
            _filterUniforms["_blackLevel"] = {blackLevel / 255.f};
            return true;
        },
        [&]() -> Values {
            auto it = _filterUniforms.find("_blackLevel");
            if (it == _filterUniforms.end())
                _filterUniforms["_blackLevel"] = {0.f}; // Default value
            return {_filterUniforms["_blackLevel"][0].as<float>() * 255.f};
        },
        {'r'});
    setAttributeDescription("blackLevel", "Set the black level for the linked texture, between 0 and 255");

    addAttribute(
        "blackLevelAuto",
        [&](const Values& args) {
            _autoBlackLevelTargetValue = std::min(255.f, std::max(0.f, args[0].as<float>()));
            _autoBlackLevelSpeed = std::max(0.f, args[1].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_autoBlackLevelTargetValue, _autoBlackLevelSpeed};
        },
        {'r', 'r'});
    setAttributeDescription("blackLevelAuto",
        "If the first parameter is not zero, automatic black level is enabled.\n"
        "The first parameter is the black level value (between 0 and 255) to match if needed.\n"
        "The second parameter is the maximum time to match the black level, in seconds.\n"
        "The black level will be updated so that the minimum overall luminance matches the target.");
}

} // namespace Splash
