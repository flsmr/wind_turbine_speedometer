#ifndef CLUSTERING_H_
#define CLUSTERING_H_
//#define PI = 3.141592653589793238462643383279502884
#include <map>
class Cluster {
public:
    // Members
    // mean of cluster
    std::vector<double> center;
    // covariance matrix
    std::vector<std::vector<double>> sigma;
    // weighting wrt. other clusters
    double weighting;

    // Constructor
    Cluster();
    Cluster(std::vector<double> center, std::vector<std::vector<double>> sigma,double weighting); 
    /*
    // Destructor
    ~Cluster();
    // Copy Constructor 
    Cluster (const Cluster &src) = delete;
    // Copy Assign Constructor
    Cluster &operator=(const Cluster &src) = delete;
    // Move Constructor 
    Cluster(Cluster &&src) = delete;
    // Move Assign Constructor 
    Cluster &operator=(Cluster &&src) = delete;
    */
    // Functions
    // return the angle in rad of the (major) principal axis ratio and x direction (pos about z)
    double getAngle();
    // return the signal to noise ratio of this cluster
    double getSNR();
    // refit cluster to best fit points
    void maximize(std::vector<std::vector<double>> points, std::vector<double> probabilities);
    // determine probabilities for cluster to generate given set of points
    void expectation(std::vector<std::vector<double>> points, std::vector<double>&prob);
};


class ClusterModel
{
public:
    //std::vector<Cluster> clusters;
    std::multimap <std::shared_ptr<Cluster>, size_t> clu2pnt;
    std::vector<std::shared_ptr<Cluster>> clusters;
    std::vector<std::vector<double>> points;

    // Default constructor
    ClusterModel(std::vector<std::vector<double>> points, std::vector<Cluster> clusters);
    /*
    // Copy constructor
    ClusterModel(const ClusterModel&) = delete;

    // Copy assignment
    ClusterModel& operator=(const ClusterModel&) = delete;

    // Move constructor
    ClusterModel(ClusterModel&&) = delete;

    // Move assignment
    ClusterModel& operator=(ClusterModel&&) = delete;
    */
public:
    // finds best fit for clusters by expectation maximization
    void runClusterFitting();
    // prints a matrix (for debugging purposes)
    static void printMat(const std::vector<std::vector<double>>& mat, std::string title);
    void getClusterPoints(size_t clusterID, std::vector<std::vector<double>> &pointlist);

private:
    // removes any clusters with no points associated
    void deleteEmptyClusters();
};

#endif // CLUSTERING_H_

