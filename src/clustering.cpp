#ifndef CLUSTERING_CPP_
#define CLUSTERING_CPP_
# define PI           3.14159265358979323846
#include <algorithm>
#include <math.h>
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <numeric>
#include <map>

#include "clustering.h"
#include "utility.h"


Cluster::Cluster(std::vector<double> centerIn, std::vector<std::vector<double>> sigmaIn,double weightingIn) {
    center = centerIn;
    sigma = sigmaIn;
    weighting = weightingIn;
    points = std::make_shared<std::vector<std::vector<double>>>();
}
// return the angle in rad of the (major) principal axis ratio and x direction (pos about z)
double Cluster::getAngle() {
    return 0.5 * std::atan(2.0 * sigma[0][1] / (sigma[0][0] - sigma[1][1]));
}

// return the signal to noise ratio of this cluster
double Cluster::getSNR() {
    double a = sigma[0][0];
    double b = sigma[1][1];
    double c = sigma[0][1];
    double d = std::sqrt(std::pow(a - b, 2.0) + 4.0 * std::pow(c, 2.0));
    double eig1  = 0.5 * (a + b + d);
    double eig2  = 0.5 * (a + b - d);
    return eig2 / eig1;
}

void Cluster::expectation(std::vector<std::vector<double>> points, std::vector<double>&probabilities) {
    std::vector<std::vector<double>> sigmaChol{{0,0},{0,0}};
    // Cholesky decomposition of sigma matrix
    Utility::choleskyDecomposition(sigma, sigmaChol);

    std::vector<double> pnt1{0,0};
    std::vector<double> pnt2{0,0};
    
    // normalization constant
    double c1 = 2 * log(2 * PI) + 2 * log(sigmaChol[0][0]) + log(sigmaChol[1][1]);
    // nomalize points:    
    for (size_t i = 0; i < points.size(); ++i)
    {
        pnt1 = {points[i][0] - center[0], points[i][1] - center[1]};
        Utility::backwardSubstitution(sigmaChol, pnt1, pnt2);
        probabilities[i] = -(c1 + pnt2[0] * pnt2[0] + pnt2[1] * pnt2[1]) / 2.0 + log(weighting);
    }
}

void Cluster::maximize(std::vector<std::vector<double>> points, std::vector<double> prob) {
    double sumProb = std::accumulate(prob.begin(), prob.end(), 0.0);

    // calculate new weight
    weighting = sumProb / points.size();

    // calculate new center values
    // 1/sumProbabilities * sum(points * probabilities)
    double c1 =  1.0 / sumProb * 
        std::inner_product(points.begin(), points.end(), prob.begin(), 0.0, std::plus<>(),
        [](auto pnt, auto p) {return pnt[0]*p;});
    double c2 = 1.0 / sumProb * 
        std::inner_product(points.begin(), points.end(), prob.begin(), 0.0, std::plus<>(),
        [](auto pnt, auto p) {return pnt[1]*p;});   
    center = {c1, c2};
 
    // calculate new covariance matrix
    sigma[0][0] = 1.0 / (sumProb + 1.0e-6) * 
        std::inner_product(points.begin(), points.end(), prob.begin(), 0.0, std::plus<>(),
        [c1](auto pnt, auto p) {double pnt1 = (pnt[0] - c1) * sqrt(p);
            return pnt1*pnt1; });
    sigma[0][1] = 1.0 / (sumProb) * 
        std::inner_product(points.begin(), points.end(), prob.begin(), 0.0, std::plus<>(),
        [c1,c2](auto pnt, auto p) {double pnt1 = (pnt[0] - c1) * sqrt(p);double pnt2 = (pnt[1] - c2) * sqrt(p);
            return pnt1*pnt2; });
    sigma[1][0] = sigma[0][1];
    sigma[1][1] = 1.0 / (sumProb + 1.0e-6) * 
        std::inner_product(points.begin(), points.end(), prob.begin(), 0.0, std::plus<>(),
        [c2](auto pnt, auto p) {double pnt2 = (pnt[1] - c2) * sqrt(p);
            return pnt2*pnt2; });
}

double Cluster::getClusterDistance(std::shared_ptr<Cluster> &cluster) {
    double x0 = center[0];
    double y0 = center[1];
    double x1 = cluster->center[0];
    double y1 = cluster->center[1];
    return std::sqrt(std::pow(x0-x1,2)+std::pow(y0-y1,2));
}

void Cluster::matchClusters(const std::vector<std::shared_ptr<Cluster>>&clist1,
    const std::vector<std::shared_ptr<Cluster>>&clist2,
    std::map<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> &cmap){
        std::vector<std::pair<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>>> tuples;
        // generate all possible tuples of clusters
        for(auto &cluster1:clist1) {
            for(auto &cluster2:clist2) {
                tuples.push_back(std::make_pair(cluster1, cluster2));
            }
        }

        //sort by min distance
        std::sort(tuples.begin(),tuples.end(), [](
            std::pair<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> &a,
            std::pair<std::shared_ptr<Cluster>,std::shared_ptr<Cluster>> &b)->bool
            {return a.first->getClusterDistance(a.second) > b.first->getClusterDistance(b.second);});

        // add closest clusters to map
        while(tuples.size() > 0) {
            auto cpair = tuples.back();
            // check if cluster is already in map
            if (cmap.find(cpair.first) == cmap.end()) {
                cmap.insert(cpair);
            }
            tuples.pop_back();
        }
}

