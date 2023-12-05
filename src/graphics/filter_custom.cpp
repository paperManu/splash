#include "./graphics/filter_custom.h"

#include "./graphics/api/shader_gfx_impl.h"

namespace Splash
{

/*************/
FilterCustom::FilterCustom(RootObject* root)
    : Filter(root)
{
    _type = "filter_custom";
    registerAttributes();
}

/*************/
bool FilterCustom::setFilterSource(const std::string& source)
{
    auto shader = std::make_shared<Shader>(_root);
    // Save the value for all existing uniforms
    auto uniformValues = _filterUniforms;

    std::map<gfx::ShaderType, std::string> shaderSources;
    shaderSources[gfx::ShaderType::fragment] = source;
    if (!shader->setSource(shaderSources))
    {
        Log::get() << Log::WARNING << "Filter::" << __FUNCTION__ << " - Could not apply shader filter" << Log::endl;
        return false;
    }
    Log::get() << Log::MESSAGE << "Filter::" << __FUNCTION__ << " - Shader filter updated" << Log::endl;
    _screen->setShader(shader);

    // This is a trick to force the shader compilation
    _screen->activate();
    _screen->deactivate();

    // Unregister previously added uniforms
    // We remove the associated attribute fonction if it exists
    for (const auto& uniform : _filterUniforms)
    {
        auto uniformName = uniform.first;
        assert(uniformName.length() != 0);

        // Uniforms for the default shader start with an underscore
        if (uniformName[0] == '_')
            uniformName = std::string(uniformName, 1, uniformName.length() - 1);
        removeAttribute(uniformName);
    }
    _filterUniforms.clear();

    // Register the attributes corresponding to the shader uniforms
    auto uniforms = shader->getUniforms();
    auto uniformsDocumentation = shader->getUniformsDocumentation();

    for (const auto& u : uniforms)
    {
        // Uniforms starting with a underscore are kept hidden
        if (u.first.empty() || u.first[0] == '_')
            continue;

        std::vector<char> types;
        for (auto& v : u.second)
            types.push_back(v.getTypeAsChar());

        _filterUniforms[u.first] = u.second;
        addAttribute(
            u.first,
            [=](const Values& args) {
                _filterUniforms[u.first] = args;
                return true;
            },
            [=]() -> Values { return _filterUniforms[u.first]; },
            types);

        auto documentation = uniformsDocumentation.find(u.first);
        if (documentation != uniformsDocumentation.end())
            setAttributeDescription(u.first, documentation->second);

        // Reset the value if this uniform already existed
        auto uniformValueIt = uniformValues.find(u.first);
        if (uniformValueIt != uniformValues.end())
            setAttribute(u.first, uniformValueIt->second);
    }

    return true;
}

/*************/
void FilterCustom::registerAttributes()
{
    Filter::registerAttributes();

    addAttribute(
        "filterSource",
        [&](const Values& args) {
            auto src = args[0].as<std::string>();
            if (src.empty())
                return true; // No shader specified
            _shaderSource = src;
            _shaderSourceFile = "";
            addTask([=]() { setFilterSource(src); });
            return true;
        },
        [&]() -> Values { return {_shaderSource}; },
        {'s'});
    setAttributeDescription("filterSource", "Set the fragment shader source for the filter");

    addAttribute(
        "fileFilterSource",
        [&](const Values& args) {
            const auto srcFile = args[0].as<std::string>();
            if (srcFile.empty())
                return true; // No shader specified

            std::ifstream in(srcFile, std::ios::in | std::ios::binary);
            if (in)
            {
                std::string contents;
                in.seekg(0, std::ios::end);
                contents.resize(in.tellg());
                in.seekg(0, std::ios::beg);
                in.read(&contents[0], contents.size());
                in.close();

                _shaderSourceFile = srcFile;
                _shaderSource = "";
                addTask([=]() {
                    setFilterSource(contents);
                    _lastShaderSourceRead = Timer::getTime();
                });

                return true;
            }
            else
            {
                Log::get() << Log::WARNING << __FUNCTION__ << " - Unable to load file " << srcFile << Log::endl;
                return false;
            }
        },
        [&]() -> Values { return {_shaderSourceFile}; },
        {'s'});
    setAttributeDescription("fileFilterSource", "Set the fragment shader source for the filter from a file");

    addAttribute("shaderReadTime", [&]() -> Values { return {_lastShaderSourceRead}; });
    setAttributeDescription("shaderReadTime", "Time at which the shader was read");

    addAttribute(
        "watchShaderFile",
        [&](const Values& args) {
            _watchShaderFile = args[0].as<bool>();

            if (_watchShaderFile)
            {
                addPeriodicTask(
                    "watchShader",
                    [=]() {
                        if (_shaderSourceFile.empty())
                            return;

                        std::filesystem::path sourcePath(_shaderSourceFile);
                        try
                        {
                            const auto lastWriteTime = std::filesystem::last_write_time(sourcePath);
                            if (lastWriteTime != _lastShaderSourceWrite)
                            {
                                _lastShaderSourceWrite = lastWriteTime;
                                setAttribute("fileFilterSource", {_shaderSourceFile});
                            }
                        }
                        catch (...)
                        {
                        }
                    },
                    500);
            }
            else
            {
                removePeriodicTask("watchShader");
            }

            return true;
        },
        [&]() -> Values { return {_watchShaderFile}; },
        {'b'});
    setAttributeDescription("watchShaderFile", "If true, automatically updates the shader from the source file");
}

} // namespace Splash
