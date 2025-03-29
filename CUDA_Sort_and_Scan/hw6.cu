/**
* @file hw6.cu
 * @brief CUDA Bitonic Sort and Parallel Prefix Scan for CSV Data
 *
 * This program sorts `x` values using **Bitonic Sort** and computes the cumulative sum
 * of `y` values using a **Parallel Prefix Scan**. The input is a CSV file with (x, y) pairs,
 * and the output is a sorted CSV with cumulative y-values.
 *
 * @Author: Zhou Liu - Seattle University, CPSC 5600, Winter 2025
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cuda_runtime.h>
#include <stdexcept>
#include <string>
using namespace std;

// Maximum threads per block
const int MAX_BLOCK_SIZE = 1024;

// Struct to hold (x, y) values along with cumulative Y values and original row index
struct X_Y {
    float x, y; // x and y values from CSV file
    float cumulativeY; // Cumulative sum of y values
    size_t originalRow; // Original row index in the CSV file
};

/**
 * @brief CUDA Kernel for Bitonic Sort.
 * @param data Device array of X_Y structures.
 * @param k Outer loop iteration for bitonic sort.
 * @param j Inner loop iteration for bitonic sort.
 * @param size Number of elements in the data array.
 */
__global__ void bitonic(X_Y *data, int k, int j, int size) {
    // Compute global thread index
	int i = blockDim.x * blockIdx.x + threadIdx.x;

    // Compute partner index for comparison
	int ixj = i ^ j;

    // Prevent out-of-bounds memory access
	if(ixj >= size){
		return;
	}

    // Perform Bitonic Sorting comparison and swap
	if (ixj > i) {
	    // Sorting in ascending order for even-indexed blocks
		if ((i & k) == 0 && data[i].x > data[ixj].x)
		{
			X_Y temp = data[i];
			data[i] = data[ixj];
			data[ixj] = temp;
		}
	    // Sorting in descending order for odd-indexed blocks
		if ((i & k) != 0 && data[i].x < data[ixj].x)
		{
			X_Y temp = data[i];
			data[i] = data[ixj];
			data[ixj] = temp;
		}
	}
}

/**
 * @brief CUDA Kernel for Parallel Prefix Scan (Cumulative Sum).
 * @param data Device array of X_Y structures.
 * @param size Number of elements in the data array.
 */
__global__ void scan(X_Y *data, int size, int tier) {
    __shared__ X_Y local[MAX_BLOCK_SIZE];
    if(tier == 1){
        int gindex = blockDim.x * blockIdx.x + threadIdx.x;
        if(gindex >= size){
            return;
        }
        int index = threadIdx.x;
        local[index] = data[gindex];
        local[index].cumulativeY = local[index].y;
        // Inclusive scan within the block
        for (int stride = 1; stride < blockDim.x; stride *= 2) {
            __syncthreads();
            float addend = 0;
            if (stride <= index)
                addend = local[index - stride].cumulativeY;
            __syncthreads();
            local[index].cumulativeY += addend;
        }
        data[gindex] = local[index];
    }
    if(tier == 2){
        int gindex = ((blockDim.x  * (blockDim.x * blockIdx.x + threadIdx.x)) + blockDim.x) - 1;
        if(gindex >= size){
            return;
        }
        int index = threadIdx.x;
        local[index] = data[gindex];
        for (int stride = 1; stride < blockDim.x; stride *= 2) {
            __syncthreads();
            float addend = 0;
            if (stride <= index)
                addend = local[index - stride].cumulativeY;
            __syncthreads();
            local[index].cumulativeY += addend;
        }
        __syncthreads();
        data[gindex] = local[index];
    }
}

/**
 * @brief Kernel for block-level cleanup in scan (propagates sums across blocks).
 * @param data Device array of X_Y structures.
 * @param blockSums Prefix sum of each block.
 * @param size Number of elements in the data array.
 */
__global__ void clean(X_Y *data, int size){
    int gindex = blockDim.x * blockIdx.x + threadIdx.x;
    int lastIndex = blockDim.x * blockIdx.x - 1;
    if(blockIdx.x != 0 && threadIdx.x != (blockDim.x-1)){ // Add previous block's sum
        data[gindex].cumulativeY += data[lastIndex].cumulativeY;
    }
}

/**
 * @brief Reads a CSV file and loads data into a vector.
 * @param filename Name of the CSV file.
 * @param data Vector to store parsed X_Y structures.
 */
void readCSV(const string &filename, vector<X_Y> &data) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(EXIT_FAILURE);
    }

    string line;
	getline(file, line);
    size_t index = 1;
    while (getline(file, line)) {
        istringstream iss(line);
        X_Y point;
        char comma;  // to store the comma between x and y values
        iss >> point.x >> comma >> point.y;
        point.originalRow = index;
        data.push_back(point);
        index++;
    }

    file.close();
}

/**
 * @brief Writes the processed data to an output CSV file.
 * @param filename Output file name.
 * @param data Array of X_Y structures.
 * @param size Number of elements in the data array.
 */
void writeCSV(const string &filename, const X_Y* data, size_t size) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(EXIT_FAILURE);
    }

    file << "x value, y value, cumulative Y value, original row number\n";

    for (size_t i = 0; i < size; i++)
    {
        file << data[i].x << "," << data[i].y << "," << data[i].cumulativeY << "," << data[i].originalRow << endl;
    }

    file.close();
}

/**
 * @brief Error handling function for CUDA API calls.
 * @param status CUDA error code
 */
void auto_throw(cudaError_t status) {
    if (status != cudaSuccess) {
        string message = "ERROR: '";
        message += cudaGetErrorString(status);
        message += "'\n";
        throw runtime_error(message);
    }
}

/**
 * @brief Main function for CUDA-based Bitonic Sort and Prefix Scan.
 */
int main() {
    //Data vector to hold CSV data file
    vector<X_Y> vectorData;
    string filename = "x_y.csv";
    string outFileName = "x_y_scan.csv";
    const int tier = 2;
    readCSV(filename, vectorData);
	X_Y *data; //Holds the CSV data file on GPU

    //Allocate memory on GPU
	auto_throw(cudaMallocManaged(&data, vectorData.size() * sizeof(X_Y)));

    //Divide the number of blocks based on vector data size
    const int numOfBlocks = (vectorData.size() + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;

    // Copy data from vector to CUDA Unified Memory
	for(int i = 0; i < vectorData.size(); i++){
		data[i] = vectorData[i];
	}

    // Perform Bitonic Sort
    for (int k = 2; k <= vectorData.size(); k *= 2) {
        for (int j = k / 2; j > 0; j /= 2) {
            bitonic<<<numOfBlocks, MAX_BLOCK_SIZE>>>(data, k, j, vectorData.size());
            //Synchronize the threads over blocks
            auto_throw(cudaDeviceSynchronize());
        }
    }

    // Perform Prefix Scan
    for(int i = 1; i < tier + 1; i++ ){
        scan<<<numOfBlocks, MAX_BLOCK_SIZE>>>(data, vectorData.size(), i);
        auto_throw(cudaDeviceSynchronize());
    }
    // Perform Cleanup
    clean<<<numOfBlocks, MAX_BLOCK_SIZE>>>(data, vectorData.size());
    auto_throw(cudaDeviceSynchronize());

    // Write results to CSV
    writeCSV(outFileName, data, vectorData.size());

    // Free GPU memory
	cudaFree(data);
	return 0;
}
