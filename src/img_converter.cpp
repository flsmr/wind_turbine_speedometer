#ifndef IMGCONVERTER_CPP_
#define IMGCONVERTER_CPP_

#include "img_converter.h"

/* Use image reading library from
 * https://github.com/nothings/stb
*/
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Constructor
ImgConverter::ImgConverter() {}; 
// Destructor
ImgConverter::~ImgConverter() { stbi_image_free(_img); }

// load image from given filename
void ImgConverter::load(std::string filename) {
    _filename = filename;
    int width;
    int height;
    _img = stbi_load(filename.c_str(), &width, &height, &_nbChannels, 0);
    if (_img == NULL) {
        std::cout << "Could not load image file " << filename << std::endl;
        return;
    }
    _width = static_cast<size_t>(width);
    _height = static_cast<size_t>(height);
}

// save loaded image to filename. If no filename is given, the current file is overwritten
void ImgConverter::save() { save(_filename);};
void ImgConverter::save(std::string filename) {
    if (_img != NULL) {
        int width = static_cast<int>(_width);
        int height = static_cast<int>(_height);
        if (stbi_write_png(filename.c_str(), width, height, _nbChannels, _img, _width*_nbChannels) == 0) {
            std::cout << "Could not save image to file " << _filename << std::endl;
            return;
        }
    } else {
        std::cout << "No image to save."<< std::endl;
    }
}

// returns true if pixel coordinates are in bound of loaded image
bool ImgConverter::inBound (const Point point) {
    return (point[0] < _height && point[1] < _width);
}

// sets pixels with coordinates given in points to specified rgb color in loaded image 
void ImgConverter::writePointsToImg (std::shared_ptr<PointList> points, std::vector<uint8_t> color) {
    if (_img != NULL) {
        for (Point point: (*points)) {
            if (inBound(point)) {
                setRGBValue(point, color);
            }
        }
    } else {
        std::cout << "Could not write points to image: No image loaded."<< std::endl;
    }
}

// returns the rgb value of a pixel in loaded image or {0,0,0} if out of bound
void ImgConverter::getRGBValue(const Point point, std::vector<uint8_t> &rgbVal) {
    if (_img != NULL && inBound(point)) {
        size_t pos = (point[0] * _width + point[1]) * _nbChannels;
        for (size_t i = 0; i < _nbChannels; ++i) {
            rgbVal[i] = _img[pos+i];
        }
    } else {
        std::cout << "Could not get rgb value of point in image: ("<< point[0] <<", "<< point[1] <<") is out of bound or no image loaded."<< std::endl;
        rgbVal = {0,0,0};
    }        
}

// sets the rgb value of a pixel in loaded image to specified color
void ImgConverter::setRGBValue(const Point point, const std::vector<uint8_t> &rgbVal) {
    if (_img != NULL && inBound(point)) {
        size_t pos = (point[0] * _width + point[1]) * _nbChannels;
        for (size_t i = 0; i < _nbChannels; ++i) {
            _img[pos+i] = rgbVal[i];
        }
    } else {
        std::cout << "Could not set rgb value of point in image: ("<< point[0] <<", "<< point[1] <<") is out of bound or no image loaded."<< std::endl;
    }        
}

// returns a list of points above rgb threshold in defined region of interest
void ImgConverter::getPointsInROIAboveThreshold (const ROI roi, const std::vector<uint8_t> threshold, std::shared_ptr<PointList> points) {
    // limit region of interest to image boundaries
    int minCol = (roi.minCol < 0) ? 0 : roi.minCol;
    int maxCol = (roi.maxCol > _width) ? _width : roi.maxCol;
    int minRow = (roi.minRow < 0) ? 0 : roi.minRow;
    int maxRow = (roi.maxRow > _height) ? _height : roi.maxRow;
    std::vector<uint8_t> rgbVal = {0,0,0};

    // iterate of region of interest and check whether rgb values are above threshold
    for (size_t row = minRow; row < maxRow; ++row) {
        for (size_t col = minCol; col < maxCol; ++col) {
            getRGBValue({row, col}, rgbVal);
            bool aboveThreshold = false;
            for (size_t channel = 0; channel < _nbChannels; ++channel) {
                aboveThreshold = (aboveThreshold || rgbVal[channel] > threshold[channel]) ? true : false;
            }
            if(aboveThreshold) points->push_back({row,col});
        }
    }
}

// returns list of points for a line between start and endpoint according to Bresenham's line algorithm
// Pseudocode: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
void ImgConverter::getLinePoints(const Point startPoint, const Point endPoint, PointList &pointList) {
    // swap start and end to draw line from left to right
    bool swap = (startPoint[0]<endPoint[0]);
    size_t x0 = swap ? endPoint[0] : startPoint[0];
    size_t y0 = swap ? endPoint[1] : startPoint[1];
    size_t x1 = swap ? startPoint[0] : endPoint[0];
    size_t y1 = swap ? startPoint[1] : endPoint[1];

    float deltax = x1 - x0;
    float deltay = y1 - y0;
    // if vertical line
    if (deltax < 0.0000001f) {

    }

    float deltaerr = std::abs(deltay / deltax);
    float error = 0.0f;
    size_t y = y0;
    for (size_t x = x0; x < x1; ++x) {
        pointList.push_back({x,y});
        error = error + deltaerr;
        if (error >= 0.5f) {
            y = y + (deltay>0) ? 1 : -1;
            error = error - 1.0;
        }
    }
}

void ImgConverter::writeLineToImg (const Point startPoint, const Point endPoint, std::vector<uint8_t> color) {
    std::shared_ptr<PointList> linePoints;
    getLinePoints(startPoint,endPoint, (*linePoints));
    writePointsToImg (linePoints, color);
}

#endif /* IMGCONVERTER_CPP_ */