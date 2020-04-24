#include <stdint.h>
#include <vector>
#include <iostream>
#include <memory>

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
    roi.maxCol = 500;
    roi.maxRow = 500;
    roi.minCol = 0;
    roi.minRow = 0;
    ImgConverter imgConv;
    const char* filename = "../img/vid00001.png";
    const char* filename2 = "../img/out.png";
    imgConv.load(filename);
    //std::vector<std::vector<double>>* points = new std::vector<std::vector<double>>;
    //ImgConverter::PointList* points = new ImgConverter::PointList();
    std::vector<std::vector<size_t>>* points = new std::vector<std::vector<size_t>>();
    
    imgConv.getPointsInROIAboveThreshold (roi, {200,200,255}, points);
    std::cout << "num points above threshold: " << points->size() << std::endl;
    // remove tower
    //(*points).erase
    for (auto it = points->begin(); it !=points->end(); ++it) {
        if ((*it)[1] > 150 && (*it)[1] < 200 && (*it)[0] > 200) {
            // (*it)[0]: rows
            // (*it)[1]: cols 
            points->erase(it);
            --it;
        }
    }
    
//    imgConv.writePointsToImg (points, {255,0,0});
//    imgConv.save(filename2);

    std::vector<std::vector<double>> pointsDbl;
    // convert to double
    double meanx = 0.0;
    double meany = 0.0;
    double scale = 50;//300.0;
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
    Cluster cluster2({meanx,meany+maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0);
    Cluster cluster3({meanx,meany-maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0);
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
    
    std::shared_ptr<Cluster>  c1(new Cluster({meanx,meany}, {{1,0},{0,1}}, 1.0/3.0));
    std::shared_ptr<Cluster>  c2(new Cluster({meanx,meany+maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));
    std::shared_ptr<Cluster>  c3(new Cluster({meanx,meany-maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));

    std::shared_ptr<Cluster>  c4(new Cluster({meanx,meany}, {{1,0},{0,1}}, 1.0/3.0));
    std::shared_ptr<Cluster>  c5(new Cluster({meanx,meany+maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));
    std::shared_ptr<Cluster>  c6(new Cluster({meanx,meany-maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));
    //std::shared_ptr<Cluster> c1 = std
    std::cout << "test1 " << std::endl;
    std::vector<std::shared_ptr<Cluster>> clist1 = {c1,c2,c3};
    std::vector<std::shared_ptr<Cluster>> clist2 = {c6,c5,c4};
    std::cout << "test2 " << std::endl;
    std::map<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> cmap;
    std::cout << "test3 " << std::endl;
    Cluster::matchClusters(clist1,clist2,cmap);
    std::cout << "test4 " << cmap.size() << std::endl;
    std::cout << "center should: " << meany << " is " <<  cmap.find(c1)->second->center[1] <<std::endl;

//    const std::vector<std::shared_ptr<Cluster>>&clist2,
//    std::map<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> &cmap){
    return 0;
}