// ------------------------------ CLUSTERMODEL -------------------------

ClusterModel::ClusterModel(std::vector<std::vector<double>> pointsIn, std::vector<Cluster> clustersIn)
{
    for (auto cluster: clustersIn) {
        std::shared_ptr<Cluster> clusterPtr = std::make_shared<Cluster>(cluster.center,cluster.sigma, cluster.weighting);
        clusters.push_back(clusterPtr);
    }
    points = pointsIn;
}

void ClusterModel::deleteEmptyClusters()
{
    auto _clu2pnt = clu2pnt;
    /*
    clusters.erase(std::remove_if(clusters.begin(), 
                              clusters.end(),
                              [_clu2pnt](Cluster cluster){return (_clu2pnt.count(cluster) == 0);}),
               clusters.end());
*/
}

void ClusterModel::runClusterFitting()
{
    double tol = 1e-6;
    double likelihood(0);
    double lastLikelihood(0);
    size_t maxIt = 10;
    size_t maxCluster = 0;
    double curMax = 0.0;
    double maxProb;
    std::vector<double> loglikelihood(maxIt, -1e20);

    // initialize point probabilities for clusters
    std::map <std::shared_ptr<Cluster>, std::vector<double>> probs;
    for (auto cluster: clusters) {
        probs.insert(std::make_pair(cluster, std::vector<double>(points.size(),0.0)));
    }
    // Expectation Maximization Algorithm
    for (size_t i = 1; i < maxIt; i++)
    {
        // =======================================================
        // EXPECTATION STEP
        // =======================================================
        // Apply new probability model to dataset and receive new likelihoods
        // EXPECTATIONSTEP();
        double norm(0.0);
        double traceSumLog(0.0);
        double normConst(0.0);
        
        for (auto& cluster: clusters) {
            cluster->expectation(points, probs[cluster]);
            cluster->points->clear();
        }

        // likelihood of all points to appear for the current set of clusters
        double expsum;
        double logsum;
        likelihood = 0;
        clu2pnt.clear();
        for (size_t iPnt = 0; iPnt < points.size(); ++iPnt)
        {
            // determine maximum probability for this point of all clusters
            maxProb = -1e8;
            for (auto& cluster: clusters) {
                maxProb = (probs[cluster][iPnt]>maxProb)?probs[cluster][iPnt] : maxProb;
            }
//        std::cout << "maxProb: "<<maxProb <<std::endl;

            expsum = 0.0;
            // switch to log form
            for (auto cluster: clusters) {
                expsum += exp(probs[cluster][iPnt] - maxProb);
            }

            logsum = maxProb + log(expsum);
//        std::cout << "logsum: "<<logsum <<std::endl;
            likelihood +=logsum;
//        std::cout << "likelihood: "<<likelihood <<std::endl;

            // reverse to exponential form
            //for (auto it = m_probability.begin(); it !=m_probability.end(); ++it)
            for (auto cluster:clusters) {
                probs[cluster][iPnt] = exp(probs[cluster][iPnt] - logsum);
//                std::cout << "new probs: "<<probs[cluster][iPnt] <<std::endl;
            }

            // find out for which cluster the point is most likely
            curMax = 0.0;
            std::shared_ptr<Cluster> maxCluster;
            for (auto& cluster:clusters) {
                if (curMax < probs[cluster][iPnt]) {
                    curMax = probs[cluster][iPnt];
                    maxCluster = cluster;
                    //maxCluster =&cluster - &clusters[0];
                } 
            }
            maxCluster->points->push_back(points[iPnt]);
            // clu2pnt.insert(std::make_pair(clusters[maxCluster], iPnt));
            clu2pnt.insert(std::make_pair(maxCluster, points[iPnt]));
        } 

        // =======================================================
        // REMOVING EMPY CLUSTERS
        // =======================================================
        deleteEmptyClusters();

        // Clusters not changing anymore? => done.
        if (std::fabs(likelihood - lastLikelihood) < tol * std::fabs(likelihood))
            break;
        lastLikelihood = likelihood;

        // =======================================================
        // MAXIMIZATION STEP
        // =======================================================
        for (auto cluster:clusters) {
            cluster->maximize(points, probs[cluster]);
        }
    }
    std::cout << "FITTING RESULTS" <<std::endl;
    std::cout << "num Clusters: "<<clusters.size() <<std::endl;
    Utility::printMatrix(clusters[0]->sigma,"sigma 0:");
    Utility::printMatrix(clusters[1]->sigma,"sigma 1:");
    Utility::printMatrix(clusters[2]->sigma,"sigma 2:");
    Utility::printMatrix({clusters[0]->center,clusters[1]->center,clusters[2]->center},"center 0 1 2:");
}

void ClusterModel::getClusterPoints(const std::shared_ptr<Cluster> &cluster, std::shared_ptr<std::vector<std::vector<double>>> &pointlist) {
    /*
	auto it_range = clu2pnt.equal_range(clusters); //std::pair<MMAPIterator, MMAPIterator>
	for (auto it = it_range.first; it != it_range.second; it++) {
		pointlist.push_back(points[it->second]);
//        std::cout << it->second << ": " << pointlist.back()[0]<< " | " <<pointlist.back()[1]<< std::endl;
    }
	int num = std::distance(it_range.first, it_range.second);
    */
	std::cout << "cluster has  number of points."<< std::endl;
    
}


#endif /* CLUSTERING_CPP_ */