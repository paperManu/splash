#include <doctest.h>

#include "./controller/colorcalibrator.h"
#include "./image/image_list.h"

#include "./utils/osutils.h"

#include <fstream>

using namespace Splash;

/*************/
class RootObjectMock : public RootObject
{
  public:
    RootObjectMock()
        : RootObject()
    {
        registerAttributes();
        _name = "world";
        _tree.setName(_name);
    }

    void step() { updateTreeFromObjects(); }
};

/*************/
class ColorCalibratorMock : public ColorCalibrator
{
  public:
    ColorCalibratorMock(RootObject* root)
        : ColorCalibrator(root){};

    float testFindCorrectExposure(float expectedExposure) { return (std::abs(expectedExposure - findCorrectExposure()) < 1e-5); }

    bool testCRF(cv::Mat expectedCRF)
    {
        captureHDR(9, 0.33);
        return (cv::norm(expectedCRF, _crf, cv::NORM_L1) < 1e-5);
    }

    bool testCaptureHDR(cv::Mat3f expectedHdr)
    {
        cv::Mat3f hdr = captureHDR(1);
        for (int x = 0; x < hdr.rows; x++)
            for (int y = 0; y < hdr.cols; y++)
                for (int c = 0; c < 3; c++)
                    if (std::abs(expectedHdr.at<cv::Vec3f>(x, y)[c] - hdr.at<cv::Vec3f>(x, y)[c]) > 1e-3)
                        return false;
        return true;
    }

    bool testGetMaskROI(std::vector<bool> expectedMask)
    {
        // Compute hdr difference
        cv::Mat hdr = captureHDR(1);
        cv::Mat othersHdr = captureHDR(1);

        cv::Mat diffHdr = hdr.clone();
        for (int y = 0; y < diffHdr.rows; ++y)
            for (int x = 0; x < diffHdr.cols; ++x)
            {
                auto pixelValue = diffHdr.at<cv::Vec3f>(y, x) - othersHdr.at<cv::Vec3f>(y, x) * _displayDetectionThreshold;
                diffHdr.at<cv::Vec3f>(y, x)[0] = std::max(0.f, pixelValue[0]);
                diffHdr.at<cv::Vec3f>(y, x)[1] = std::max(0.f, pixelValue[1]);
                diffHdr.at<cv::Vec3f>(y, x)[2] = std::max(0.f, pixelValue[2]);
            }

        // Compute mask
        _mask = getMaskROI(diffHdr);

        for (int i = 0; i < _mask.size(); i++)
            if (!(_mask[i] == expectedMask[i]))
                return false;

        return true;
    }

    bool testGetMeanValue(std::vector<float> expectedMean)
    {
        cv::Mat hdr = captureHDR(1);
        std::vector<float> mean = getMeanValue(hdr, _mask);
        std::cout << mean[0] << "," << mean[1] << "," << mean[2] << "\n";
        for (int i = 0; i < 3; i++)
            if (std::abs(mean[i] - expectedMean[i]) > 1e-3)
                return false;
        return true;
    }

    bool testComputeProjectorFunctionInverse(std::vector<float> colorCurves, std::vector<float> expectedCurves)
    {
        // Input curves
        std::vector<Curve> curves{3};
        for (int s = 0; s < 3; ++s)
        {
            for (int c = 0; c < 3; ++c)
            {
                RgbValue sampleValue = RgbValue(0., 0., 0.);
                sampleValue[c] = colorCurves[s * 6 + 2 * c + 1];
                curves[c].push_back(Point(colorCurves[s * 6 + 2 * c], sampleValue));
            }
        }

        // Output projector curves
        std::vector<Curve> projCurves = computeProjectorFunctionInverse(curves);

        // Check values one by one
        for (unsigned int s = 0; s < _colorLUTSize; ++s)
        {
            for (int c = 0; c < 3; ++c)
            {
                bool sample = std::abs(expectedCurves[s * 6 + c * 2] - projCurves[c][s].first) < 1e-5;
                bool value = std::abs(expectedCurves[s * 6 + c * 2 + 1] - projCurves[c][s].second[c]) < 1e-5;

                if (!(sample && value))
                    return false;
            }
        }

        return true;
    }

