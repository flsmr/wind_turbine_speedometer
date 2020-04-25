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
    std::sort(files.begin(),files.end(),[](std::string a, std::string b){return a>b;});//sort the vector
    for (auto file: files)
        std::cout << "file: " << file << std::endl;




    // MULTITHREADING: LOAD AND CLUSTER IMAGES
    // ======================================
    int width; 
    int height;
    int bpp;
    // initialize image queue
    // - number of max parallel threads
    size_t fileID = 0;
//auto up = make_unique<LongTypeName>(args)
    std::shared_ptr<ParallelImageProcessor<size_t>> pip(new ParallelImageProcessor<size_t>(roi, rgbThreshold, scale, maxThreads));

    std::cout << "Spawning threads..." << std::endl;
    std::vector<std::future<void>> futures;

    // process all files
    while (files.size()>0) {
        std::string file = files.back();
        files.pop_back();
        futures.emplace_back(std::async(std::launch::async, &ParallelImageProcessor<size_t>::processImage, pip, std::move(fileID), file));
        ++fileID;
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

    std::for_each(futures.begin(), futures.end(), [](std::future<void> &ftr) {
        ftr.wait();
    });

*/
std::shared_ptr<std::vector<std::vector<size_t>>> points (new std::vector<std::vector<size_t>>());

// THREAD BARRIER
    // wait for threats

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