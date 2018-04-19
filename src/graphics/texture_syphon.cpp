#include "./graphics/texture_syphon.h"

#include "./utils/log.h"

using namespace std;

namespace Splash
{

/**************/
Texture_Syphon::Texture_Syphon(RootObject* root)
    : Texture(root)
{
    init();
}

/**************/
void Texture_Syphon::init()
{
    _type = "texture_syphon";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;
}

/**************/
Texture_Syphon::~Texture_Syphon()
{
    if (_syphonReceiver.isConnected())
        _syphonReceiver.disconnect();
}

/**************/
bool Texture_Syphon::linkTo(const shared_ptr<BaseObject>& obj)
{
    // Mandatory before trying to link
    return Texture::linkTo(obj);
}

/**************/
void Texture_Syphon::bind()
{
    if (_syphonReceiver.isConnected())
    {
        _shaderUniforms.clear();
        _shaderUniforms["size"] = {(float)_syphonReceiver.getWidth(), (float)_syphonReceiver.getHeight()};

        glGetIntegerv(GL_ACTIVE_TEXTURE, &_activeTexture);
        auto frameId = _syphonReceiver.getFrame();
        if (frameId != -1)
            glBindTexture(GL_TEXTURE_RECTANGLE, frameId);
    }
}

/**************/
void Texture_Syphon::unbind()
{
    if (_syphonReceiver.isConnected())
    {
#ifdef DEBUG
        glActiveTexture((GLenum)_activeTexture);
        glBindTexture(GL_TEXTURE_2D, 0);
#endif
        _syphonReceiver.releaseFrame();
    }
}

/**************/
GLuint Texture_Syphon::getTexId() const
{
    if (_syphonReceiver.isConnected())
    {
        auto frameId = _syphonReceiver.getFrame();
        return frameId;
    }

    return 0;
}

/**************/
void Texture_Syphon::registerAttributes()
{
    Texture::registerAttributes();

    addAttribute("connect",
        [&](const Values& args) {
            if (args.size() == 0)
            {
                _serverName = "";
                _appName = "";
            }
            else if (args.size() == 1 && args[0].as<Values>().size() == 2 && args[0].as<Values>()[0].as<string>() == "servername")
            {
                _serverName = args[0].as<Values>()[1].as<string>();
                _appName = "";
            }
            else if (args.size() == 1 && args[0].as<Values>().size() == 2 && args[0].as<Values>()[0].as<string>() == "appname")
            {
                _serverName = "";
                _appName = args[0].as<Values>()[1].as<string>();
            }
            else if (args.size() == 2 && args[0].as<Values>().size() == 2 && args[1].as<Values>().size() == 2)
            {
                _serverName = args[0].as<Values>()[1].as<string>();
                _appName = args[1].as<Values>()[1].as<string>();
            }

            if (!_syphonReceiver.connect(_serverName.c_str(), _appName.c_str()))
            {
                Log::get() << Log::WARNING << "Texture_Syphon::connect - Could not connect to the specified syphon source (servername: " << _serverName << ", appname: " << _appName
                           << ")" << Log::endl;
                return false;
            }
            else
            {
                Log::get() << Log::MESSAGE << "Texture_Syphon::connect - Connected to the specified syphon source (servername: " << _serverName << ", appname: " << _appName << ")"
                           << Log::endl;
            }
            return true;
        },
        [&]() -> Values {
            Values v;
            v.push_back(Values({"servername", _serverName}));
            v.push_back(Values({"appname", _appName}));
            return v;
        });
    setAttributeDescription("connect", "Try to connect to the Syphon server, given its server name and application name");
}

} // end of namespace
