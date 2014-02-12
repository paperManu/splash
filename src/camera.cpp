#include "camera.h"
#include "timer.h"
#include "threadpool.h"

#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#define SPLASH_SCISSOR_WIDTH 8
#define SPLASH_WORLDMARKER_SCALE 0.01
#define SPLASH_SCREENMARKER_SCALE 0.05
#define SPLASH_MARKER_SELECTED {0.0, 1.0, 0.0, 1.0}
#define SPLASH_MARKER_ADDED {0.0, 0.5, 1.0, 1.0}
#define SPLASH_MARKER_SET {1.0, 0.5, 0.0, 1.0}

using namespace std;
using namespace glm;
using namespace OIIO_NAMESPACE;

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
void Camera::computeBlendingMap(ImagePtr& map)
{
    if (map->getSpec().format != oiio::TypeDesc::UINT16)
    {
        SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - Input map is not of type UINT16." << Log::endl;
        return;
    }

    glfwMakeContextCurrent(_window->get());
    // We want to render the object with a specific texture, containing texture coordinates
    vector<vector<Value>> shaderFill;
    for (auto& obj : _objects)
    {
        vector<Value> fill;
        obj->getAttribute("fill", fill);
        obj->setAttribute("fill", {"uv"});
        shaderFill.push_back(fill);
    }

    // Increase the render size for more precision
    int width = _width;
    int height = _height;
    int dims[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, dims);
    if (width >= height)
        dims[1] = dims[0] * height / width;
    else
        dims[0] = dims[1] * width / height;
    glfwMakeContextCurrent(NULL);

    setOutputSize(dims[0] / 4, dims[1] / 4);

    // Render with the current texture, with no marker or frame
    bool drawFrame = _drawFrame;
    bool displayCalibration = _displayCalibration;
    _drawFrame = _displayCalibration = false;
    render();
    _drawFrame = drawFrame;
    _displayCalibration = displayCalibration;

    glfwMakeContextCurrent(_window->get());
    GLenum error = glGetError();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
    ImageBuf img(_outTextures[0]->getSpec());
    glReadPixels(0, 0, img.spec().width, img.spec().height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, img.localpixels());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Reset the objects to their initial shader
    int fillIndex {0};
    for (auto& obj : _objects)
    {
        obj->setAttribute("fill", shaderFill[fillIndex]);
        fillIndex++;
    }
    error = glGetError();
    glfwMakeContextCurrent(NULL);

    setOutputSize(width, height);

    if (error)
        SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - Error while computing the blending map : " << error << Log::endl;

    // Go through the rendered image, fill the map with the "used" pixels from the original texture
    oiio::ImageSpec mapSpec = map->getSpec();
    vector<bool> isSet(mapSpec.width * mapSpec.height); // If a pixel is detected for this camera, only note it once
    unsigned short* imageMap = (unsigned short*)map->data();
    
    for (ImageBuf::ConstIterator<unsigned char> p(img); !p.done(); ++p)
    {
        if (!p.exists())
            continue;

        // UV coordinates are mapped on 2 uchar each
        int x = (int)((p[0] * 65536.f + p[1] * 256.f) * 0.0000152587890625f * (float)mapSpec.width);
        int y = (int)((p[2] * 65536.f + p[3] * 256.f) * 0.0000152587890625f * (float)mapSpec.height);

        if (isSet[y * mapSpec.width + x])
            continue;
        isSet[y * mapSpec.width + x] = true;

        float distX = (float)std::min(p.x(), img.spec().width - p.x()) / (float)img.spec().width;
        float distY = (float)std::min(p.y(), img.spec().height - p.y()) / (float)img.spec().height;
        if (_blendWidth > 0.f)
        {
            int blendValue = (int)std::min(255.f, std::min(distX, distY) * 255.f / _blendWidth);
            imageMap[y * mapSpec.width + x] += blendValue; // One more camera displaying this pixel
        }
        else
            imageMap[y * mapSpec.width + x] += 255; // One more camera displaying this pixel
    }
}

