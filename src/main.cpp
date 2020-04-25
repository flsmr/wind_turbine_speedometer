#include <stdint.h>
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>

// file search
#include <glob.h> 

// CSV writing
#include <fstream>

// multi-threading
#include <thread>
#include <future>
#include <queue>
#include <mutex>

#include "clustering.h"
#include "img_converter.h"
#include "parallel_image_processor.h"

int main() {

    // PARAMETERS
    // ======================================
    unsigned int nCores = std::thread::hardware_concurrency();
    std::cout << "This machine supports concurrency with " << nCores << " cores available" << std::endl;
    // filenames and folders
    std::string pattern = "../img/*.png";
    std::string csvFileName = "../imgOut/AngularVelocity.csv";
    // fps
    double fps = 10;
    // parallel threads
    size_t maxThreads = 2;
    // region of interest
    ImgConverter::ROI roi;
    roi.maxCol = 500;
    roi.maxRow = 500;
    roi.minCol = 0;
    roi.minRow = 0;

    // rgb threshold
    std::vector<uint8_t> rgbThreshold {200,200,255};

    // scale of image points (pixel coordinates will be scaled down to avoid numerical issues in clustering algorithm)
    double scale = 50;//300.0;

    // cluster colors
    std::vector<uint8_t> col1 = {255,0,0};
    std::vector<uint8_t> col2 = {0,255,0};
    std::vector<uint8_t> col3 = {0,0,255};

    // COLLECTING IMAGE FILES IN FOLDER
    // ======================================
    // file reading snippet from stack overflow
    // https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
    glob_t glob_result;
    glob(pattern.c_str(),GLOB_TILDE,NULL,&glob_result);
    std::vector<std::string> files;
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        files.push_back(std::string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    // sort the vector of files to assure correct processing order
    std::sort(files.begin(),files.end(),[](std::string a, std::string b){return a < b;}); 

    // MULTITHREADING: LOAD AND CLUSTER IMAGES
    // ======================================
    // initialize image queue
    std::cout << "Analyzing " << files.size() << " images..." << std::endl;
    std::shared_ptr<ParallelImageProcessor<size_t>> pip(new ParallelImageProcessor<size_t>(roi, rgbThreshold, scale, maxThreads));
    std::vector<std::future<size_t>> futures;
    // process all files
    for (size_t i = 0; i < files.size(); ++i) {
        std::string file = files[i];
        //TODO: avoid data race for futures
        futures.emplace_back(std::async(std::launch::async, &ParallelImageProcessor<size_t>::processImage, pip, std::move(i), file));
        pip->readyForNextImage();
    }

    // PROCESS FRAMES SEQUENTIALLY FOR SPEED ESTIMATION
    // ======================================
    size_t frameID = 0;
    std::vector<double> avgAngVels{0.0}; // vector of estimated angular velocities (0 for first frame) [rad/s]
    std::map<std::shared_ptr<Cluster>,std::vector<uint8_t>> colMap; // maps clusters to color
    while(futures.size()>0) {
        // get first future
        auto &ftr = futures.front();
        ftr.wait();
        frameID = ftr.get();
        std::cout << files.at(frameID) << " (frameID : " << frameID << ") finished." << std::endl;
        futures.erase (futures.begin());

        // match clusters of current and previous frame
        std::vector<std::shared_ptr<Cluster>> cListCur;
        pip->getClusters(frameID, cListCur);

        if (frameID == 0) {
            // color mapping  rgbThreshold {200,200,255}
            colMap.insert(std::make_pair(cListCur[0], col1));
            colMap.insert(std::make_pair(cListCur[1], col2));
            colMap.insert(std::make_pair(cListCur[2], col3));
        } else {
            std::vector<std::shared_ptr<Cluster>> cListPrev;
            pip->getClusters(frameID-1, cListPrev);

            std::map<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> cmap;
            Cluster::matchClusters(cListPrev,cListCur,cmap);
            double avgAngVel = 0;
            for (auto &match : cmap) {
                auto cPrev = match.first;
                auto cCur = match.second;
                // OPT TODO: check cluster size for plausibility
                double angPrev = cPrev->getAngle();
                double angCur = cCur->getAngle();
                // OPT TODO: check angle difference for plausibility
                // OPT TODO: apply modulo for angle difference
                double angVel = angCur-angPrev;
                //std::cout << "cur speed: " << angVel << std::endl;
                avgAngVel += angVel/fps;
                // match colors to previous cluster color
                std::vector<uint8_t> prevColor = colMap.find(cPrev)->second;
                colMap.insert(std::make_pair(cCur, prevColor));
                colMap.erase(cPrev);
            }
            avgAngVel /= cmap.size();
            avgAngVels.push_back(avgAngVel);

            // color clusters in image and save to output folder
            const char* fn = files[frameID].c_str();
            ImgConverter imgConv;
            imgConv.load(fn);
            for (auto &cluster : cListCur) {
                std::shared_ptr<ImgConverter::PointList> pointsImg = std::make_shared<ImgConverter::PointList>();
                for(auto pnt = cluster->points->begin(); pnt != cluster->points->end(); ++pnt) {
                    pointsImg->push_back({static_cast<size_t>(pnt->front()*scale),static_cast<size_t>(pnt->back()*scale)});
                }
                imgConv.writePointsToImg (pointsImg, colMap.find(cluster)->second);
            }
            imgConv.save("../imgOut/out" + std::to_string(frameID) + ".png");
        }
    }

    // WRITE RESULTS TO CSV FILE
    // ======================================
    std::cout << "Writing to CSV: " << csvFileName << std::endl;
    std::ofstream output_stream(csvFileName, std::ios::binary);
    if (!output_stream.is_open()) {
        std::cerr << "failed to open file: " << csvFileName << std::endl;
        return EXIT_FAILURE;
    }
   
    // write CSV header row
    output_stream << "ID" << "," << "filename" << "," << "Avg Ang Vel [rad/s]" <<std::endl;
    
    // write det/des performance data to .csv output file line by line
    for (int i = 0; i < files.size(); ++i) {

    output_stream << i
                    << "," << files.at(i)
                    << "," << avgAngVels.at(i)
                    << std::endl;
    }
    output_stream.close();

    std::cout << "done."<< std::endl;

    return 0;
}


