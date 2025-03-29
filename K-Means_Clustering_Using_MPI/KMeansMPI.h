/**
* @file KMeansMPI.h
 * @brief Implementation of k-means clustering using MPI (Message Passing Interface)
 * @author Zhou Liu
 */
#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <array>
#include <iostream>
#include <mpi.h>
using namespace std;

/**
 * @class KMeansMPI
 * @brief Parallel k-means clustering using MPI
 *
 * @tparam k Number of clusters
 * @tparam d Dimensionality of each color data
 */

template <int k, int d>
class KMeansMPI {
public:
    virtual ~KMeansMPI() = default;

    // Definitions
    struct Cluster;
    const int MAX_NUM_GENERATIONS = 300;
    using Element = array<unsigned char, d>; /// Represent a single data point
    using Clusters = array<Cluster, k>; /// Collection of k clusters


    // Debugging flag
    const bool VERBOSE = true;  // set to true for debugging output
#define V(stuff) if(VERBOSE) {using namespace std; stuff}

    /**
     * @struct Cluster
     * @brief Represents a single cluster with a centroid and associated data points.
    */
    struct Cluster {
        vector<int> elements; ///< Indices of elements belonging to the cluster
        Element centroid;  ///< Cluster centroid

        /**
         * @brief Compares centroids of two clusters.
         */
        friend bool operator==(const Cluster& left, const Cluster& right) {
            return left.centroid == right.centroid;  // equality means the same centroid, regardless of elements
        }
    };

    /**
     * @brief Retrieves the clusters computed from the last k-means iteration.
     * @return Reference to the computed clusters.
     */
    virtual const Clusters& getClusters() {
        return clusters;
    }

    /**
     * @brief Runs k-means clustering on the dataset.
     *
     * @param colorList The dataset to cluster.
     * @param n The number of data points in the dataset.
     */
    virtual void fit(const Element* colorList, int n) {
        elements = colorList;
        nColors = n;
        fitWork(ROOT);
    }

    /**
     * Per-process work for fitting
     * @param rank Process rank within MPI_COMM_WORLD
     * @pre n and elements are set in ROOT process; all p processes call fitWork simultaneously
     * @post clusters are now stable (or we gave up after MAX_NUM_GENERATIONS)
     */
    virtual void fitWork(int rank) {
        broadcastSize();
        partitionColors(rank);
        if (rank == ROOT)
            selectClusters();
        distributeCentroids(rank);
        Clusters prev = clusters;
        ++prev[0].centroid[0];  // just to make it different the first time
        for (int generation = 0; generation < MAX_NUM_GENERATIONS; generation++) {
            if (prev == clusters) {
                break;
            }
            V(cout<<rank<<" working on generation "<<generation<<endl;)
            updateDistances();
            prev = clusters;
            updateClusters();
            combineClusters(rank);
            distributeCentroids(rank);
        }
        collectClusterAssignments(rank);
        colorIds = nullptr;
        partition = nullptr;
        delete[] colorIds;
        delete[] partition;

    }

protected:
    const int ROOT = 0;                      /// Total number of MPI processes
    const Element* elements = nullptr;       /// Pointer to input data
    Element* partition = nullptr;            /// Subset of data assigned to the process
    int* colorIds = nullptr;                 /// locally track indices in this->elements
    int nColors = 0;                         /// Total number of data points
    int maxNum = 0;                          /// Maximum number of elements handled per process
    int proccesses = 0;                      /// Total number of MPI processes
    Clusters clusters;                       /// Clustering results
    vector<array<double,k>> dist;            /// Stores distances between points and centroids

