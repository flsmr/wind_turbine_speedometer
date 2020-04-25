#include <stdint.h>
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>
#include <glob.h> // file search

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
    // image folder
    std::string pattern = "../img/*.png";

    // fps

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
    std::cout << " number of files: " << files.size() << std::endl;
    std::sort(files.begin(),files.end(),[](std::string a, std::string b){return a>b;}); //sort the vector
    for (auto file: files)
        std::cout << "file: " << file << std::endl;

    // MULTITHREADING: LOAD AND CLUSTER IMAGES
    // ======================================
    // initialize image queue
    // - number of max parallel threads
//auto up = make_unique<LongTypeName>(args)
    std::shared_ptr<ParallelImageProcessor<size_t>> pip(new ParallelImageProcessor<size_t>(roi, rgbThreshold, scale, maxThreads));

    std::cout << "Spawning threads..." << std::endl;
    std::vector<std::future<size_t>> futures;
    // process all files
    for (size_t i = 0; i < files.size(); ++i) {
        std::string file = files[i];
        //TODO: avoid data race for futures
        futures.emplace_back(std::async(std::launch::async, &ParallelImageProcessor<size_t>::processImage, pip, std::move(i), file));
        std::cout << "   waiting" << std::endl;
        size_t message = pip->readyForNextImage();
        std::cout << "   num tasks #" << message << " finished " << std::endl;
    }
    std::cout << "totally finished " << std::endl;

    // PROCESS FRAMES SEQUENTIALLY FOR SPEED ESTIMATION
    // ======================================
    size_t frameID = 0;
    std::map<std::shared_ptr<Cluster>,std::vector<uint8_t>> colMap; // maps clusters to color
    while(futures.size()>0) {
        // get first future
        auto &ftr = futures.front();
        ftr.wait();
        frameID = ftr.get();
        std::cout << "future " << frameID << " finished " << std::endl;
        futures.erase (futures.begin());

        // match clusters of current and previous frame
        //TODO perform under the lock
        std::vector<std::shared_ptr<Cluster>> cListCur = pip->clusterList.find(frameID)->second;

        if (frameID == 0) {
            // color mapping  rgbThreshold {200,200,255}
            colMap.insert(std::make_pair(cListCur[0], col1));
            colMap.insert(std::make_pair(cListCur[1], col2));
            colMap.insert(std::make_pair(cListCur[2], col3));
        } else {
            //TODO perform under the lock
            std::vector<std::shared_ptr<Cluster>> cListPrev = pip->clusterList.find(frameID-1)->second;

            std::map<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> cmap;
            Cluster::matchClusters(cListPrev,cListCur,cmap);
            double avgAngVel = 0;
            for (auto &match : cmap) {
                auto cPrev = match.first;
                auto cCur = match.second;
                //TODO: check cluster size for plausibility
                double angPrev = cPrev->getAngle();
                double angCur = cCur->getAngle();
                //TODO: check angle difference for plausibility
                //TODO: apply modulo for angle difference
                double angVel = angCur-angPrev;
                std::cout << "cur speed: " << angVel << std::endl;
                avgAngVel += angVel;
                // match colors to previous cluster color
                std::vector<uint8_t> prevColor = colMap.find(cPrev)->second;
                colMap.insert(std::make_pair(cCur, prevColor));
                colMap.erase(cPrev);
            }
            avgAngVel /= cmap.size();

            // color clusters in image and save to output folder
            const char* fn = files[frameID].c_str();
            ImgConverter imgConv;
            imgConv.load(fn);
            for (auto &cluster : cListCur) {
                std::cout << "converting cluster with points: " << cluster->points->size() << std::endl;
                std::shared_ptr<ImgConverter::PointList> pointsImg = std::make_shared<ImgConverter::PointList>();
                std::cout << "pointsImg: " << pointsImg->size() << std::endl;
                for(auto pnt = cluster->points->begin(); pnt != cluster->points->end(); ++pnt) {
                    std::cout << "converting point " << static_cast<size_t>(pnt->front()*scale) << std::endl;
                    pointsImg->push_back({static_cast<size_t>(pnt->front()*scale),static_cast<size_t>(pnt->back()*scale)});
                }
                imgConv.writePointsToImg (pointsImg, colMap.find(cluster)->second);
            }
            imgConv.save("../imgOut/out" + std::to_string(frameID) + ".png");
        }
/*
        // convert back
        // TODO: create single for loop
        std::vector<std::vector<double>> mypoints2;
        std::vector<std::vector<double>> mypoints3;
        std::vector<std::vector<size_t>>* points2 = new std::vector<std::vector<size_t>>();
        std::vector<std::vector<size_t>>* points3 = new std::vector<std::vector<size_t>>();
        cm.getClusterPoints(0,mypoints1);
        cm.getClusterPoints(1,mypoints2);
        cm.getClusterPoints(2,mypoints3);
        for(auto pnt : mypoints1) {
            points1->push_back({static_cast<size_t>(pnt[0]*scale),static_cast<size_t>(pnt[1]*scale)});
        }
        for(auto pnt : mypoints2) {
            points2->push_back({static_cast<size_t>(pnt[0]*scale),static_cast<size_t>(pnt[1]*scale)});
        }
        for(auto pnt : mypoints3) {
            points3->push_back({static_cast<size_t>(pnt[0]*scale),static_cast<size_t>(pnt[1]*scale)});
        }




            // write output to file
    // create ringbuffer
    // add newest cluster
    // if length == 1
        // set colors for clusters (via map)
        // color image and save output
        // save angles (via map)
    // if length == 2
        // match clusters
        // swap color map to new clusters
        // color image and save output
        // for each map check distance and number of points for plausibility 
        // calc delta angle => angular velocity for frame
        // imprint value to image and save output

        }
        // delete first future from future list
    */

    }