/*************/
bool Camera::doCalibration()
{
    vector<cv::Point3f> objectPoints;
    vector<cv::Point2f> imagePoints;

    for (auto& point : _calibrationPoints)
    {
        if (!point.isSet)
            continue;

        objectPoints.push_back(cv::Point3f(point.world.x, point.world.y, point.world.z));
        imagePoints.push_back(cv::Point2f((point.screen.x + 1.f) / 2.f * _width, (-point.screen.y + 1.f) / 2.f * _height));
    }

    // We need at least 4 points to get a meaningful calibration
    if (objectPoints.size() < 4)
    {
        SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - Calibration needs at least 4 points" << Log::endl;
        return false;
    }

    double fov = (_height / 2.0) / tan(_fov * M_PI / 360.0); // _fov is indeed fovY
    cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) << fov, 0.0, _width / 2.0,
                                                      0.0, fov, _height / 2.0,
                                                      0.0, 0.0, 1.0);

    mat4 lookM = lookAt(_eye, _target, _up);
    lookM = inverse(lookM);
    cv::Mat distCoeffs = cv::Mat::zeros(5, 1, CV_64F);
    cv::Mat rvec = (cv::Mat_<double>(3, 3) << lookM[0][0], lookM[1][0], lookM[2][0],
                                              lookM[0][1], lookM[1][1], lookM[2][1],
                                              lookM[0][2], lookM[1][2], lookM[2][2]);
    cv::Mat tvec = (cv::Mat_<double>(3, 1) << _eye[0], _eye[1], _eye[2]);

    try
    {
        cv::solvePnPRansac(objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec, true);
    }
    catch (cv::Exception& e)
    {
        SLog::log << "Camera::" << __FUNCTION__ << " - An exception was caught while running calibration" << Log::endl;
        return false;
    }

    // Get a usable rotation matrix
    cv::Mat r;
    cv::Rodrigues(rvec, r);

    mat4 trans(1.f);
    trans[3][0] = tvec.at<double>(0);
    trans[3][1] = tvec.at<double>(1);
    trans[3][2] = tvec.at<double>(2);

    mat4 rot = mat4(r.at<double>(0, 0), r.at<double>(1, 0), r.at<double>(2, 0), 0.f, 
                    r.at<double>(0, 1), r.at<double>(1, 1), r.at<double>(2, 1), 0.f, 
                    r.at<double>(0, 2), r.at<double>(1, 2), r.at<double>(2, 2), 0.f, 
                    0.f, 0.f, 0.f, 1.f); 

    mat4 viewMatrix = trans * rot;
    // We have the object pose in the camera base. We want the camera pose in the world base
    viewMatrix = inverse(viewMatrix);

    vec4 origin(0.f, 0.f, 0.f, 1.f);
    origin = viewMatrix * origin;
    _eye = vec3(origin.x, origin.y, origin.z);

    vec4 direction(0.f, 0.f, 1.f, 1.f);
    direction = viewMatrix * direction;
    _target = vec3(direction.x, direction.y, direction.z);

    vec4 up(0.f, -1.f, 0.f, 1.f);
    up = viewMatrix * up;
    _up = vec3(up.x, up.y, up.z);

    SLog::log << "Camera::" << __FUNCTION__ << " - Calibration done" << Log::endl;

    return true;
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
    float realY = y * _height;

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
        vec3 point = unProject(screenPoint, lookAt(_eye, _target, _up) * obj->getModelMatrix(),
                               perspectiveFov(_fov, _width, _height, _near, _far), vec4(0, 0, _width, _height));
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
vector<Value> Camera::pickCalibrationPoint(float x, float y)
{
    // Convert the normalized coordinates ([0, 1]) to pixel coordinates
    float realX = x * _width;
    float realY = y * _height;

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
    for (auto& cPoint : _calibrationPoints)
    {
        vec3 point = unProject(screenPoint, lookAt(_eye, _target, _up),
                               perspectiveFov(_fov, _width, _height, _near, _far), vec4(0, 0, _width, _height));
        glm::vec3 closestVertex;
        float tmpDist;
        if ((tmpDist = length(point - cPoint.world)) < distance)
        {
            distance = tmpDist;
            vertex = cPoint.world;
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
        obj->getShader()->setAttribute("blendWidth", {_blendWidth});
        obj->activate();
        obj->setViewProjectionMatrix(computeViewProjectionMatrix());
        obj->draw();
        obj->deactivate();
    }

    // Draw the calibration points
    if (_displayCalibration)
    {
        for (int i = 0; i < _calibrationPoints.size(); ++i)
        {
            auto& point = _calibrationPoints[i];
            ObjectPtr worldMarker = _models["3d_marker"];
            worldMarker->setAttribute("position", {point.world.x, point.world.y, point.world.z});
            if (_selectedCalibrationPoint == i)
                worldMarker->setAttribute("color", SPLASH_MARKER_SELECTED);
            else if (point.isSet)
                worldMarker->setAttribute("color", SPLASH_MARKER_SET);
            else
                worldMarker->setAttribute("color", SPLASH_MARKER_ADDED);
            worldMarker->activate();
            worldMarker->setViewProjectionMatrix(computeViewProjectionMatrix());
            worldMarker->draw();
            worldMarker->deactivate();

            if (point.isSet && _selectedCalibrationPoint == i || _showAllCalibrationPoints) // Draw the target position on screen as well
            {
                ObjectPtr screenMarker = _models["2d_marker"];
                screenMarker->setAttribute("position", {point.screen.x, point.screen.y, 0.f});
                screenMarker->setAttribute("scale", {SPLASH_SCREENMARKER_SCALE});
                if (_selectedCalibrationPoint == i)
                    screenMarker->setAttribute("color", SPLASH_MARKER_SELECTED);
                else
                    screenMarker->setAttribute("color", SPLASH_MARKER_SET);
                screenMarker->activate();
                screenMarker->setViewProjectionMatrix(mat4(1.f));
                screenMarker->draw();
                screenMarker->deactivate();
                screenMarker->setAttribute("scale", {SPLASH_WORLDMARKER_SCALE});
            }
        }
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
bool Camera::addCalibrationPoint(std::vector<Value> worldPoint)
{
    if (worldPoint.size() < 3)
        return false;

    vec3 world(worldPoint[0].asFloat(), worldPoint[1].asFloat(), worldPoint[2].asFloat());

    // Check if the point is already present
    for (int i = 0; i < _calibrationPoints.size(); ++i)
        if (_calibrationPoints[i].world == world)
        {
            if (!_calibrationPoints[i].isSet)
            {
                _calibrationPoints.erase(_calibrationPoints.begin() + i);
                _selectedCalibrationPoint = -1;
                return false;
            }
            else
            {
                _selectedCalibrationPoint = i;
                return true;
            }
        }

    _calibrationPoints.push_back(CalibrationPoint(world));
    _selectedCalibrationPoint = _calibrationPoints.size() - 1;
    return true;
}

/*************/
void Camera::moveCalibrationPoint(float dx, float dy)
{
    if (_selectedCalibrationPoint == -1)
        return;

    _calibrationPoints[_selectedCalibrationPoint].screen.x += dx / _width;
    _calibrationPoints[_selectedCalibrationPoint].screen.y += dy / _height;
}

/*************/
void Camera::removeCalibrationPoint(std::vector<Value> worldPoint)
{
    if (worldPoint.size() < 3)
        return;

    vec3 world(worldPoint[0].asFloat(), worldPoint[1].asFloat(), worldPoint[2].asFloat());

    for (int i = 0; i < _calibrationPoints.size(); ++i)
        if (_calibrationPoints[i].world == world)
        {
            _calibrationPoints.erase(_calibrationPoints.begin() + i);
            _selectedCalibrationPoint = -1;
        }
}

/*************/
bool Camera::setCalibrationPoint(std::vector<Value> screenPoint)
{
    if (_selectedCalibrationPoint == -1)
        return false;

    _calibrationPoints[_selectedCalibrationPoint].screen = glm::vec2(screenPoint[0].asFloat(), screenPoint[1].asFloat());
    _calibrationPoints[_selectedCalibrationPoint].isSet = true;
    return true;
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
    mat4 viewMatrix = lookAt(_eye, _target, _up);
    mat4 projMatrix = perspectiveFov(_fov, _width, _height, _near, _far);
    mat4 viewProjectionMatrix = projMatrix * viewMatrix;

    return viewProjectionMatrix;
}

/*************/
void Camera::loadDefaultModels()
{
    map<string, string> files {{"3d_marker", "3d_marker.obj"}, {"2d_marker", "2d_marker.obj"}};
    
    for (auto& file : files)
    {
        MeshPtr mesh(new Mesh());
        mesh->setAttribute("name", {file.first});
        mesh->setAttribute("file", {file.second});

        ObjectPtr obj(new Object());
        obj->setAttribute("name", {file.first});
        obj->setAttribute("scale", {SPLASH_WORLDMARKER_SCALE});
        obj->setAttribute("fill", {"color"});
        obj->setAttribute("color", SPLASH_MARKER_SET);
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
    }, [&]() {
        return vector<Value>({_eye.x, _eye.y, _eye.z});
    });

    _attribFunctions["target"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _target = vec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());
        return true;
    }, [&]() {
        return vector<Value>({_target.x, _target.y, _target.z});
    });

    _attribFunctions["fov"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _fov = args[0].asFloat();
        return true;
    }, [&]() {
        return vector<Value>({_fov});
    });

    _attribFunctions["up"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _up = vec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());
        return true;
    }, [&]() {
        return vector<Value>({_up.x, _up.y, _up.z});
    });

    _attribFunctions["size"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        setOutputSize(args[0].asInt(), args[1].asInt());
        return true;
    }, [&]() {
        return vector<Value>({_width, _height});
    });

    // More advanced attributes
    _attribFunctions["moveEye"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _eye.x = _eye.x + args[0].asFloat();
        _eye.y = _eye.y + args[1].asFloat();
        _eye.z = _eye.z + args[2].asFloat();
        return true;
    });

    _attribFunctions["moveTarget"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _target.x = _target.x + args[0].asFloat();
        _target.y = _target.y + args[1].asFloat();
        _target.z = _target.z + args[2].asFloat();
        return true;
    });

    _attribFunctions["rotateAroundTarget"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        auto direction = _target - _eye;
        auto rotZ = rotate(mat4(1.f), args[0].asFloat(), vec3(0.0, 0.0, 1.0));
        auto newDirection = vec4(direction, 1.0) * rotZ;
        _eye = _target - vec3(newDirection.x, newDirection.y, newDirection.z);
        return true;
    });

    // Rendering options
    _attribFunctions["frame"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        if (args[0].asInt() > 0)
            _drawFrame = true;
        else
            _drawFrame = false;
        return true;
    });

    _attribFunctions["wireframe"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;

        string primitive;
        if (args[0].asInt() == 0)
            primitive = "texture";
        else
            primitive = "wireframe";

        for (auto& obj : _objects)
            obj->setAttribute("fill", {primitive});
        return true;
    });

    _attribFunctions["blendWidth"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _blendWidth = args[0].asFloat();
        return true;
    }, [&]() {
        return vector<Value>({_blendWidth});
    });

    // Various options
    _attribFunctions["displayCalibration"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        if (args[0].asInt() > 0)
            _displayCalibration = true;
        else
            _displayCalibration = false;
        return true;
    });

    _attribFunctions["switchShowAllCalibrationPoints"] = AttributeFunctor([&](vector<Value> args) {
        _showAllCalibrationPoints = !_showAllCalibrationPoints;
        return true;
    });
}

} // end of namespace
