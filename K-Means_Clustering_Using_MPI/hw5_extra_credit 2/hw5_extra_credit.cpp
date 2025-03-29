/**
 * @file hw5_extra_credit.cpp
 * @brief Parallel k-means clustering on the MNIST dataset using MPI.
 *
 * This program implements the k-means clustering algorithm on the MNIST dataset
 * using the Message Passing Interface (MPI) for parallel computing.
 * The dataset is distributed across multiple processes to enhance computation speed
 * and generate a visual representation of the clustering results.
 *
 * @author Zhou
 */

/**
 * @section Summary
 * - Implemented k-means clustering with MPI for improved performance.
 * - Encountered challenges in distributing data efficiently among processes.
 * - Debugging MPI communications was difficult due to output synchronization issues.
 * - Future improvements: Optimize data partitioning and explore alternative clustering techniques.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <random>
#include "MNISTKMeansMPI.h"
#include "mpi.h"

using namespace std;

const int IMAGE_MAX = 250;
const int K = 10;
const int ROOT = 0;

const string MNIST_IMAGES_FILEPATH = "./images-idx3-ubyte";
const string MNIST_LABELS_FILEPATH = "./labels-idx1-ubyte";

/**
 * Reads and loads MNIST image data from a binary file.
 * @param images Double pointer to store the loaded image data.
 * @param n Pointer to store the total number of images read.
 */
void loadMNISTImages(MNISTPixel**, int*);

/**
 * Reads and loads MNIST label data from a binary file.
 * @param labels Double pointer to store the label data.
 * @param n Pointer to store the total number of labels read.
 */
void loadMNISTLabels(unsigned char**, int*);

/**
 * Swaps the byte order of a 32-bit integer (Big Endian to Little Endian and vice versa).
 * @param i The integer whose byte order needs to be reversed.
 * @return The byte-reversed integer.
 */
uint32_t swapEndian(uint32_t);

/**
 * Displays the k-means clustering results, showing MNIST labels grouped by clusters.
 * @param clusters The final clusters after convergence.
 * @param labels Pointer to the MNIST label data.
 */
void displayClusters(
    const MNISTKMeansMPI<K, MNISTPixel::getNumPixels()>::Clusters&,
    const unsigned char*
);

/**
 * Creates an HTML visualization of the clustered MNIST images.
 * @param clusters The final clusters after convergence.
 * @param images Pointer to the MNIST image data.
 * @param filename The name of the HTML file to be generated.
 */
void generateHTML(
    const MNISTKMeansMPI<K, MNISTPixel::getNumPixels()>::Clusters&,
    const MNISTPixel*,
    const string&
);

/**
 * Generates an HTML table cell representing a single MNIST image.
 * @param f Reference to the output file stream.
 * @param image The MNIST image to be displayed.
 */
void createHTMLCell(ofstream&, const MNISTPixel&);

/**
 * Generates a random hex color for the HTML background.
 * @return A string representing a random hex color code.
 */
string generateRandomHexColor();

int main() {
    MNISTPixel* images = nullptr;
    unsigned char* labels = nullptr;

    MPI_Init(nullptr, nullptr);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Initialize k-means clustering
    MNISTKMeansMPI<K, MNISTPixel::getNumPixels()> kMeans;

    // Load MNIST data and run clustering on the root process
    if (rank == ROOT) {
        int images_n;
        int labels_n;
        loadMNISTImages(&images, &images_n);
        loadMNISTLabels(&labels, &labels_n);
        kMeans.fit(images, images_n);
    } else {
        kMeans.fitWork(rank);
        MPI_Finalize();
        return 0;
    }

    // Retrieve final clustering results
    MNISTKMeansMPI<K, MNISTPixel::getNumPixels()>::Clusters clusters = kMeans.getClusters();

    // Display and visualize the clustering results
    displayClusters(clusters, labels);
    string filename = "kmeans_mnist_mpi.html";
    generateHTML(clusters, images, filename);
    cout << "\n Visualization complete! Open '" << filename << "' in your browser to explore the clusters. \n\n";

    delete[] images;
    delete[] labels;
    MPI_Finalize();
    return 0;
}

