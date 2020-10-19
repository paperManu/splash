#include "./graphics/filter.h"

#include "./core/scene.h"
#include "./graphics/camera.h"
#include "./graphics/texture_image.h"
#include "./utils/cgutils.h"
#include "./utils/log.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{

/*************/
Filter::Filter(RootObject* root)
    : Texture(root)
{
    _type = "filter";
    _renderingPriority = Priority::FILTER;
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    _fbo = make_unique<Framebuffer>(_root);
    _fbo->getColorTexture()->setAttribute("filtering", {true});
    _fbo->setSixteenBpc(_sixteenBpc);

    // Setup the virtual screen
    _screen = make_shared<Object>(_root);
    _screen->setAttribute("fill", {"image_filter"});
    auto virtualScreen = make_shared<Geometry>(_root);
    _screen->addGeometry(virtualScreen);
}

/*************/
void Filter::bind()
{
    _fbo->getColorTexture()->bind();
}

/*************/
unordered_map<string, Values> Filter::getShaderUniforms() const
{
    unordered_map<string, Values> uniforms;
    uniforms["size"] = {static_cast<float>(_fbo->getColorTexture()->getSpec().width), static_cast<float>(_fbo->getColorTexture()->getSpec().height)};
    return uniforms;
}

/*************/
bool Filter::linkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (!obj)
        return false;

    if (dynamic_pointer_cast<Texture>(obj))
    {
        if (!_inTextures.empty() && _inTextures[_inTextures.size() - 1].expired())
            _screen->removeTexture(_inTextures[_inTextures.size() - 1].lock());

        auto tex = dynamic_pointer_cast<Texture>(obj);
        _screen->addTexture(tex);
        _inTextures.push_back(tex);
        _sizeOverride[0] = -1;
        _sizeOverride[1] = -1;

        return true;
    }
    else if (dynamic_pointer_cast<Image>(obj))
    {
        auto tex = dynamic_pointer_cast<Texture_Image>(_root->createObject("texture_image", getName() + "_" + obj->getName() + "_tex").lock());
        if (tex->linkTo(obj))
            return linkTo(tex);
        else
            return false;
    }
    else if (dynamic_pointer_cast<Camera>(obj).get())
    {
        auto cam = dynamic_pointer_cast<Camera>(obj).get();
        auto tex = cam->getTexture();
        return linkTo(tex);
    }

    return false;
}

/*************/
void Filter::unbind()
{
    _fbo->getColorTexture()->unbind();
}

/*************/
void Filter::unlinkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (dynamic_pointer_cast<Texture>(obj).get())
    {
        for (uint32_t i = 0; i < _inTextures.size();)
        {
            if (_inTextures[i].expired())
                continue;

            auto inTex = _inTextures[i].lock();
            auto tex = dynamic_pointer_cast<Texture>(obj);
            if (inTex == tex)
            {
                _screen->removeTexture(tex);
                _inTextures.erase(_inTextures.begin() + i);
            }
            else
            {
                ++i;
            }
        }
    }
    else if (dynamic_pointer_cast<Image>(obj).get())
    {
        auto textureName = getName() + "_" + obj->getName() + "_tex";

        if (auto tex = _root->getObject(textureName))
        {
            tex->unlinkFrom(obj);
            unlinkFrom(tex);
        }

        _root->disposeObject(textureName);
    }
    else if (dynamic_pointer_cast<Camera>(obj).get())
    {
        auto cam = dynamic_pointer_cast<Camera>(obj);
        auto tex = cam->getTexture();
        unlinkFrom(tex);
    }
}

/*************/
void Filter::setKeepRatio(bool keepRatio)
{
    if (keepRatio == _keepRatio)
        return;

    _keepRatio = keepRatio;
    updateSizeWrtRatio();
}

/*************/
void Filter::setSixteenBpc(bool active)
{
    _sixteenBpc = active;
    if (!_fbo)
        return;

    _fbo->setSixteenBpc(active);
}

/*************/
void Filter::updateSizeWrtRatio()
{
    if (_keepRatio && (_sizeOverride[0] || _sizeOverride[1]))
    {
        auto inputSpec = _inTextures[0].lock()->getSpec();

        float ratio = static_cast<float>(inputSpec.width) / static_cast<float>(inputSpec.height);
        ratio = ratio != 0.f ? ratio : 1.f;

        if (_sizeOverride[0] > _sizeOverride[1])
            _sizeOverride[1] = static_cast<int>(_sizeOverride[0] / ratio);
        else
            _sizeOverride[0] = static_cast<int>(_sizeOverride[1] * ratio);
    }
}

