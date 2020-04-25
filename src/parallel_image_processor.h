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

    // limit the number of threads running in parallel
    void readyForNextImage()
    {
        std::unique_lock<std::mutex> uLock(_mutex);
        _cond.wait(uLock, [this] { return _runningThreads < _maxThreads; });
    }

    // loads image, extracts points and clusters rotorblades
    T processImage(T &&msg, std::string filename)
    {
        std::unique_lock<std::mutex> lck(_mutex );
        std::cout << filename << " (frameID : " << msg << ") is being processed." << std::endl;
        ++_runningThreads;
        // copy parameters to avoid unnecessary locking/unlocking
        auto roi = _roi;
        auto rgbThreshold = _rgbThreshold;
        auto scale = _scale;
        lck.unlock();

        // load image file
        ImgConverter imgConv;
        imgConv.load(filename);

        // extracting rotor blade points
        std::shared_ptr<std::vector<std::vector<size_t>>> points (new std::vector<std::vector<size_t>>());
        imgConv.getPointsInROIAboveThreshold (roi, rgbThreshold, points); // DATA RACE!
        // remove tower (awful hack - but makes life easier for the first shot!)
        // OPT TODO: add 4th cluster to "catch" tower and ignore "non-moving" clusters
        for (auto it = points->begin(); it !=points->end(); ++it) {
            // (*it)[0]: rows | (*it)[1]: cols 
            if ((*it)[1] > 150 && (*it)[1] < 200 && (*it)[0] > 200) {
                points->erase(it);
                --it;
            }
        }
            
        // initialize clusters
        double meanx = 0.0;
        double meany = 0.0;
        double minx  = 1e8;
        double miny  = 1e8;
        double maxx  = 0.0;
        double maxy  = 0.0;
        // convert and scale pixel coordinates for clustering
        std::vector<std::vector<double>> pointsDbl;
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
        std::vector<std::shared_ptr<Cluster>> clusters = {cluster1,cluster2,cluster3};

        // fit clusters to extracted points
        ClusterModel cm(pointsDbl,clusters);
        cm.runClusterFitting();
 
        // Add fitted clusters to list (under the lock)
        lck.lock();
        _clusterList.insert(std::make_pair(msg, cm.clusters));
        --_runningThreads;
        _cond.notify_one();
        return msg;
    }

    // returns the clusters identified in the given frame
    void getClusters(const  size_t frameID, std::vector<std::shared_ptr<Cluster>> &clusters)
    {
        std::unique_lock<std::mutex> uLock(_mutex);
        clusters = _clusterList.find(frameID)->second;
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
    // maps frame ID to list of clusters detected in this frame
    std::map<size_t,std::vector<std::shared_ptr<Cluster>>> _clusterList;
};

#endif // PARALLELIMAGEPROCESSOR_H_
