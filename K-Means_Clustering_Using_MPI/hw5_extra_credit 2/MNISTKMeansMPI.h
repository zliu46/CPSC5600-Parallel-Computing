/**
 * @file MNISTClusteringMPI.h - A subclass of KMeansMPI to cluster MNIST images
 * @author Zhou Liu
 */

#pragma once
#include "KMeansMPI.h"
#include "MNISTPixel.h"
#include <iostream>
using namespace std;
/**
 * @class MNIST clustering MPI class using k-means
 * @tparam k the number of clusters for k-means
 * @tparam d the dimensionality of an MNIST image
 */
template<int k, int d>
class MNISTKMeansMPI : public KMeansMPI<k, d> {
public:
    using KMeansMPI<10, 784>::fit;
     /**
     * Run k-means clustering on MNIST images
     * @param data pointer to the MNIST data
     * @param num the number of data
     */
    void fit(MNISTPixel* data, int num) {
        KMeansMPI<k, d>::fit(reinterpret_cast<array<unsigned char, d>*>(data), num);
    }

protected:
    using Element = array<unsigned char, d>;
     /**
     * Euclidean distance between MNIST image data
     * @param x one MNIST data
     * @param y another MNIST data
     * @return distance between x and y
     */
    double distance(const Element& x, const Element& y) const {
        return MNISTPixel(x).calculateEuclideanDistance(MNISTPixel(y));
    }
};