    /**
      * @brief Distribute the dataset size across MPI processes.
      */
    virtual void broadcastSize() {
        MPI_Bcast(&nColors, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    }

    /**
     * @brief Distributes dataset among MPI processes using scatter.
     *
     * Each process receives a subset of elements to process. The root process
     * first prepares the data and partitions it accordingly.
     *
     * @param rank MPI rank of the current process.
     */
    virtual void partitionColors(int rank) {
        MPI_Comm_size(MPI_COMM_WORLD, &proccesses);
        unsigned char* sendbuf = nullptr,
        *recvbuf = nullptr;
        int* sendcounts = nullptr,
        *displs = nullptr;
        int colorsEachProcess= nColors / proccesses;

        // Root process prepares data for scattering
        if (rank == ROOT) {
            sendbuf = new unsigned char[nColors * (d + 1)];
            sendcounts = new int[proccesses];
            displs = new int[proccesses];
            int index = 0;
            for (int i = 0; i < nColors; i++) {
                for (int j = 0; j < d; j++)
                    sendbuf[index++] = elements[i][j]; // Store color data
                sendbuf[index++] = (unsigned char)i; // Store index
            }
            // Compute displacement and count for each process
            for (int z = 0; z < proccesses; z++) {
                displs[z] = z * colorsEachProcess* (d + 1);
                sendcounts[z] = colorsEachProcess* (d + 1);
                if (z == proccesses - 1)
                    sendcounts[z] = index - ((proccesses - 1) * colorsEachProcess* (d + 1));
            }
        }

        // Set maxNum for the current process
        maxNum = colorsEachProcess;
        if (rank == proccesses - 1)
            maxNum = nColors - (colorsEachProcess* (proccesses - 1));
        dist.resize(maxNum);

        // Allocate buffer for receiving scattered data
        int recvcount = maxNum * (d + 1);
        recvbuf = new unsigned char[recvcount];

        // Scatter data to all processes
        MPI_Scatterv(
            sendbuf, sendcounts, displs, MPI_UNSIGNED_CHAR,
            recvbuf, recvcount, MPI_UNSIGNED_CHAR,
            ROOT, MPI_COMM_WORLD
        );

        // Unmarshal received data
        partition = new Element[maxNum];
        colorIds = new int[maxNum];
        int index = 0;
        for (int i = 0; i < maxNum; i++) {
            for (int j = 0; j < d; j++)
                partition[i][j] = recvbuf[index++];
            colorIds[i] = (int)recvbuf[index++];
        }

        // Clean up allocated memory
        delete[] sendbuf;
        delete[] recvbuf;
        delete[] sendcounts;
        delete[] displs;
    }

    /**
     * @brief Merges cluster centroids from all MPI processes.
     *
     * Each process sends its cluster centroids to the root process, which then averages
     * the centroids to update the global cluster centroids.
     *
     * @param rank The MPI rank of the current process.
     */
    virtual void combineClusters(int rank) {

        int sendCount = k * (d + 1), recvCount = proccesses * sendCount;
        unsigned char* sendbuf = new unsigned char[sendCount], *recvbuf = nullptr;

        // Serialize local cluster centroids
        int index = 0;
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < d; j++)
                sendbuf[index++] = clusters[i].centroid[j];
            sendbuf[index++] = clusters[i].elements.size();
        }

        if (rank == ROOT)
            recvbuf = new unsigned char[recvCount];

        // Gather all cluster data at the root process
        MPI_Gather(
            sendbuf, sendCount, MPI_UNSIGNED_CHAR,
            recvbuf, sendCount, MPI_UNSIGNED_CHAR,
            ROOT, MPI_COMM_WORLD
        );