void loadMNISTImages(MNISTPixel** images, int* n) {
    ifstream file(MNIST_IMAGES_FILEPATH, ios::binary);
    if (file.is_open()) {
        uint32_t magicNumber = 0, images_n = 0, rows_n = 0, cols_n = 0;

        file.read((char*)&magicNumber, sizeof(magicNumber));
        file.read((char*)&images_n, sizeof(images_n));
        file.read((char*)&rows_n, sizeof(rows_n));
        file.read((char*)&cols_n, sizeof(cols_n));

        magicNumber = swapEndian(magicNumber);
        images_n = swapEndian(images_n);
        rows_n = swapEndian(rows_n);
        cols_n = swapEndian(cols_n);

        MNISTPixel* imagesData = new MNISTPixel[IMAGE_MAX];
        for (int i = 0; i < IMAGE_MAX; i++) {
            array<unsigned char, MNISTPixel::getNumPixels()> imageData;
            file.read(reinterpret_cast<char*>(imageData.data()), MNISTPixel::getNumPixels());
            imagesData[i] = MNISTPixel(imageData);
        }
        *images = imagesData;
        *n = IMAGE_MAX;
    }
}

void loadMNISTLabels(unsigned char** labels, int* n) {
    ifstream file(MNIST_LABELS_FILEPATH, ios::binary);
    if (file.is_open()) {
        uint32_t magicNumber = 0, labels_n = 0;

        file.read((char*)&magicNumber, sizeof(magicNumber));
        file.read((char*)&labels_n, sizeof(labels_n));

        magicNumber = swapEndian(magicNumber);
        labels_n = swapEndian(labels_n);

        unsigned char* labelsData = new unsigned char[IMAGE_MAX];
        for (int i = 0; i < IMAGE_MAX; i++)
            file.read((char*)&labelsData[i], 1);
        *labels = labelsData;
        *n = IMAGE_MAX;
    }
}

uint32_t swapEndian(uint32_t i) {
    return (i >> 24) | ((i >> 8) & 0x0000FF00) | ((i << 8) & 0x00FF0000) | (i << 24);
}

void displayClusters(
    const MNISTKMeansMPI<K, MNISTPixel::getNumPixels()>::Clusters& clusters,
    const unsigned char* labels
) {
    cout << "\n MNIST Cluster Report:\n";
    for (size_t i = 0; i < clusters.size(); i++) {
        cout << "\n Cluster #" << i + 1 << ":\n";
        for (int j : clusters[i].elements)
            cout << static_cast<int>(labels[j]) << " ";
        cout << endl;
    }
}

void generateHTML(
    const MNISTKMeansMPI<K, MNISTPixel::getNumPixels()>::Clusters& clusters,
    const MNISTPixel* images,
    const string& filename
) {
    ofstream f(filename);
    f << "<body style=\"background:#" << generateRandomHexColor() << ";\">";
    f << "<table><tbody><tr style=\"vertical-align:top;\">\n";
    for (const auto& cluster : clusters) {
        f << "\t<td><table><tbody>\n";
        createHTMLCell(f, cluster.centroid);
        for (const auto& i : cluster.elements)
            createHTMLCell(f, images[i]);
        f << "</tbody></table></td>\n";
    }
    f << "</tr></tbody></table></body>\n";
}

void createHTMLCell(ofstream& f, const MNISTPixel& image) {
    f << "\t\t<tr><td><table style=\"border-collapse:collapse;\"><tbody>\n";
    for (int row = 0; row < MNISTPixel::getNumRows(); row++) {
        f << "\t\t\t<tr>\n";
        for (int col = 0; col < MNISTPixel::getNumCols(); col++) {
            f << "\t\t\t\t<td style=\"background:#" << image.getPixelHex(row, col) << ";";
            f << "width:5px;height:5px;\"></td>\n";
        }
        f << "\t\t\t</tr>\n";
    }
    f << "\t\t</tbody></table></td></tr>\n";
}

string generateRandomHexColor() {
    mt19937 rng(random_device{}());
    uniform_int_distribution<> distrib(0, 255);
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "%02x%02x%02x", distrib(rng), distrib(rng), distrib(rng));
    return string(buffer);
}
