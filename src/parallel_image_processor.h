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
    // Constructor
    ParallelImageProcessor(ImgConverter::ROI roi, std::vector<uint8_t> rgbThreshold, double scale, size_t maxThreads) : 
        _roi(roi) , _rgbThreshold(rgbThreshold) , _scale(scale), _maxThreads(maxThreads) {}

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
    // loads image, extracts points and clusters rotorblades

    void processImage(T &&msg, std::string filename)
    {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

/*
        // READ IMAGE FILE
        const char* fn = filename.c_str();
        ImgConverter imgConv;
        imgConv.load(fn);
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
        Cluster cluster1({meanx,meany}, {{1,0},{0,1}}, 1.0/3.0);
        Cluster cluster2({meanx,meany+maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0);
        Cluster cluster3({meanx,meany-maxy/2.0}, {{1,0},{0,1}}, 1.0/3.0);
        std::vector<Cluster> clusterList =  {cluster1,cluster2,cluster3};
        ClusterModel cm(pointsDbl,clusterList);
        // fit clusters to extracted points
        cm.runClusterFitting();


        // convert back
        // TODO: create single for loop
        std::vector<std::vector<double>> mypoints1;
        std::vector<std::vector<double>> mypoints2;
        std::vector<std::vector<double>> mypoints3;
        std::vector<std::vector<size_t>>* points1 = new std::vector<std::vector<size_t>>();
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
*/        
        // perform vector modification under the lock
        std::lock_guard<std::mutex> uLock(_mutex);
        // add points to cluster points
        // add clusters to cluster list

        // add vector to queue
        std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;
        _messages.push_back(std::move(msg));
        _cond.notify_one(); // notify client after pushing new Vehicle into vector
    }

private:
    std::mutex _mutex;
    std::condition_variable _cond;
    std::deque<T> _messages;

    ImgConverter::ROI _roi;
    std::vector<uint8_t> _rgbThreshold;
    double _scale;
    size_t _maxThreads;
    std::map<std::shared_ptr<Cluster>,std::vector<std::vector<double>>> _clusterPoints; // should be changed to 
    std::map<size_t,std::shared_ptr<Cluster>> _clusterList; // maps frame ID to list of clusters detected in this frame

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
