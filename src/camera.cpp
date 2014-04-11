#include "camera.h"
#include "timer.h"
#include "threadpool.h"

#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#define SPLASH_SCISSOR_WIDTH 8
#define SPLASH_WORLDMARKER_SCALE 0.015
#define SPLASH_SCREENMARKER_SCALE 0.05
#define SPLASH_MARKER_SELECTED {0.9, 0.1, 0.1, 1.0}
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
    if (!_window->setAsCurrentContext()) 
		 SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
    glGetError();
    glGenFramebuffers(1, &_fbo);
    _window->releaseContext();

    setOutputNbr(1);

    if (!_window->setAsCurrentContext()) 
		 SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
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

    _window->releaseContext();

    registerAttributes();
}

/*************/
Camera::~Camera()
{
#ifdef DEBUG
    SLog::log<< Log::DEBUGGING << "Camera::~Camera - Destructor" << Log::endl;
#endif
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

    if (!_window->setAsCurrentContext()) 
		 SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
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
    _window->releaseContext();

    bool writeToShm = _writeToShm;
    _writeToShm = false;
    setOutputSize(dims[0] / 4, dims[1] / 4);

    // Render with the current texture, with no marker or frame
    bool drawFrame = _drawFrame;
    bool displayCalibration = _displayCalibration;
    _drawFrame = _displayCalibration = false;
    render();
    _drawFrame = drawFrame;
    _displayCalibration = displayCalibration;

    if (!_window->setAsCurrentContext()) 
		 SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
    GLenum error = glGetError();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
    ImageBuf img(_outTextures[0]->getSpec());
    glReadPixels(0, 0, img.spec().width, img.spec().height, GL_RGBA, GL_UNSIGNED_SHORT, img.localpixels());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Reset the objects to their initial shader
    int fillIndex {0};
    for (auto& obj : _objects)
    {
        obj->setAttribute("fill", shaderFill[fillIndex]);
        fillIndex++;
    }
    error = glGetError();
    _window->releaseContext();

    _writeToShm = writeToShm;
    setOutputSize(width, height);

    if (error)
        SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - Error while computing the blending map : " << error << Log::endl;

    // Go through the rendered image, fill the map with the "used" pixels from the original texture
    oiio::ImageSpec mapSpec = map->getSpec();
    vector<bool> isSet(mapSpec.width * mapSpec.height); // If a pixel is detected for this camera, only note it once
    unsigned short* imageMap = (unsigned short*)map->data();
    
    for (ImageBuf::ConstIterator<unsigned short> p(img); !p.done(); ++p)
    {
        if (!p.exists())
            continue;

        // UV coordinates are mapped on 2 uchar each
        int x = (int)round((p[0] * 65536.f + p[1] * 256.f) * 0.0000152587890625f * (float)mapSpec.width);
        int y = (int)round((p[2] * 65536.f + p[3] * 256.f) * 0.0000152587890625f * (float)mapSpec.height);

        if (isSet[y * mapSpec.width + x])
            continue;
        isSet[y * mapSpec.width + x] = true;

        float distX = (float)std::min(p.x(), img.spec().width - 1 - p.x()) / (float)img.spec().width;
        float distY = (float)std::min(p.y(), img.spec().height - 1 - p.y()) / (float)img.spec().height;
        if (_blendWidth > 0.f)
        {
            int blendValue = (int)std::min(256.f, std::min(distX, distY) * 256.f / _blendWidth);
            imageMap[y * mapSpec.width + x] += blendValue; // One more camera displaying this pixel
        }
        else
            imageMap[y * mapSpec.width + x] += 256; // One more camera displaying this pixel

        // We keep the real number of projectors, hidden higher in the shorts
        imageMap[y * mapSpec.width + x] += 4096;
    }
}


