#include "camera.h"
#include "timer.h"

#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define SPLASH_SCISSOR_WIDTH 8

using namespace std;
using namespace glm;

namespace Splash {

/*************/
Camera::Camera(GlWindowPtr w)
{
    _type = "camera";

    if (w.get() == nullptr)
        return;

    _window = w;

    // Intialize FBO, textures and everything OpenGL
    glfwMakeContextCurrent(_window->get());
    glGetError();
    glGenFramebuffers(1, &_fbo);
    glfwMakeContextCurrent(NULL);

    setOutputNbr(1);

    glfwMakeContextCurrent(_window->get());
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    GLenum _status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (_status != GL_FRAMEBUFFER_COMPLETE)
        SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - Error while initializing framebuffer object: " << _status << Log::endl;
    else
        SLog::log << Log::MESSAGE << "Camera::" << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLenum error = glGetError();
    if (error)
    {
        SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - Error while binding framebuffer" << Log::endl;
        _isInitialized = false;
    }
    else
    {
        SLog::log << Log::MESSAGE << "Camera::" << __FUNCTION__ << " - Camera correctly initialized" << Log::endl;
        _isInitialized = true;
    }

    // Load some models
    loadDefaultModels();

    glfwMakeContextCurrent(NULL);

    registerAttributes();
}

/*************/
Camera::~Camera()
{
    SLog::log<< Log::DEBUG << "Camera::~Camera - Destructor" << Log::endl;
}

/*************/
void Camera::addObject(ObjectPtr& obj)
{
    _objects.push_back(obj);
}

/*************/
bool Camera::linkTo(BaseObjectPtr obj)
{
    if (dynamic_pointer_cast<Object>(obj).get() != nullptr)
    {
        ObjectPtr obj3D = dynamic_pointer_cast<Object>(obj);
        addObject(obj3D);
        return true;
    }

    return false;
}

/*************/
vector<Value> Camera::pickVertex(float x, float y)
{
    // Convert the normalized coordinates ([0, 1]) to pixel coordinates
    float realX = x * _width;
    float realY = (1.f - y) * _height;

    // Get the depth at the given point
    glfwMakeContextCurrent(_window->get());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
    float depth;
    glReadPixels(realX, realY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glfwMakeContextCurrent(NULL);

    if (depth == 1.f)
        return vector<Value>();

    // Unproject the point
    vec3 screenPoint(realX, realY, depth);

    float distance = numeric_limits<float>::max();
    vec3 vertex;
    for (auto& obj : _objects)
    {
        vec3 point = unProject(screenPoint, lookAt(_eye, _target, _up) * obj->getModelMatrix(), perspectiveFov(_fov, _width, _height, _near, _far), vec4(0, 0, _width, _height));
        glm::vec3 closestVertex;
        float tmpDist;
        if ((tmpDist = obj->pickVertex(point, closestVertex)) < distance)
        {
            distance = tmpDist;
            vertex = closestVertex;
        }
    }

    return vector<Value>({vertex.x, vertex.y, vertex.z});
}

/*************/
bool Camera::render()
{
    glfwMakeContextCurrent(_window->get());

    if (_outTextures.size() < 1)
        return false;

    glGetError();
    ImageSpec spec = _outTextures[0]->getSpec();
    if (spec.width != _width || spec.height != _height)
        setOutputSize(spec.width, spec.height);

    glViewport(0, 0, _width, _height);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    GLenum fboBuffers[_outTextures.size()];
    for (int i = 0; i < _outTextures.size(); ++i)
        fboBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
    glDrawBuffers(_outTextures.size(), fboBuffers);
    glEnable(GL_DEPTH_TEST);

    if (_drawFrame)
    {
        glClearColor(1.0, 0.5, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glEnable(GL_SCISSOR_TEST);
        glScissor(SPLASH_SCISSOR_WIDTH, SPLASH_SCISSOR_WIDTH, _width - SPLASH_SCISSOR_WIDTH * 2, _height - SPLASH_SCISSOR_WIDTH * 2);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the objects
    for (auto& obj : _objects)
    {
        obj->activate();
        obj->setViewProjectionMatrix(computeViewProjectionMatrix());
        obj->draw();
        obj->deactivate();
    }

    // Draw the calibration points
    for (auto& point : _calibrationPoints)
    {
        ObjectPtr marker = _models["3d_marker"];
        marker->setAttribute("position", {point.first.x, point.first.y, point.first.z});
        marker->activate();
        marker->setViewProjectionMatrix(computeViewProjectionMatrix());
        marker->draw();
        marker->deactivate();
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);

    // We need to regenerate the mipmaps for all the output textures
    glActiveTexture(GL_TEXTURE0);
    for (auto t : _outTextures)
        t->generateMipmap();

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    GLenum error = glGetError();
    if (error)
        SLog::log << Log::WARNING << _type << "::" << __FUNCTION__ << " - Error while rendering the camera: " << error << Log::endl;

    glfwMakeContextCurrent(NULL);

    return error != 0 ? true : false;
}

/*************/
bool Camera::setCalibrationPoint(std::vector<Value> worldPoint, std::vector<Value> screenPoint)
{
    if (worldPoint.size() < 3 || screenPoint.size() < 2)
        return false;

    vec3 world(worldPoint[0].asFloat(), worldPoint[1].asFloat(), worldPoint[2].asFloat());
    vec2 screen(screenPoint[0].asFloat() * _width, screenPoint[1].asFloat() * _height);

    // Check if the point is already present
    for (auto& point : _calibrationPoints)
        if (point.first == world)
        {
            point.second = screen;
            return true;
        }

    _calibrationPoints.push_back(pair<vec3, vec2>(world, screen));        
    return false;
}

/*************/
void Camera::setOutputNbr(int nbr)
{
    if (nbr < 1 || nbr == _outTextures.size())
        return;

    glfwMakeContextCurrent(_window->get());

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    if (!_depthTexture)
    {
        TexturePtr texture(new Texture);
        texture->reset(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 512, 512, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        _depthTexture = move(texture);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->getTexId(), 0);
    }

    if (nbr < _outTextures.size())
    {
        for (int i = nbr; i < _outTextures.size(); ++i)
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);

        _outTextures.resize(nbr);
    }
    else
    {
        for (int i = _outTextures.size(); i < nbr; ++i)
        {
            TexturePtr texture(new Texture);
            texture->reset(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
            _outTextures.push_back(texture);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, texture->getTexId(), 0);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glfwMakeContextCurrent(NULL);
}

/*************/
void Camera::setOutputSize(int width, int height)
{
    if (width == 0 || height == 0)
        return;

    glfwMakeContextCurrent(_window->get());
    _depthTexture->resize(width, height);

    for (auto tex : _outTextures)
        tex->resize(width, height);

    _width = width;
    _height = height;

    glfwMakeContextCurrent(NULL);
}

/*************/
mat4x4 Camera::computeViewProjectionMatrix()
{
    mat4x4 viewMatrix = lookAt(_eye, _target, _up);
    mat4x4 projMatrix = perspectiveFov(_fov, _width, _height, _near, _far);
    mat4x4 viewProjectionMatrix = projMatrix * viewMatrix;

    return viewProjectionMatrix;
}

/*************/
void Camera::loadDefaultModels()
{
    map<string, string> files {{"3d_marker", "3d_marker.obj"}};
    
    for (auto& file : files)
    {
        MeshPtr mesh(new Mesh());
        mesh->setAttribute("name", {file.first});
        mesh->setAttribute("file", {file.second});

        ObjectPtr obj(new Object());
        obj->setAttribute("name", {file.first});
        obj->setAttribute("scale", {0.05});
        obj->getShader()->setAttribute("color", {1.0, 0.5, 0.0, 1.0});
        obj->linkTo(mesh);

        _models[file.first] = obj;
    }
}

/*************/
void Camera::registerAttributes()
{
    _attribFunctions["eye"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _eye = vec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());
        return true;
    });

    _attribFunctions["target"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _target = vec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());
        return true;
    });

    _attribFunctions["fov"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _fov = args[0].asFloat();
        return true;
    });

    _attribFunctions["up"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _up = vec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());
        return true;
    });

    _attribFunctions["size"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        setOutputSize(args[0].asInt(), args[1].asInt());
        return true;
    });

    // More advanced attributes
    _attribFunctions["moveEye"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _eye.x = _eye.x + args[0].asFloat();
        _eye.y = _eye.y + args[1].asFloat();
        _eye.z = _eye.z + args[2].asFloat();
    });

    _attribFunctions["moveTarget"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _target.x = _target.x + args[0].asFloat();
        _target.y = _target.y + args[1].asFloat();
        _target.z = _target.z + args[2].asFloat();
    });

    _attribFunctions["rotateAroundTarget"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        auto direction = _target - _eye;
        auto rotZ = rotate(mat4(1.f), args[0].asFloat(), vec3(0.0, 0.0, 1.0));
        auto newDirection = vec4(direction, 1.0) * rotZ;
        _eye = _target - vec3(newDirection.x, newDirection.y, newDirection.z);
    });

    // Rendering options
    _attribFunctions["frame"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        if (args[0].asInt() > 0)
            _drawFrame = true;
        else
            _drawFrame = false;
    });


}

} // end of namespace
