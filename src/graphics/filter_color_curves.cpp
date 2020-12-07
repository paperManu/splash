#include "./graphics/filter_color_curves.h"

namespace Splash
{

/*************/
FilterColorCurves::FilterColorCurves(RootObject* root)
    : Filter(root)
{
    _type = "filter_color_curves";
}

/*************/
void FilterColorCurves::updateShaderParameters()
{
    if (!_colorCurves.empty()) // Validity of color curve has been checked earlier
        _screen->setAttribute("fill", {"color_curves_filter", "COLOR_CURVE_COUNT " + std::to_string(static_cast<int>(_colorCurves[0].size()))});

    // This is a trick to force the shader compilation
    _screen->activate();
    _screen->deactivate();
}

/*************/
void FilterColorCurves::updateUniforms()
{
    Filter::updateUniforms();
    auto shader = _screen->getShader();

    if (!_colorCurves.empty())
    {
        Values tmpCurves;
        for (uint32_t i = 0; i < _colorCurves[0].size(); ++i)
            for (uint32_t j = 0; j < _colorCurves.size(); ++j)
                tmpCurves.push_back(_colorCurves[j][i].as<float>());
        Values curves;
        curves.push_back(tmpCurves);
        shader->setAttribute("uniform", {"_colorCurves", curves});
    }
}

/*************/
void FilterColorCurves::registerDefaultShaderAttributes()
{
    addAttribute(
        "colorCurves",
        [&](const Values& args) {
            uint32_t pointCount = 0;
            for (auto& v : args)
                if (pointCount == 0)
                    pointCount = v.size();
                else if (pointCount != v.size())
                    return false;

            if (pointCount < 2)
                return false;

            addTask([=]() {
                _colorCurves = args;
                updateShaderParameters();
            });
            return true;
        },
        [&]() -> Values { return _colorCurves; },
        {'v', 'v', 'v'});

    addAttribute(
        "colorCurveAnchors",
        [&](const Values& args) {
            auto count = args[0].as<uint32_t>();

            if (count < 2)
                return false;
            if (!_colorCurves.empty() && _colorCurves[0].size() == count)
                return true;

            Values linearCurve;
            for (uint32_t i = 0; i < count; ++i)
                linearCurve.push_back(static_cast<float>(i) / (static_cast<float>(count - 1)));

            addTask([=]() {
                _colorCurves.clear();
                for (uint32_t i = 0; i < 3; ++i)
                    _colorCurves.push_back(linearCurve);
                updateShaderParameters();
            });
            return true;
        },
        [&]() -> Values {
            if (_colorCurves.empty())
                return {0};
            else
                return {_colorCurves[0].size()};
        },
        {'i'});
}

} // namespace Splash