/*    

    // create monitor object as a shared pointer to enable access by multiple threads

    std::cout << "Collecting results..." << std::endl;
    while (true)
    {
        int message = queue->receive();
        std::cout << "   Message #" << message << " has been removed from the queue" << std::endl;

        std::shared_ptr<Cluster>  c4(new Cluster({meanx,meany}, {{1,0},{0,1}}, 1.0/3.0));
        std::shared_ptr<Cluster>  c5(new Cluster({meanx,meany+maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));
        std::shared_ptr<Cluster>  c6(new Cluster({meanx,meany-maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));

    std::vector<std::shared_ptr<Cluster>> clist1 = {c1,c2,c3};
    std::vector<std::shared_ptr<Cluster>> clist2 = {c6,c5,c4};
    std::map<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> cmap;
    Cluster::matchClusters(clist1,clist2,cmap);
    std::cout << "center should: " << meany << " is " <<  cmap.find(c1)->second->center[1] <<std::endl;

    }
            std::shared_ptr<Cluster>  c1(new Cluster({meanx,meany}, {{1,0},{0,1}}, 1.0/3.0));
            std::shared_ptr<Cluster>  c2(new Cluster({meanx,meany+maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));
            std::shared_ptr<Cluster>  c3(new Cluster({meanx,meany-maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));

            std::shared_ptr<Cluster>  c4(new Cluster({meanx,meany}, {{1,0},{0,1}}, 1.0/3.0));
            std::shared_ptr<Cluster>  c5(new Cluster({meanx,meany+maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));
            std::shared_ptr<Cluster>  c6(new Cluster({meanx,meany-maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));
            std::vector<std::shared_ptr<Cluster>> clist1 = {c1,c2,c3};
            std::vector<std::shared_ptr<Cluster>> clist2 = {c6,c5,c4};


*/
std::shared_ptr<std::vector<std::vector<size_t>>> points (new std::vector<std::vector<size_t>>());

// THREAD BARRIER
    // wait for threats

    // run clustering on rotor blade points

//    imgConv.writePointsToImg (points, {255,0,0});
//    imgConv.save(filename2);


