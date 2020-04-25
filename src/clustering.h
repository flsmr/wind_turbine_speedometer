#ifndef CLUSTERING_H_
#define CLUSTERING_H_
//#define PI = 3.141592653589793238462643383279502884
#include <map>
#include <memory>
class Cluster {
public:
    // Members
    // mean of cluster
    std::vector<double> center;
    // covariance matrix
    std::vector<std::vector<double>> sigma;
    // weighting wrt. other clusters
    double weighting;
    std::shared_ptr<std::vector<std::vector<double>>> cPoints;
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
    // returns the euclidean distance between the mean of this and the provided cluster
    double getClusterDistance(std::shared_ptr<Cluster> &cluster);
    // returns a 1:1 map between the closest clusters in clist1 and clist2 by distance of their centers
    static void matchClusters(const std::vector<std::shared_ptr<Cluster>>&clist1,
        const std::vector<std::shared_ptr<Cluster>>&clist2,
        std::map<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> &cmap);
};


class ClusterModel
{
public:
    // vector of cluster pointers of current cluster model
    std::vector<std::shared_ptr<Cluster>> clusters;
    // vector of points which are to be clustered
    std::vector<std::vector<double>> points;

    // Constructor
    ClusterModel(std::vector<std::vector<double>> points, std::vector<std::shared_ptr<Cluster>> &clusters) : points(points) , clusters(clusters) {};
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
};

#endif // CLUSTERING_H_

