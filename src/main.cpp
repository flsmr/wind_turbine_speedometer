#include <stdint.h>
#include <vector>

#include "clustering.h"
#include "img_converter.h"

int main() {
    std::vector<int> ROIrow = {0,8000};
    std::vector<int> ROIcol = {0,1200}; //all

    uint8_t threshold = 200;


    // READ FILE
    int width; 
    int height;
    int bpp;

    ImgConverter::ROI roi;
    roi.maxCol = 1200;
    roi.maxRow = 8000;
    roi.minCol = 0;
    roi.minRow = 0;
    ImgConverter imgConv;
    const char* filename = "../img/vid01111.png";
    const char* filename2 = "../img/out.png";
    imgConv.load(filename);
    //std::vector<std::vector<double>>* points = new std::vector<std::vector<double>>;
    //ImgConverter::PointList* points = new ImgConverter::PointList();
    std::vector<std::vector<size_t>>* points = new std::vector<std::vector<size_t>>();
    
    imgConv.getPointsInROIAboveThreshold (roi, {200,200,255}, points);
    std::cout << "num points above threshold: " << points->size() << std::endl;
//    imgConv.writePointsToImg (points, {255,0,0});
//    imgConv.save(filename2);

    std::vector<std::vector<double>> pointsDbl;
    // convert to double
    double meanx = 0.0;
    double meany = 0.0;
    double scale = 300.0;
    double minx = scale;
    double miny = scale;
    double maxx = 0.0;
    double maxy = 0.0;
    for(int i = 0; i < points->size(); ++i) { // points->size()
        pointsDbl.push_back({static_cast<double>((*points)[i][0])/scale,static_cast<double>((*points)[i][1]/scale)});
        //std::cout << "pnt: " << pointsDbl.back()[0]<< " |  " << pointsDbl.back()[1] << std::endl;
        meanx += pointsDbl.back()[0];
        meany += pointsDbl.back()[1];
        minx = (pointsDbl.back()[0] < minx) ? pointsDbl.back()[0]: minx;
        miny = (pointsDbl.back()[0] < miny) ? pointsDbl.back()[0]: miny;
        maxx = (pointsDbl.back()[0] > maxx) ? pointsDbl.back()[0]: maxx;
        maxy = (pointsDbl.back()[0] > maxy) ? pointsDbl.back()[0]: maxy;
    }
    meanx /= pointsDbl.size();
    meany /= pointsDbl.size();
    Cluster cluster1({meanx,meany}, {{1,0},{0,1}}, 1.0/3.0);
    Cluster cluster2({meanx+maxx/2.0,meany}, {{1,0},{0,1}}, 1.0/3.0);
    Cluster cluster3({meanx-maxx/2.0,meany}, {{1,0},{0,1}}, 1.0/3.0);
    std::vector<Cluster> clusterList =  {cluster1,cluster2,cluster3};


    std::cout << "conversion done." << std::endl;
    ClusterModel cm(pointsDbl,clusterList);
    std::cout << "cluster creation done." << std::endl;
    cm.runClusterFitting();
    std::cout << "fitting done." << std::endl;
    std::vector<std::vector<double>> mypoints1;
    std::vector<std::vector<double>> mypoints2;
    std::vector<std::vector<double>> mypoints3;


    std::vector<std::vector<size_t>>* points1 = new std::vector<std::vector<size_t>>();
    std::vector<std::vector<size_t>>* points2 = new std::vector<std::vector<size_t>>();
    std::vector<std::vector<size_t>>* points3 = new std::vector<std::vector<size_t>>();
    cm.getClusterPoints(0,mypoints1);
    cm.getClusterPoints(1,mypoints2);
    cm.getClusterPoints(2,mypoints3);

    // convert back
    for(auto pnt : mypoints1) {
        points1->push_back({static_cast<size_t>(pnt[0]*scale),static_cast<size_t>(pnt[1]*scale)});
    }
    for(auto pnt : mypoints2) {
        points2->push_back({static_cast<size_t>(pnt[0]*scale),static_cast<size_t>(pnt[1]*scale)});
    }
    for(auto pnt : mypoints3) {
        points3->push_back({static_cast<size_t>(pnt[0]*scale),static_cast<size_t>(pnt[1]*scale)});
    }

    imgConv.writePointsToImg (points1, {255,0,0});
    imgConv.writePointsToImg (points2, {0,255,0});
    imgConv.writePointsToImg (points3, {0,0,255});
    imgConv.save(filename2);

    return 0;
}