/*************/
void Filter::render()
{
    // Intialize FBO, textures and everything OpenGL
    if (!_shaderAttributesRegistered)
    {
        _shaderAttributesRegistered = true;
        registerDefaultShaderAttributes();
    }

    if (!_inTextures.empty() && !_inTextures[0].expired())
    {
        auto input = _inTextures[0].lock();
        auto inputSpec = input->getSpec();

        if (inputSpec != _spec || (_sizeOverride[0] > 0 && _sizeOverride[1] > 0))
        {
            auto newOutTextureSpec = inputSpec;
            if (_sizeOverride[0] > 0 || _sizeOverride[1] > 0)
            {
                updateSizeWrtRatio();
                newOutTextureSpec.width = _sizeOverride[0] ? _sizeOverride[0] : _sizeOverride[1];
                newOutTextureSpec.height = _sizeOverride[1] ? _sizeOverride[1] : _sizeOverride[0];
            }

            if (_spec != newOutTextureSpec)
            {
                _spec = newOutTextureSpec;
                _fbo->setSize(_spec.width, _spec.height);
            }
        }
    }
    else
    {
        if (_sizeOverride[0] == -1 && _sizeOverride[1] == -1)
        {
            _sizeOverride[0] = _defaultSize[0];
            _sizeOverride[1] = _defaultSize[1];
        }

        if (_sizeOverride[0] != static_cast<int>(_spec.width) || _sizeOverride[1] != static_cast<int>(_spec.height))
        {
            _spec.width = _sizeOverride[0];
            _spec.height = _sizeOverride[1];
            _fbo->setSize(_spec.width, _spec.height);
        }
    }

    // Update the timestamp to the latest from all input textures
    int64_t timestamp{0};
    for (const auto& texture : _inTextures)
    {
        auto texturePtr = texture.lock();
        if (!texturePtr)
            continue;
        timestamp = std::max(timestamp, texturePtr->getTimestamp());
    }
    _spec.timestamp = timestamp;

    _fbo->bindDraw();
    glViewport(0, 0, _spec.width, _spec.height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _screen->activate();
    updateUniforms();
    _screen->draw();
    _screen->deactivate();

    _fbo->unbindDraw();

    _fbo->getColorTexture()->generateMipmap();
    if (_grabMipmapLevel >= 0)
    {
        auto colorTexture = _fbo->getColorTexture();
        _mipmapBuffer = colorTexture->grabMipmap(_grabMipmapLevel).getRawBuffer();
        auto spec = colorTexture->getSpec();
        _mipmapBufferSpec = {spec.width, spec.height, spec.channels, spec.bpp, spec.format};
    }
}

/*************/
void Filter::updateUniforms()
{
    auto shader = _screen->getShader();

    // Built-in uniforms
    _filterUniforms["_time"] = {static_cast<int>(Timer::getTime() / 1000)};
    _filterUniforms["_resolution"] = {static_cast<float>(_spec.width), static_cast<float>(_spec.height)};

    int64_t masterClock;
    bool paused;
    if (Timer::get().getMasterClock<chrono::milliseconds>(masterClock, paused))
        _filterUniforms["_clock"] = {static_cast<int>(masterClock)};

    // Update generic uniforms
    for (auto& weakObject : _linkedObjects)
    {
        auto obj = weakObject.lock();
        if (!obj)
            continue;

        if (obj->getType() == "image")
        {
            Values remainingTime, duration;
            obj->getAttribute("duration", duration);
            obj->getAttribute("remaining", remainingTime);
            if (remainingTime.size() == 1)
                shader->setAttribute("uniform", {"_filmRemaining", remainingTime[0].as<float>()});
            if (duration.size() == 1)
                shader->setAttribute("uniform", {"_filmDuration", duration[0].as<float>()});
        }
    }

    // Update uniforms specific to the current filtering shader
    for (auto& uniform : _filterUniforms)
    {
        Values param;
        param.push_back(uniform.first);
        for (auto& v : uniform.second)
            param.push_back(v);
        shader->setAttribute("uniform", param);
    }
}

/*************/
void Filter::registerAttributes()
{
    Texture::registerAttributes();

    addAttribute(
        "size",
        [&](const Values&) { return true; },
        [&]() -> Values {
            if (_inTextures.empty())
                return {0, 0};

            auto texture = _inTextures[0].lock();
            if (!texture)
                return {0, 0};

            auto inputSpec = texture->getSpec();
            return {inputSpec.width, inputSpec.height};
        },
        {});
    setAttributeDescription("size", "Size of the input texture");

    addAttribute(
        "sizeOverride",
        [&](const Values& args) {
            auto width = args[0].as<int>();
            auto height = args[1].as<int>();
            addTask([=]() {
                _sizeOverride[0] = width;
                _sizeOverride[1] = height;
            });
            return true;
        },
        [&]() -> Values {
            return {_sizeOverride[0], _sizeOverride[1]};
        },
        {'i', 'i'});
    setAttributeDescription("sizeOverride", "Sets the filter output to a different resolution than its input");

    //
    // Mipmap capture
    addAttribute(
        "grabMipmapLevel",
        [&](const Values& args) {
            _grabMipmapLevel = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {_grabMipmapLevel}; },
        {'i'});
    setAttributeDescription("grabMipmapLevel", "If set to 0 or superior, sync the rendered texture to the tree, at the given mipmap level");

    addAttribute(
        "buffer", [&](const Values&) { return true; }, [&]() -> Values { return {_mipmapBuffer}; }, {});
    setAttributeDescription("buffer", "Getter attribute which gives access to the mipmap image, if grabMipmapLevel is greater or equal to 0");

    addAttribute(
        "bufferSpec", [&](const Values&) { return true; }, [&]() -> Values { return _mipmapBufferSpec; }, {});
    setAttributeDescription("bufferSpec", "Getter attribute to the specs of the attribute buffer");
}

/*************/
void Filter::registerDefaultShaderAttributes()
{
    addAttribute("brightness",
        [&](const Values& args) {
            auto brightness = args[0].as<float>();
            brightness = std::max(0.f, std::min(2.f, brightness));
            _filterUniforms["_brightness"] = {brightness};
            return true;
        },
        [&]() -> Values {
            auto it = _filterUniforms.find("_brightness");
            if (it == _filterUniforms.end())
                _filterUniforms["_brightness"] = {1.f}; // Default value
            return _filterUniforms["_brightness"];
        },
        {'r'});
    setAttributeDescription("brightness", "Set the brightness for the linked texture");

    addAttribute("contrast",
        [&](const Values& args) {
            auto contrast = args[0].as<float>();
            contrast = std::max(0.f, std::min(2.f, contrast));
            _filterUniforms["_contrast"] = {contrast};
            return true;
        },
        [&]() -> Values {
            auto it = _filterUniforms.find("_contrast");
            if (it == _filterUniforms.end())
                _filterUniforms["_contrast"] = {1.f}; // Default value
            return _filterUniforms["_contrast"];
        },
        {'r'});
    setAttributeDescription("contrast", "Set the contrast for the linked texture");

    addAttribute("colorTemperature",
        [&](const Values& args) {
            auto colorTemperature = args[0].as<float>();
            colorTemperature = std::max(0.f, std::min(16000.f, colorTemperature));
            _filterUniforms["_colorTemperature"] = {colorTemperature};
            auto colorBalance = colorBalanceFromTemperature(colorTemperature);
            _filterUniforms["_colorBalance"] = {colorBalance.x, colorBalance.y};
            return true;
        },
        [&]() -> Values {
            auto it = _filterUniforms.find("_colorTemperature");
            if (it == _filterUniforms.end())
                _filterUniforms["_colorTemperature"] = {6500.f}; // Default value
            return _filterUniforms["_colorTemperature"];
        },
        {'r'});
    setAttributeDescription("colorTemperature", "Set the color temperature correction for the linked texture");

    addAttribute("invertChannels",
        [&](const Values& args) {
            auto enable = args[0].as<bool>();
            _filterUniforms["_invertChannels"] = {enable};
            return true;
        },
        [&]() -> Values {
            auto it = _filterUniforms.find("_invertChannels");
            if (it == _filterUniforms.end())
                _filterUniforms["_invertChannels"] = {true};
            return _filterUniforms["_invertChannels"];
        },
        {'b'});
    setAttributeDescription("invertChannels", "Invert red and blue channels");

    addAttribute("keepRatio",
        [&](const Values& args) {
            setKeepRatio(args[0].as<bool>());
            return true;
        },
        [&]() -> Values { return {_keepRatio}; },
        {'b'});
    setAttributeDescription("keepRatio", "If true, keeps the ratio of the input image");

    addAttribute("saturation",
        [&](const Values& args) {
            auto saturation = args[0].as<float>();
            saturation = std::max(0.f, std::min(2.f, saturation));
            _filterUniforms["_saturation"] = {saturation};
            return true;
        },
        [&]() -> Values {
            auto it = _filterUniforms.find("_saturation");
            if (it == _filterUniforms.end())
                _filterUniforms["_saturation"] = {1.f}; // Default value
            return _filterUniforms["_saturation"];
        },
        {'r'});
    setAttributeDescription("saturation", "Set the saturation for the linked texture");

    addAttribute("scale",
        [&](const Values& args) {
            auto scale_x = args[0].as<float>();
            auto scale_y = args[1].as<float>();
            _filterUniforms["_scale"] = {scale_x, scale_y};
            return true;
        },
        [&]() -> Values {
            auto it = _filterUniforms.find("_scale");
            if (it == _filterUniforms.end())
                _filterUniforms["_scale"] = {1.0, 1.0}; // Default value
            return _filterUniforms["_scale"];
        },
        {'r', 'r'});
    setAttributeDescription("scale", "Set the scaling of the texture along both axes");
}

} // namespace Splash
