#ifndef IMGCONVERTER_H_
#define IMGCONVERTER_H_

#include <string>
#include <memory>
#include <vector>
#include <iostream>

class ImgConverter
{
    public:
    using Point = std::vector<size_t>; // {row Idx, col Idx}
    using PointList = std::vector<Point>;

private:
    const char* _filename;
    uint8_t* _img = NULL;
    size_t _width = 0;
    size_t _height = 0;
    int _nbChannels = 3;

public:
    struct ROI
    {
        size_t minCol;
        size_t maxCol;
        size_t minRow;
        size_t maxRow;
    };

    // Constructors
    ImgConverter();                     
    ImgConverter(std::string filename); 
    // Destructor
    ~ImgConverter();
    // Copy Constructor 
    ImgConverter (const ImgConverter &src) = delete;
    // Copy Assign Constructor
    ImgConverter &operator=(const ImgConverter &src) = delete;
    // Move Constructor 
    ImgConverter(ImgConverter &&src) = delete;
    // Move Assign Constructor 
    ImgConverter &operator=(ImgConverter &&src) = delete;

    // Functions
    // load image from given filename
    void load(const char* filename);

    // save loaded image to filename. If no filename is given, the current file is overwritten
    void save();
    void save(const char* filename);

    // returns true if pixel coordinates are in bound of loaded image
    bool inBound (const Point point);

    // sets pixels with coordinates given in points to specified rgb color in loaded image 
    void writePointsToImg (std::shared_ptr<PointList> points, std::vector<uint8_t> color);

    // returns the rgb value of a pixel in loaded image or {0,0,0} if out of bound
    void getRGBValue(const Point point, std::vector<uint8_t> &rgbVal);

    // sets the rgb value of a pixel in loaded image to specified color
    void setRGBValue(const Point point, const std::vector<uint8_t> &rgbVal);

    // returns a list of points above rgb threshold in defined region of interest
    void getPointsInROIAboveThreshold (const ROI roi, const std::vector<uint8_t> threshold, std::shared_ptr<PointList> points);

    // returns list of points lying on a line between start and endpoint according to Bresenham's line algorithm
    // Pseudocode: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
    void getLinePoints(const Point startPoint, const Point endPoint, PointList &pointList);
         
    // draws a line on the image between start and end point in provided color
    void writeLineToImg (const Point startPoint, const Point endPoint, std::vector<uint8_t> color);
};

#endif /* IMGCONVERTER_H_ */