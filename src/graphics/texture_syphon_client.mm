#include "./graphics/texture_syphon_client.h"

#import <iostream>
#import <Syphon/Syphon.h>

using namespace std;

namespace Splash
{

/**************/
SyphonReceiver::SyphonReceiver() :
    _syphonClient(nullptr),
    _syphonImage(nullptr),
    _sharedDirectory(nullptr),
    _width(1), _height(1)
{
}

/**************/
SyphonReceiver::~SyphonReceiver()
{
    disconnect();
    if (_sharedDirectory)
        [(SyphonServerDirectory*)_sharedDirectory release];
}

/**************/
bool SyphonReceiver::connect(const char* serverName, const char* appName)
{
    bool result = true;

    SyphonServerDirectory* sharedDirectory = (SyphonServerDirectory*)[SyphonServerDirectory sharedDirectory];
    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];

    SyphonClient* newClient = nullptr;
    SyphonClient* client = (SyphonClient*)_syphonClient;

    NSArray* serverMatches;
    if (strlen(serverName) != 0 && strlen(appName) == 0)
        serverMatches = [sharedDirectory serversMatchingName:[NSString stringWithCString:serverName] appName:nil]; 
    else if (strlen(serverName) == 0 && strlen(appName) != 0)
        serverMatches = [sharedDirectory serversMatchingName:nil appName:[NSString stringWithCString:appName]]; 
    else if (strlen(serverName) != 0 && strlen(appName) != 0)
        serverMatches = [sharedDirectory serversMatchingName:[NSString stringWithCString:serverName] appName:[NSString stringWithCString:appName]]; 
    else
        serverMatches = [sharedDirectory serversMatchingName:nil appName:nil]; 

    if ([serverMatches count] != 0)
    {
        NSString *currentServer, *foundServer;
        foundServer = [[serverMatches lastObject] objectForKey:SyphonServerDescriptionUUIDKey];
        currentServer = [client.serverDescription objectForKey:SyphonServerDescriptionUUIDKey];

        if (foundServer && [currentServer isEqualToString:foundServer])
            newClient = [client retain];
        else
            newClient = [[SyphonClient alloc] initWithServerDescription:[serverMatches lastObject] options:nil newFrameHandler:nil];
    }
    else
        result = false;

    if (client)
        [client release];
    _syphonClient = newClient;

    return result;
}

/**************/
void SyphonReceiver::disconnect()
{
    if (_syphonClient)
        [(SyphonClient*)_syphonClient stop];
}

/**************/
int SyphonReceiver::getFrame()
{
    _syphonImage = (void*)[(SyphonClient*)_syphonClient newFrameImageForContext:CGLGetCurrentContext()];

    if (_syphonImage)
    {
        NSSize imageSize = [(SyphonImage*)_syphonImage textureSize];
        _width = imageSize.width;
        _height = imageSize.height;

        return (int)[(SyphonImage*)_syphonImage textureName];
    }
    else
        return -1;
}

/**************/
void SyphonReceiver::releaseFrame()
{
    if (_syphonImage)
        [(SyphonImage*)_syphonImage release];
}

} // end of namespace
