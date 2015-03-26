#include "texture_syphon.h"

#include "log.h"

using namespace std;

namespace Splash
{

/**************/
Texture_Syphon::Texture_Syphon()
{
    registerAttributes();
}

/**************/
Texture_Syphon::~Texture_Syphon()
{
    if (_syphonReceiver.isConnected())
        _syphonReceiver.disconnect();
}

/**************/
void Texture_Syphon::bind()
{
    if (_syphonReceiver.isConnected())
    {
        glGetIntegerv(GL_ACTIVE_TEXTURE, &_activeTexture);
        auto frameId = _syphonReceiver.getFrame();
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, frameId);
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
void Texture_Syphon::registerAttributes()
{
    _attribFunctions["connect"] = AttributeFunctor([&](const Values& args) {
        if (args.size() == 0)
        {
            _serverName = "";
            _appName = "";
        }
        else if (args.size() == 1 && args[0].asValues().size() == 2 && args[0].asValues()[0].asString() == "servername")
        {
            _serverName = args[0].asValues()[1].asString();
            _appName = "";
        }
        else if (args.size() == 1 && args[0].asValues().size() == 2 && args[0].asValues()[0].asString() == "appname")
        {
            _serverName = "";
            _appName = args[0].asValues()[1].asString();
        }
        else if (args.size() == 2 && args[0].asValues().size() == 2 && args[1].asValues().size() == 2)
        {
            _serverName = args[0].asValues()[1].asString();
            _appName = args[1].asValues()[1].asString();
        }

        if (!_syphonReceiver.connect(_serverName.c_str(), _appName.c_str()))
        {
            SLog::log << Log::WARNING << "Texture_Syphon::connect - Could not connect to the specified syphon source (servername: " << _serverName << ", appname: " << _appName << ")" << Log::endl;
            return false;
        }
        else
        {
            SLog::log << Log::MESSAGE << "Texture_Syphon::connect - Connected to the specified syphon source (servername: " << _serverName << ", appname: " << _appName << ")" << Log::endl;
        }
        return true;
    }, [&]() -> Values {
        Values v;
        v.push_back(Values({"servername", _serverName}));
        v.push_back(Values({"appname", _appName}));
        return v;
    });
}

} // end of namespace