/*
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

    std::vector<std::shared_ptr<Cluster>> clist1 = {c1,c2,c3};
    std::vector<std::shared_ptr<Cluster>> clist2 = {c6,c5,c4};
    std::map<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> cmap;
    Cluster::matchClusters(clist1,clist2,cmap);
    std::cout << "center should: " << meany << " is " <<  cmap.find(c1)->second->center[1] <<std::endl;
*/
//    const std::vector<std::shared_ptr<Cluster>>&clist2,
//    std::map<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> &cmap){
    return 0;
}

/*
template <class T>
class MessageQueue
{
public:
    T receive()
    {
        // perform queue modification under the lock
        std::unique_lock<std::mutex> uLock(_mutex);
        _cond.wait(uLock, [this] { return !_messages.empty(); }); // pass unique lock to condition variable

        // remove last vector element from queue
        T msg = std::move(_messages.back());
        _messages.pop_back();

        return msg; // will not be copied due to return value optimization (RVO) in C++
    }

    void send(T &&msg)
    {
        // simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // perform vector modification under the lock
        std::lock_guard<std::mutex> uLock(_mutex);

        // add vector to queue
        std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;
        _messages.push_back(std::move(msg));
        _cond.notify_one(); // notify client after pushing new Vehicle into vector
    }

private:
    std::mutex _mutex;
    std::condition_variable _cond;
    std::deque<T> _messages;
};

int main()
{
    // create monitor object as a shared pointer to enable access by multiple threads
    std::shared_ptr<MessageQueue<int>> queue(new MessageQueue<int>);

    std::cout << "Spawning threads..." << std::endl;
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 10; ++i)
    {
        int message = i;
        futures.emplace_back(std::async(std::launch::async, &MessageQueue<int>::send, queue, std::move(message)));
    }

    std::cout << "Collecting results..." << std::endl;
    while (true)
    {
        int message = queue->receive();
        std::cout << "   Message #" << message << " has been removed from the queue" << std::endl;
    }

    std::for_each(futures.begin(), futures.end(), [](std::future<void> &ftr) {
        ftr.wait();
    });

    std::cout << "Finished!" << std::endl;

    return 0;
}

/* MINIMAL EXAMPLE MULTITHREADING
#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <mutex>
#include <algorithm>

class Vehicle
{
public:
    Vehicle(int id) : _id(id) {}
    int getID() { return _id; }

private:
    int _id;
};

class WaitingVehicles
{
public:
    WaitingVehicles() {}

    void printIDs()
    {
        std::lock_guard<std::mutex> myLock(_mutex); // lock is released when myLock goes out of scope
        for(auto &v : _vehicles)
            std::cout << "   Vehicle #" << v.getID() << " is now waiting in the queue" << std::endl;
        
    }

    int pushBack(Vehicle &&v)
    {
        // perform vector modification under the lock
        std::lock_guard<std::mutex> uLock(_mutex);
        std::cout << "   Vehicle #" << v.getID() << " will be added to the queue" << std::endl; 
        _vehicles.emplace_back(std::move(v));

        // simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        return v.getID();
    }

private:
    std::vector<Vehicle> _vehicles; // list of all vehicles waiting to enter this intersection
    std::mutex _mutex;
};

int main()
{
    // create monitor object as a shared pointer to enable access by multiple threads
    std::shared_ptr<WaitingVehicles> queue(new WaitingVehicles);

    std::cout << "Spawning threads..." << std::endl;
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i)
    {
        // create a new Vehicle instance and move it into the queue
        Vehicle v(i);
        futures.emplace_back(std::async(std::launch::async, &WaitingVehicles::pushBack, queue, std::move(v)));
    }

    std::for_each(futures.begin(), futures.end(), [](std::future<int > &ftr) {
        ftr.wait();
        std::cout << "received something from "<< ftr.get() << std::endl;
    });

    std::cout << "Collecting results..." << std::endl;
    queue->printIDs();

    return 0;
}
*/