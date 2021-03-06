#include <stdint.h>
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>
#include <cmath>

// file search
#include <glob.h> 

// CSV writing
#include <fstream>

// multi-threading
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <chrono>

#include "clustering.h"
#include "img_converter.h"
#include "parallel_image_processor.h"

# define PI0_5           1.570796327

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "==== WIND TURBINE SPEEDOMETER ====" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;
    unsigned int nCores = std::thread::hardware_concurrency();
    // parallel threads
    size_t maxThreads;
    std::cout << "Your machine supports concurrency with " << nCores << "." << std::endl;
    std::cout << "How many threads should be run in parallel [1-100]?" << std::endl;
    std::cin >> maxThreads;
    if(std::cin.fail()){
        std::cout << "Could not read number of cores (expected a positive number between 1 and 100) " << std::endl;
        return 1;
    } else if(maxThreads < 1 || maxThreads > 100) {
        std::cout << "Could not read number of cores (expected a positive number between 1 and 100) " << std::endl;
        return 1;
    }

    // PARAMETERS
    // ======================================
    // OPT TODO: Create parameter file to read in parameters
    // filenames and folders
    std::string pattern = "../img/*.png";
    std::string csvFileName = "../imgOut/AngularVelocity.csv";
    // fps
    double fps = 30;
    // region of interest for analysis
    ImgConverter::ROI roi;
    roi.maxCol = 500;
    roi.maxRow = 500;
    roi.minCol = 0;
    roi.minRow = 0;

    // rgb threshold above which pixels will be considered for clustering
    std::vector<uint8_t> rgbThreshold {80,250,255};
    double varianceThreshold{1500.0};
    // scale of image points (pixel coordinates will be scaled down to avoid numerical issues in clustering algorithm)
    double scale = 50;//300.0;

    // cluster colors
    std::vector<uint8_t> col1  = {255,0,0};
    std::vector<uint8_t> col2  = {0,255,0};
    std::vector<uint8_t> col3  = {0,0,255};
    std::vector<uint8_t> black = {0,0,0};
    
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

    // MULTITHREADING: LOAD AND CLUSTER IMAGES
    // ======================================
    // initialize image queue
    std::cout << "Analyzing " << files.size() << " images..." << std::endl;
    std::shared_ptr<ParallelImageProcessor<size_t>> pip(new ParallelImageProcessor<size_t>(roi, rgbThreshold, varianceThreshold, scale, maxThreads));
    std::vector<std::future<size_t>> futures;
    // sort the vector of files to assure correct processing order
    std::sort(files.begin(),files.end(),[](std::string a, std::string b){return a < b;}); 
    // start time measurement
    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    // process all files
    for (size_t i = 0; i < files.size(); ++i) {
        std::string file = files[i];
        futures.emplace_back(std::async(&ParallelImageProcessor<size_t>::processImage, pip, std::move(i), file)); //std::launch::async
        std::cout << files.at(i) << " (frameID : " << i << ") is being processed." << std::endl;
        pip->readyForNextImage();
    }

    // PROCESS FRAMES SEQUENTIALLY FOR SPEED ESTIMATION
    // ================================================
    size_t frameID = 0;
    // vector of estimated angular velocities (0 for first frame) [rad/s]
    std::vector<double> avgAngVels{0.0}; 
    std::vector<double> medAngVels{0.0}; 
    std::vector<std::vector<double>> indivAngVels{{0.0,0.0,0.0}}; 
    // maps clusters to color
    std::map<std::shared_ptr<Cluster>,std::vector<uint8_t>> colMap; 
    // sequentially estimate angular velocity from concurrent images 
    while(futures.size()>0) {
        auto &ftr = futures.front();
        ftr.wait();
        frameID = ftr.get();
        futures.erase (futures.begin());
        std::cout << files.at(frameID) << " (frameID : " << frameID << ") finished." << std::endl;

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
            double avgAngVel = 0.0;
            std::vector<double> indivAngVel;
            for (auto &match : cmap) {
                auto cPrev = match.first;
                auto cCur = match.second;

                // get cluster angles and account for angle ranges
                double angPrev = cPrev->getAngle();
                double angCur = cCur->getAngle();
                angCur = (angCur < 0 && angPrev > 0) ? angCur+PI0_5 : angCur;
                double angVel = (angCur-angPrev)*fps;
                indivAngVel.push_back(angVel);
                avgAngVel += angVel;
                // match colors to previous cluster color
                colMap.insert(std::make_pair(cCur, colMap.find(cPrev)->second));
                colMap.erase(cPrev);
            }
            // mean angles
            avgAngVel /= cmap.size();
            avgAngVels.push_back(avgAngVel);
            // median angles
            std::nth_element(indivAngVel.begin(), indivAngVel.begin() + indivAngVel.size()/2, indivAngVel.end());
            medAngVels.push_back(indivAngVel[indivAngVel.size()/2]);
            //individual angles
            indivAngVels.push_back(indivAngVel);
        }
        // color clusters in image and save to output folder
        const char* fn = files[frameID].c_str();
        ImgConverter imgConv;
        imgConv.load(fn);

        for (auto &cluster : cListCur) {
            if (cluster->cPoints->size() > 0) {
                std::shared_ptr<ImgConverter::PointList> pointsImg = std::make_shared<ImgConverter::PointList>();
                for(auto pnt = cluster->cPoints->begin(); pnt != cluster->cPoints->end(); ++pnt) {
                    pointsImg->push_back({static_cast<size_t>(pnt->front()*scale),static_cast<size_t>(pnt->back()*scale)});
                }
                imgConv.writePointsToImg (pointsImg,colMap.find(cluster)->second);
            }
        }
        imgConv.save("../imgOut/out" + std::to_string(frameID) + ".png");
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
    output_stream << "ID" << "," << "filename" 
                          << "," << "Avg Ang Vel [rad/s]"
                          << "," << "Med Ang Vel [rad/s]"
                          << "," << "Ang Vel 1 [rad/s]" 
                          << "," << "Ang Vel 2 [rad/s]" 
                          << "," << "Ang Vel 3 [rad/s]" <<std::endl;
    
    // write det/des performance data to .csv output file line by line
    for (int i = 0; i < files.size(); ++i) {

    output_stream << i
                    << "," << files.at(i)
                    << "," << avgAngVels.at(i)
                    << "," << medAngVels.at(i)
                    << "," << indivAngVels.at(i).at(0)
                    << "," << indivAngVels.at(i).at(1)
                    << "," << indivAngVels.at(i).at(2)
                    << std::endl;
    }
    output_stream.close();

    // print timing results
    std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( endTime - startTime ).count();
    std::cout << "Execution finished after " << duration/1000.0 <<" seconds." << std::endl;

    return 0;
}


