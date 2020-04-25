#ifndef PARALLELIMAGEPROCESSOR_H_
#define PARALLELIMAGEPROCESSOR_H_

#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <string>
#include <memory>

#include "clustering.h"
#include "img_converter.h"

template <class T>
class ParallelImageProcessor
{
public:
    std::map<size_t,std::vector<std::shared_ptr<Cluster>>> clusterList; // maps frame ID to list of clusters detected in this frame

    // Constructor
    ParallelImageProcessor(ImgConverter::ROI roi, std::vector<uint8_t> rgbThreshold, double scale, size_t maxThreads) : 
        _roi(roi) , _rgbThreshold(rgbThreshold) , _scale(scale), _maxThreads(maxThreads) {}

    T readyForNextImage()
    {
        // perform queue modification under the lock
        std::unique_lock<std::mutex> uLock(_mutex);
//        _cond.wait(uLock, [this] { return !_messages.empty(); }); // pass unique lock to condition variable
        _cond.wait(uLock, [this] { return _runningThreads < _maxThreads; }); // pass unique lock to condition variable

        // remove last vector element from queue
//        T msg = std::move(_messages.back());
//        _messages.pop_back();

        //return msg; // will not be copied due to return value optimization (RVO) in C++
        return _runningThreads;
    }
    // loads image, extracts points and clusters rotorblades

    T processImage(T &&msg, std::string filename)
    {
        std::unique_lock<std::mutex> lck(_mutex );
        std::cout << "processing " << filename << std::endl;
        ++_runningThreads;
        // copy parameters to avoid unnecessary locking/unlocking
        auto roi = _roi;
        auto rgbThreshold = _rgbThreshold;
        auto scale = _scale;
        lck.unlock();
//                std::this_thread::sleep_for(std::chrono::milliseconds(1000));


        // READ IMAGE FILE
        ImgConverter imgConv;
        imgConv.load(filename);
        // std::vector<std::vector<size_t>>* points = new std::vector<std::vector<size_t>>();

        // extracting rotor blade points
        std::shared_ptr<std::vector<std::vector<size_t>>> points (new std::vector<std::vector<size_t>>());

        imgConv.getPointsInROIAboveThreshold (roi, rgbThreshold, points); // DATA RACE!
        std::cout << "num points above threshold: " << points->size() << std::endl;
        // remove tower (awful hack!)
        for (auto it = points->begin(); it !=points->end(); ++it) {
            // (*it)[0]: rows | (*it)[1]: cols 
            if ((*it)[1] > 150 && (*it)[1] < 200 && (*it)[0] > 200) {
                points->erase(it);
                --it;
            }
        }
            
        // INTIALIZE CLUSTERS
        double meanx = 0.0;
        double meany = 0.0;
        double minx = 1e8;//scale;
        double miny = 1e8;//scale;
        double maxx = 0.0;
        double maxy = 0.0;
        std::vector<std::vector<double>> pointsDbl;
        // convert and scale pixel coordinates for clustering
        for(int i = 0; i < points->size(); ++i) {
            pointsDbl.push_back({static_cast<double>((*points)[i][0])/scale,static_cast<double>((*points)[i][1]/scale)});
            meanx += pointsDbl.back()[0];
            meany += pointsDbl.back()[1];
            minx = (pointsDbl.back()[0] < minx) ? pointsDbl.back()[0]: minx;
            miny = (pointsDbl.back()[1] < miny) ? pointsDbl.back()[1]: miny;
            maxx = (pointsDbl.back()[0] > maxx) ? pointsDbl.back()[0]: maxx;
            maxy = (pointsDbl.back()[1] > maxy) ? pointsDbl.back()[1]: maxy;
        }
        meanx /= pointsDbl.size();
        meany /= pointsDbl.size();
        // position 1st cluster in center of extracted points, 2nd and 3rd above / below respectively
        // initialize covariance matrix as identity matrix 
        // weight corresponds to 1 / (number of clusters)
        std::shared_ptr<Cluster> cluster1(new Cluster({meanx,meany}, {{1,0},{0,1}}, 1.0/3.0));
        std::shared_ptr<Cluster> cluster2(new Cluster({meanx,meany+maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));
        std::shared_ptr<Cluster> cluster3(new Cluster({meanx,meany-maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0));
//        auto cluster1p = std::make_shared<Cluster>({meanx,meany}, {{1,0},{0,1}}, 1.0/3.0);
//        auto cluster2p = std::make_shared<Cluster>({meanx,meany+maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0);
//        auto cluster3p = std::make_shared<Cluster>({meanx,meany-maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0);
        std::vector<std::shared_ptr<Cluster>> clusters = {cluster1,cluster2,cluster3};

        // fit clusters to extracted points
        ClusterModel cm(pointsDbl,clusters);
        cm.runClusterFitting();
 
        // Add fitted clusters to list under the lock
//        std::lock_guard<std::mutex> uLock(_mutex);
        lck.lock();
        std::cout << "Frame " << msg << ": Added clusters" <<std::endl;
        clusterList.insert(std::make_pair(msg, cm.clusters)); //also try just clusters as content should have changed
        --_runningThreads;
        _cond.notify_one(); // notify client after pushing new Vehicle into vector
        return msg;
    }

private:
    std::mutex _mutex;
    std::condition_variable _cond;
    std::deque<T> _messages;

    ImgConverter::ROI _roi;
    std::vector<uint8_t> _rgbThreshold{200,200,200};
    double _scale{1};
    size_t _maxThreads{4};
    size_t _runningThreads{0};

    // max threads

    // running threads
};

/*
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
*/

#endif // PARALLELIMAGEPROCESSOR_H_