        // Root process averages the centroids
        if (rank == ROOT) {
            array<int, k> clusterSizes = {}; // Initialize cluster sizes
            for (int i = 0; i < k; i++)
                clusterSizes[i] = clusters[i].elements.size();

            // Compute the averaged centroids
            index = 0;
            for (int z = 0; z < proccesses; z++)
                for (int i = 0; i < k; i++) {
                    Element centroid = Element{}; // Temporary centroid storage

                    // Extract centroid values
                    for (int j = 0; j < d; j++)
                        centroid[j] = recvbuf[index++]; // Extract cluster size
                    int size = (int)recvbuf[index++];

                    // Update centroid by averaging values
                    updateCentroid(
                        clusters[i].centroid,
                        clusterSizes[i],
                        centroid, size
                    );
                    clusterSizes[i] += size; // Update total count
                }
        }
        // Free allocated memory
        delete[] sendbuf;
        if (rank == ROOT) {
            delete[] recvbuf;
        }
    }

    /**
     * @brief Gather all assigned elements per cluster across MPI processes.
     *
     * Each process sends its assigned cluster elements to the root process,
     * which then consolidates all assignments into the global clusters.
     *
     * @param rank The MPI rank of the current process.
     */
    virtual void collectClusterAssignments(int rank) {
        int sendcount = maxNum + k; // Each process sends its max elements + k cluster sizes
        unsigned char* sendbuf = new unsigned char[sendcount], *recvbuf = nullptr;
        int* recvcounts = nullptr, *displs = nullptr;
        int bufferIndex = 0;

        // Serialize cluster assignments
        for (int i = 0; i < k; i++) {
            sendbuf[bufferIndex++] = (unsigned char)clusters[i].elements.size(); // Store cluster size
            for (const auto& index : clusters[i].elements) {
                sendbuf[bufferIndex++] = static_cast<unsigned char>(colorIds[index]); // Store assigned element indices
            }

        }

        // Root process allocates buffers to collect results
        if (rank == ROOT) {
            recvbuf = new unsigned char[nColors + k * proccesses]; // Allocate buffer to hold all cluster assignments
            recvcounts = new int[proccesses]; // Store number of elements received from each process
            displs = new int[proccesses]; // Store displacement index for MPI_Gatherv

            int colorsEachProcess= nColors / proccesses;

            // Compute displacements and receive counts
            for (int z = 0; z < proccesses; z++) {
                recvcounts[z] = colorsEachProcess+ k;
                if (z == proccesses - 1) {
                    // Last process gets the remaining elements
                    recvcounts[z] = (nColors - (colorsEachProcess* (proccesses - 1))) + k;
                }
                displs[z] = z * (colorsEachProcess+ k);
            }
        }

        // Gather assignments at the root process
        MPI_Gatherv(
            sendbuf, sendcount, MPI_UNSIGNED_CHAR,
            recvbuf, recvcounts, displs, MPI_UNSIGNED_CHAR,
            ROOT, MPI_COMM_WORLD
        );

        // Root process consolidates cluster assignments
        if (rank == ROOT) {
            bufferIndex = 0;
            for (Cluster& cluster : clusters)
                cluster.elements.clear();

            // Deserialize received assignments
            for (int z = 0; z < proccesses; z++)
                for (int i = 0; i < k; i++) {
                    int size = recvbuf[bufferIndex++];
                    for (int e = 0; e < size; e++)
                        clusters[i].elements.push_back(recvbuf[bufferIndex++]);
                }
        }
        // Free allocated memory
        delete[] sendbuf;
        if (rank == ROOT) {
            delete[] recvbuf;
            delete[] recvcounts;
            delete[] displs;
        }
    }

    /**
     * Get the initial cluster centroids.
     * Default implementation here is to just pick k elements at random from the element
     * set
     * @return list of clusters made by using k random elements as the initial centroids
     */
    virtual void selectClusters() {
        vector<int> selectedColors;
        vector<int> indices(nColors);
        iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., nColors-1

        // Correct random number generator usage
        random_device rd;
        mt19937 rng(rd());

        // Randomly sample k unique elements
        sample(indices.begin(), indices.end(), back_inserter(selectedColors), k, rd);

        // Assign selected centroids
        for (int i = 0; i < k; i++) {
            clusters[i].centroid = elements[selectedColors[i]];
            clusters[i].elements.clear();
        }
    }

    /**
   * @brief Broadcast updated cluster centroids to all MPI processes.
   *
   * The root process sends the updated cluster centroids to all other processes
   * to ensure they work with the same centroids in subsequent iterations.
   *
   * @param rank MPI process rank.
   */
    virtual void distributeCentroids(int rank) {
        V(cout<<" "<<rank<<" bcastCentroids"<<endl;)
        int count = k * d;
        unsigned char* buffer = new unsigned char[count];
        if (rank == ROOT) {
            int index = 0;
            for (int i = 0; i < k; i++)
                for (int j = 0; j < d; j++)
                    buffer[index++] = clusters[i].centroid[j];
            V(cout<<" "<<rank<<" sending centroids ";for(int x=0;x<count;x++)printf("%03x ",buffer[x]);cout<<endl;)
        }

        // Broadcast the centroids from the root process
        MPI_Bcast(buffer, count, MPI_UNSIGNED_CHAR, ROOT, MPI_COMM_WORLD);

        if (rank != ROOT) {
            int index = 0;
            for (int i = 0; i < k; i++)
                for (int j = 0; j < d; j++)
                    clusters[i].centroid[j] = buffer[index++];
            V(cout<<" "<<rank<<" receiving centroids ";for(int x=0;x<count;x++)printf("%03x ",buffer[x]);cout<<endl;)
        }
        delete[] buffer;
    }

    /**
   * @brief Compute the distance between two elements.
   *
   * @param first First element.
   * @param second Second element.
   * @return The computed distance.
   */
    virtual double distance(const Element& first, const Element& second) const = 0;

    /**
     * @brief Assigns each element to the nearest cluster and updates centroids.
     *
     * Iterates over all elements and assigns them to the cluster with the smallest distance.
     * Updates centroids dynamically as new elements are added.
     */
    virtual void updateClusters() {
        // Reset cluster elements
        for (int j = 0; j < k; j++) {
            clusters[j].centroid = Element{};
            clusters[j].elements.clear();
        }

        // Assign elements to the closest cluster
        for (int i = 0; i < maxNum; i++) {
            int min = 0;
            for (int j = 1; j < k; j++)
                if (dist[i][j] < dist[i][min])
                    min = j;
            updateCentroid(clusters[min].centroid, clusters[min].elements.size(), partition[i], 1);
            clusters[min].elements.push_back(i);
        }
    }

    /**
    * @brief Updates a centroid by incorporating newly assigned elements.
    *
    * Uses an incremental mean update formula to maintain numerical stability.
    *
    * @param centroid The centroid being updated.
    * @param centroidCount The current number of elements in the cluster.
    * @param newElement The new element being added to the cluster.
    * @param newElementCount The count of new elements being added.
    */
    virtual void updateCentroid(Element& centroid, int centroidCount, const Element& newElement, int newElementCount) const {
        int n = centroidCount + newElementCount;
        for (int i = 0; i < d; i++) {
            double sum = (double)centroid[i] * centroidCount + (double)newElement[i] * newElementCount;
            int size = sum / n;
            centroid[i] = (unsigned char)(size);
        }
    }

    /**
      * @brief Computes the distance between each element and all cluster centroids.
      *
      * Stores the computed distances in `dist`, where `dist[i][j]` represents
      * the distance between `partition[i]` and `clusters[j].centroid`.
      */
    virtual void updateDistances() {
        for (int i = 0; i < maxNum; i++) {
            V(cout<<"distances for "<<i<<"(";for(int x=0;x<d;x++)printf("%02x ",partition[i][x]);)
            for (int j = 0; j < k; j++) {
                dist[i][j] = distance(clusters[j].centroid, partition[i]);
                V(cout<<" " << dist[i][j];)
            }
            V(cout<<endl;)
        }
    }
};