/*************/
bool Camera::doCalibration()
{
    int pointsSet = 0;
    for (auto& point : _calibrationPoints)
        if (point.isSet)
            pointsSet++;
    // We need at least 4 points to get a meaningful calibration
    if (pointsSet < 6)
    {
        SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - Calibration needs at least 6 points" << Log::endl;
        return false;
    }

    gsl_multimin_function calibrationFunc;
    calibrationFunc.n = 3;
    calibrationFunc.f = &Camera::cameraCalibration_f;
    GslParam params;
    params.context = this;
    calibrationFunc.params = (void*)&params;

    const gsl_multimin_fminimizer_type* minimizerType;
    gsl_multimin_fminimizer* minimizer;

    gsl_vector* x = gsl_vector_alloc(3);
    gsl_vector* step = gsl_vector_alloc(3);
    gsl_vector_set(step, 0, 2.0);
    gsl_vector_set(step, 1, 5.0);
    gsl_vector_set(step, 2, 5.0);

    SLog::log << "Camera::" << __FUNCTION__ << " - Starting calibration..." << Log::endl;

    minimizerType = gsl_multimin_fminimizer_nmsimplex2;

    // Starting with various values of the FOV
    double initialFov = _fov;
    double minValue = numeric_limits<double>::max();
    for (double s = -1.0; s < 6.0; ++s) // Vary the vertical shift
    for (double t = 1.0; t < 4.0; ++t) // Vary the horizontal shift
    for (double f = 0.0; f < 3.0; ++f) // Vary the FOV
    {
        minimizer = gsl_multimin_fminimizer_alloc(minimizerType, 3);

        gsl_vector_set(x, 0, 10.0 + f * 20.0);
        gsl_vector_set(x, 1, (double)_width * t * 0.2);
        gsl_vector_set(x, 2, (double)_height * s * 0.2);
        gsl_multimin_fminimizer_set(minimizer, &calibrationFunc, x, step);

        size_t iter = 0;
        int status = GSL_CONTINUE;
        while(status == GSL_CONTINUE && iter < 100)
        {
            iter++;
            status = gsl_multimin_fminimizer_iterate(minimizer);
            if (status)
            {
                SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - An error has occured during minimization" << Log::endl;
                break;
            }

            status = gsl_multimin_test_size(minimizer->size, 1e-2);
        }

        if (gsl_multimin_fminimizer_minimum(minimizer) < minValue)
        {
            minValue = gsl_multimin_fminimizer_minimum(minimizer);
            // We call the calibration function one last time, to set the extrinsic parameters
            params.setExtrinsic = true;
            cameraCalibration_f(minimizer->x, (void*)&params);

            _fov = gsl_vector_get(minimizer->x, 0);
            _cx = gsl_vector_get(minimizer->x, 1) / _width;
            _cy = (_height - gsl_vector_get(minimizer->x, 2)) / _height;

            params.setExtrinsic = false; // For next iterations
        }

        gsl_multimin_fminimizer_free(minimizer);
    }

    gsl_vector_free(x);
    gsl_vector_free(step);

    SLog::log << "Camera::" << __FUNCTION__ << " - Minumum found at (fov, cx, cy): " << _fov << " " << _cx << " " << _cy << Log::endl;
    SLog::log << "Camera::" << __FUNCTION__ << " - Minimum value: " << minValue << Log::endl;

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
    if (!_window->setAsCurrentContext()) 
		 SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
    float depth;
    glReadPixels(realX, realY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    _window->releaseContext();

    if (depth == 1.f)
        return vector<Value>();

    // Unproject the point
    vec3 screenPoint(realX, realY, depth);

    float distance = numeric_limits<float>::max();
    vec3 vertex;
    for (auto& obj : _objects)
    {
        vec3 point = unProject(screenPoint, lookAt(_eye, _target, _up) * obj->getModelMatrix(),
                               computeProjectionMatrix(), vec4(0, 0, _width, _height));
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
    if (!_window->setAsCurrentContext()) 
		 SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
    float depth;
    glReadPixels(realX, realY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    _window->releaseContext();

    if (depth == 1.f)
        return vector<Value>();

    // Unproject the point
    vec3 screenPoint(realX, realY, depth);

    float distance = numeric_limits<float>::max();
    vec3 vertex;
    for (auto& cPoint : _calibrationPoints)
    {
        vec3 point = unProject(screenPoint, lookAt(_eye, _target, _up),
                               computeProjectionMatrix(), vec4(0, 0, _width, _height));
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
    ImageSpec spec = _outTextures[0]->getSpec();
    if (spec.width != _width || spec.height != _height)
        setOutputSize(spec.width, spec.height);

    if (!_window->setAsCurrentContext()) 
		 SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;

    if (_outTextures.size() < 1)
        return false;

    glGetError();
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
        glEnable(GL_SCISSOR_TEST);
        glScissor(SPLASH_SCISSOR_WIDTH, SPLASH_SCISSOR_WIDTH, _width - SPLASH_SCISSOR_WIDTH * 2, _height - SPLASH_SCISSOR_WIDTH * 2);
    }

    if (_flashBG && !_hidden)
        glClearColor(0.6, 0.6, 0.6, 1.0);
    else
        glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!_hidden)
    {
        // Draw the objects
        for (auto& obj : _objects)
        {
            obj->getShader()->setAttribute("blendWidth", {_blendWidth});
            obj->getShader()->setAttribute("blackLevel", {_blackLevel});
            obj->getShader()->setAttribute("brightness", {_brightness});

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
                }
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

    // Output to a shared memory if set so
    if (_writeToShm)
    {
        // Only the first output texture can be sent currently
        if (_outShm.get() == nullptr)
            _outShm = make_shared<Image_Shmdata>();
        
        // First, we launch the copy to the host
        ImageBuf img(_outTextures[0]->getSpec());
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[_pboReadIndex]);
        GLubyte* gpuPixels = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        if (gpuPixels != NULL)
        {
            memcpy((void*)img.localpixels(), gpuPixels, _width * _height * 4);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        _pboReadIndex = (_pboReadIndex + 1) % 2;

        // Then we launch the copy from the FBO to the PBO, which will be finished by next frame
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[_pboReadIndex]);
        glReadPixels(0, 0, _width, _height, GL_RGBA, GL_UNSIGNED_SHORT, 0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        _outShm->write(img, string("/tmp/splash_") + _name);
    }

    _window->releaseContext();

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
                _selectedCalibrationPoint = i;
                return true;
        }

    _calibrationPoints.push_back(CalibrationPoint(world));
    _selectedCalibrationPoint = _calibrationPoints.size() - 1;
    return true;
}

/*************/
void Camera::deselectCalibrationPoint()
{
    _selectedCalibrationPoint = -1;
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
void Camera::removeCalibrationPoint(std::vector<Value> worldPoint, bool unlessSet)
{
    if (worldPoint.size() < 3)
        return;

    vec3 world(worldPoint[0].asFloat(), worldPoint[1].asFloat(), worldPoint[2].asFloat());

    for (int i = 0; i < _calibrationPoints.size(); ++i)
        if (_calibrationPoints[i].world == world)
        {
            if (_calibrationPoints[i].isSet == true && unlessSet)
                continue;
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

    if (!_window->setAsCurrentContext()) 
		 SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    if (!_depthTexture)
    {
        _depthTexture = make_shared<Texture>(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 512, 512, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
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
            TexturePtr texture = make_shared<Texture>();
            texture->disableFiltering();
            texture->reset(GL_TEXTURE_2D, 0, GL_RGBA16, 512, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT, nullptr);
            _outTextures.push_back(texture);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _outTextures.back()->getTexId(), 0);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    _window->releaseContext();
}

/*************/
void Camera::setOutputSize(int width, int height)
{
    if (width == 0 || height == 0)
        return;

    if (!_window->setAsCurrentContext()) 
		 SLog::log << Log::WARNING << "Camera::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
    _depthTexture->resize(width, height);

    for (auto tex : _outTextures)
        tex->resize(width, height);

    _width = width;
    _height = height;

    // PBOs related things
    if (_writeToShm)
    {
        glDeleteBuffers(2, _pbos);
        glGenBuffers(2, _pbos);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[0]);
        glBufferData(GL_PIXEL_PACK_BUFFER, _width * _height * 4, 0, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbos[1]);
        glBufferData(GL_PIXEL_PACK_BUFFER, _width * _height * 4, 0, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    _window->releaseContext();
}

/*************/
double Camera::cameraCalibration_f(const gsl_vector* v, void* params)
{
    if (params == NULL)
        return 0.0;

    GslParam* p = (GslParam*)params;
    Camera* camera = p->context;
    bool setExtrinsic = p->setExtrinsic;

    double fov = gsl_vector_get(v, 0);
    double cx = gsl_vector_get(v, 1);
    double cy = gsl_vector_get(v, 2);

    vector<cv::Point3f> objectPoints;
    vector<cv::Point2f> imagePoints;

    for (auto& point : camera->_calibrationPoints)
    {
        if (!point.isSet)
            continue;

        objectPoints.push_back(cv::Point3f(point.world.x, point.world.y, point.world.z));
        imagePoints.push_back(cv::Point2f((point.screen.x + 1.f) / 2.f * camera->_width, (-point.screen.y + 1.f) / 2.f * camera->_height));
    }

#ifdef DEBUG
    SLog::log << Log::DEBUGGING << "Camera::" << __FUNCTION__ << " - Values for the current iteration (fov, cx, cy): " << fov << " " << cx << " " << camera->_height - cy << Log::endl;
#endif
    double fovPix = (camera->_height / 2.0) / tan(fov * M_PI / 360.0); // _fov is indeed fovY
    cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) << fovPix, 0.0, cx,
                                                      0.0, fovPix, cy,
                                                      0.0, 0.0, 1.0);

    mat4 lookM = lookAt(camera->_eye, camera->_target, camera->_up);
    lookM = inverse(lookM);
    cv::Mat distCoeffs = cv::Mat::zeros(5, 1, CV_64F);
    cv::Mat rvec = (cv::Mat_<double>(3, 3) << lookM[0][0], lookM[1][0], lookM[2][0],
                                              lookM[0][1], lookM[1][1], lookM[2][1],
                                              lookM[0][2], lookM[1][2], lookM[2][2]);
    cv::Mat tvec = (cv::Mat_<double>(3, 1) << camera->_eye[0], camera->_eye[1], camera->_eye[2]);
    vector<int> inliers;

    try
    {
        cv::solvePnPRansac(objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec, true, 100, (int)camera->_width / 100, 100, inliers, cv::EPNP);
    }
    catch (cv::Exception& e)
    {
        SLog::log << "Camera::" << __FUNCTION__ << " - An exception was caught while running calibration" << Log::endl;
        return false;
    }

    // Compute the projection error with the new parameters
    vector<cv::Point2f> projectedPoints;
    cv::projectPoints(objectPoints, rvec, tvec, cameraMatrix, cv::Mat(), projectedPoints);
    double summedDistance = 0.0;
    for (int i = 0; i < imagePoints.size(); ++i)
    {
        cv::Point2f vec = imagePoints[i] - projectedPoints[i];
        summedDistance += pow(cv::norm(vec), 2.0);
    }
    summedDistance /= imagePoints.size();

#ifdef DEBUG
    SLog::log << Log::DEBUGGING << "Camera::" << __FUNCTION__ << " - Actual summed distance: " << summedDistance << Log::endl;
#endif

    if (setExtrinsic)
    {
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
        camera->_eye = vec3(origin.x, origin.y, origin.z);

        vec4 direction(0.f, 0.f, 1.f, 1.f);
        direction = viewMatrix * direction;
        camera->_target = vec3(direction.x, direction.y, direction.z);

        vec4 up(0.f, -1.f, 0.f, 0.f);
        up = viewMatrix * up;
        camera->_up = vec3(up.x, up.y, up.z);
    }

    return summedDistance;
}

/*************/
mat4x4 Camera::computeProjectionMatrix()
{
    float l, r, t, b, n, f;
    // Near and far are obvious
    n = _near;
    f = _far;
    // Up and down
    float tTemp = n * tan(_fov * M_PI / 360.0);
    float bTemp = -tTemp;
    t = tTemp - (_cy - 0.5) * (tTemp - bTemp);
    b = bTemp - (_cy - 0.5) * (tTemp - bTemp);
    // Left and right
    float rTemp = tTemp * _width / _height;
    float lTemp = bTemp * _width / _height;
    r = rTemp - (_cx - 0.5) * (rTemp - lTemp);
    l = lTemp - (_cx - 0.5) * (rTemp - lTemp);

    return frustum(l, r, b, t, n, f);
}

/*************/
mat4x4 Camera::computeViewProjectionMatrix()
{
    mat4 viewMatrix = lookAt(_eye, _target, _up);
    mat4 projMatrix = computeProjectionMatrix();
    mat4 viewProjectionMatrix = projMatrix * viewMatrix;

    return viewProjectionMatrix;
}

/*************/
void Camera::loadDefaultModels()
{
    map<string, string> files {{"3d_marker", "3d_marker.obj"}, {"2d_marker", "2d_marker.obj"}};
    
    for (auto& file : files)
    {
        MeshPtr mesh = make_shared<Mesh>();
        mesh->setAttribute("name", {file.first});
        mesh->setAttribute("file", {file.second});

        ObjectPtr obj = make_shared<Object>();
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

    _attribFunctions["principalPoint"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        _cx = args[0].asFloat();
        _cy = args[1].asFloat();
        return true;
    }, [&]() {
        return vector<Value>({_cx, _cy});
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

    _attribFunctions["addCalibrationPoint"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        addCalibrationPoint({args[0].asFloat(), args[1].asFloat(), args[2].asFloat()});
        return true;
    });

    _attribFunctions["deselectedCalibrationPoint"] = AttributeFunctor([&](vector<Value> args) {
        deselectCalibrationPoint();
        return true;
    });

    _attribFunctions["removeCalibrationPoint"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        else if (args.size() == 3)
            removeCalibrationPoint({args[0].asFloat(), args[1].asFloat(), args[2].asFloat()});
        else
            removeCalibrationPoint({args[0].asFloat(), args[1].asFloat(), args[2].asFloat()}, args[3].asInt());
        return true;
    });

    _attribFunctions["setCalibrationPoint"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        return setCalibrationPoint({args[0].asFloat(), args[1].asFloat()});
    });

    // Rendering options
    _attribFunctions["blendWidth"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _blendWidth = args[0].asFloat();
        return true;
    }, [&]() {
        return vector<Value>({_blendWidth});
    });

    _attribFunctions["blackLevel"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _blackLevel = args[0].asFloat();
        return true;
    }, [&]() {
        return vector<Value>({_blackLevel});
    });

    _attribFunctions["brightness"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _brightness = args[0].asFloat();
        return true;
    }, [&]() {
        return vector<Value>({_brightness});
    });

    _attribFunctions["frame"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        if (args[0].asInt() > 0)
            _drawFrame = true;
        else
            _drawFrame = false;
        return true;
    });

    _attribFunctions["hide"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        if (args[0].asInt() > 0)
            _hidden = true;
        else
            _hidden = false;
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

    _attribFunctions["shared"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        if (args[0].asInt() > 0)
            _writeToShm = true;
        else
            _writeToShm = false;
        return true;
    }, [&]() {
        return vector<Value>({_writeToShm});
    });

    _attribFunctions["flashBG"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _flashBG = args[0].asInt();
        return true;
    });
}

} // end of namespace