    bool testEqualizeWhiteBalances(RgbValue expectedWB, std::pair<RgbValue, RgbValue> whitePoints)
    {
        // Initialize parameters
        CalibrationParams params;

        params.camName = "cam1";
        params.whitePoint = whitePoints.first;
        _calibrationParams.push_back(params);

        params.camName = "cam2";
        params.whitePoint = whitePoints.second;
        _calibrationParams.push_back(params);

        RgbValue whiteBalance = equalizeWhiteBalances();
        _calibrationParams.clear();

        // Check values one by one
        for (int i = 0; i < 3; i++)
            if (std::abs(whiteBalance[i] - expectedWB[i]) > 1e-5)
                return false;

        return true;
    }

  private:
    std::vector<bool> _mask{};
};

/*************/
TEST_CASE("Testing Color Calibration")
{
    std::string dataPath = Utils::getCurrentWorkingDirectory();
    dataPath = dataPath.substr(0, dataPath.size() - 11) + "tests/assets/color_calibration/";

    int imagesWidth = 496;
    int imagesHeight = 744;

    //
    // Set the right configuration
    //

    // Initialize objects
    auto root = RootObjectMock();
    auto image = std::dynamic_pointer_cast<Image_List>(root.createObject("image_list", "list").lock());
    image->setRemoteType("image_list");
    auto calibrator = ColorCalibratorMock(&root);

    // Set attributes and links
    calibrator.setAttribute("colorSamples", {3});
    image->read(dataPath);
    root.step(); // Update objects in the tree
    CHECK_EQ(calibrator.linkTo(image), true);

    // Check attributes
    Values status = calibrator.getObjectAttribute(image->getName(), "ready");
    Values exposure = calibrator.getObjectAttribute(image->getName(), "shutterspeed");
    CHECK_EQ(status.size(), 1);
    CHECK_EQ(status[0].as<bool>(), true);

    //
    // Check functions for camera response function
    //

    // Expected values

    // crf
    std::fstream crf;
    crf.open(dataPath + "crf.csv", std::ios::in);
    std::string line, val;

    cv::Mat expectedCRF(cv::Size(1, 256), CV_32FC3);
    int i = 0;
    while (std::getline(crf, line, '\n'))
    {

        std::stringstream ss(line);
        int c = 0;
        cv::Vec3f point;

        while (std::getline(ss, val, ','))
        {
            point[c] = std::stof(val);
            c++;
        }
        expectedCRF.at<cv::Vec3f>(i) = point;
        i++;
    }

    crf.close();

    CHECK_EQ(calibrator.testFindCorrectExposure(0.01f), true);
    CHECK_EQ(calibrator.testCRF(expectedCRF), true);

    //
    // Check functions for projectors calibration
    //

    // Expected values

    // mask
    std::fstream mask;
    mask.open(dataPath + "mask.csv", std::ios::in);

    std::vector<bool> expectedMask = std::vector<bool>(imagesWidth * imagesHeight, false);
    int rowIdx = 0;
    while (std::getline(mask, line, '\n'))
    {

        std::stringstream ss(line);
        int colIdx = 0;

        while (std::getline(ss, val, ','))
        {
            if (val == "1 " || val == " 1 " || val == " 1")
                expectedMask[rowIdx * imagesHeight + colIdx] = true;
            colIdx++;
        }
        rowIdx++;
    }

    mask.close();

    std::vector<float> expectedMean{12.4351f, 11.6104f, 11.5697f};

    // hdr
    std::fstream hdr;
    hdr.open(dataPath + "hdr.csv", std::ios::in);

    cv::Mat3f expectedHdr(cv::Size(imagesHeight, imagesWidth), CV_32FC3);
    ;
    int x = 0;
    int y;
    while (std::getline(hdr, line, '\n'))
    {

        std::stringstream ss(line);
        y = 0;

        while (std::getline(ss, val, ','))
        {
            expectedHdr.at<cv::Vec3f>(x, y)[0] = std::stof(val);
            std::getline(ss, val, ',');
            expectedHdr.at<cv::Vec3f>(x, y)[1] = std::stof(val);
            std::getline(ss, val, ',');
            expectedHdr.at<cv::Vec3f>(x, y)[2] = std::stof(val);
            y++;
        }
        x++;
    }

    hdr.close();

    // projector curves
    std::vector<float> expectedCurves{0,
        0.00163939,
        0,
        0.00173332,
        0,
        0.00119925,
        0.0666667,
        0.110932,
        0.0666667,
        0.117288,
        0.0666667,
        0.0811494,
        0.133333,
        0.220224,
        0.133333,
        0.232843,
        0.133333,
        0.161099,
        0.2,
        0.329517,
        0.2,
        0.348398,
        0.2,
        0.24105,
        0.266667,
        0.438809,
        0.266667,
        0.463953,
        0.266667,
        0.321,
        0.333333,
        0.521048,
        0.333333,
        0.532143,
        0.333333,
        0.40095,
        0.4,
        0.568872,
        0.4,
        0.578858,
        0.4,
        0.4809,
        0.466667,
        0.616695,
        0.466667,
        0.625574,
        0.466667,
        0.543362,
        0.533333,
        0.664518,
        0.533333,
        0.67229,
        0.533333,
        0.600335,
        0.6,
        0.712342,
        0.6,
        0.719005,
        0.6,
        0.657308,
        0.666667,
        0.760165,
        0.666667,
        0.765721,
        0.666667,
        0.714281,
        0.733333,
        0.807989,
        0.733333,
        0.812437,
        0.733333,
        0.771254,
        0.8,
        0.855812,
        0.8,
        0.859152,
        0.8,
        0.828227,
        0.866667,
        0.903636,
        0.866667,
        0.905868,
        0.866667,
        0.8852,
        0.933333,
        0.951459,
        0.933333,
        0.952584,
        0.933333,
        0.942173,
        1,
        0.999283,
        1,
        0.999299,
        1,
        0.999145};

    // Input values

    // Curves
    std::vector<float> colorCurves{0,
        5.1534833908081055,
        0,
        6.1088733673095703,
        0,
        5.5664911270141602,
        0.5,
        9.0373992919921875,
        0.5,
        12.081029891967773,
        0.5,
        15.282636642456055,
        1,
        17.929861068725586,
        1,
        26.884256362915039,
        1,
        28.926731109619141};

    // Cameras' white point
    std::pair<RgbValue, RgbValue> whitePoints{RgbValue(18.408, 18.213, 19.231), RgbValue(22.475, 21.821, 23.419)};

    CHECK_EQ(calibrator.testFindCorrectExposure(0.00296296f), true);
    CHECK_EQ(calibrator.testGetMaskROI(expectedMask), true);
    CHECK_EQ(calibrator.testGetMeanValue(expectedMean), true);
    CHECK_EQ(calibrator.testCaptureHDR(expectedHdr), true);
    CHECK_EQ(calibrator.testComputeProjectorFunctionInverse(colorCurves, expectedCurves), true);

    // Test equalize white balance methods
    RgbValue expectedWB;
    calibrator.setAttribute("equalizeMethod", {0});
    expectedWB = RgbValue(1.020339, 1.000000, 1.064563);
    CHECK_EQ(calibrator.testEqualizeWhiteBalances(expectedWB, whitePoints), true);
    calibrator.setAttribute("equalizeMethod", {1});
    expectedWB = RgbValue(1.010707, 1.000000, 1.055894);
    CHECK_EQ(calibrator.testEqualizeWhiteBalances(expectedWB, whitePoints), true);
    calibrator.setAttribute("equalizeMethod", {2});
    expectedWB = RgbValue(1.009368, 1.000000, 1.048908);
    CHECK_EQ(calibrator.testEqualizeWhiteBalances(expectedWB, whitePoints), true);
}
