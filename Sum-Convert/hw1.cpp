/**
*@author Zhou Liu
* @file hw1.cpp
 * @brief Multithreaded prefix sum computation with encoding and decoding.
 * @see "Seattle University, CPSC5600, Winter 2025"
 *
 * This program uses two threads to speed up the encoding and decoding
 * of an array. The prefix sum (cumulative sum) is calculated in the
 * main thread to ensure correctness.
 */
#include "hw1.h"
#include <iostream>
#include "ThreadGroup.h"
using namespace std;

int encode(int v) {
    // do something time-consuming (and arbitrary)
    for (int i = 0; i < 500; i++)
        v = ((v * v) + v) % 10;
    return v;
}

int decode(int v) {
    // do something time-consuming (and arbitrary)
    return encode(v);
}

/**
 * @struct ThreadData
 * @brief Holds the shared data and settings for threads.
 *
 * This structure is passed to threads so they can access
 * the array, its size, and their assigned work.
 */
struct ThreadData {
    int *data;            // Pointer to the array
    int length;           // Total number of elements in the array
    int numThreads;       // Number of threads to use
    int segSize;          // Number of elements each thread processes
};
/**
 * @class EncodeThread
 * @brief A task that encodes part of the array in parallel.
 *
 * Each thread processes a specific segment of the array,
 * encoding the numbers in its range.
 */
class EncodeThread {
public:
    /**
     * @brief Encodes a segment of the array.
     * @param id The thread's unique ID (used to calculate its range).
     * @param sharedData A pointer to shared data for the threads.
     */
    void operator()(int id, void *sharedData) const {
        const auto *threadData = static_cast<ThreadData *>(sharedData);

        // Calculate the range of the array this thread will process
        const int start = id * threadData->segSize;
        const int end = (id == threadData->numThreads - 1) ? threadData->length : start + threadData->segSize;

        // Encode the elements in the assigned range
        for (int i = start; i < end; i++) {
            threadData->data[i] = encode(threadData->data[i]);
        }
    }
};

/**
 * @class DecodeThread
 * @brief A task that decodes part of the array in parallel.
 *
 * Each thread processes a specific segment of the array,
 * decoding the numbers in its range.
 */
class DecodeThread {
public:
    /**
     * @brief Decodes a segment of the array.
     * @param id The thread's unique ID (used to calculate its range).
     * @param sharedData A pointer to shared data for the threads.
     */
    void operator()(int id, void *sharedData) const {
        const auto *threadData = static_cast<ThreadData *>(sharedData);

        // Calculate the range of the array this thread will process
        const int start = id * threadData->segSize;
        const int end = (id == threadData->numThreads - 1) ? threadData->length : start + threadData->segSize;

        // Decode the elements in the assigned range
        for (int i = start; i < end; i++) {
            threadData->data[i] = decode(threadData->data[i]);
        }
    }
};

/**
 * @brief Computes the prefix sum of an array with encoding and decoding.
 *
 * This function:
 * - Encodes the array in parallel using threads.
 * - Computes the prefix sum (cumulative sum) in the main thread.
 * - Decodes the array in parallel using threads.
 *
 * @param data Pointer to the array.
 * @param length Total number of elements in the array.
 */
void prefixSums(int *data, int length) {
    const int numThreads = 2; // Use two threads for encoding and decoding

    // Set up shared data for threads
    ThreadData threadData = {};
    threadData.data = data;
    threadData.length = length;
    threadData.numThreads = numThreads;
    threadData.segSize = length / numThreads;

    //Encode the array in parallel
    ThreadGroup<EncodeThread> encoders;
    encoders.createThread(0, &threadData);
    encoders.createThread(1, &threadData);
    encoders.waitForAll();

    // Compute the prefix sum (cumulative sum) in the main thread
    int encodedSum = 0;
    for (int i = 0; i < length; i++) {
        encodedSum += data[i];
        data[i] = encodedSum; // Update the array with the cumulative sum
    }

    // Decode the array in parallel
    ThreadGroup<DecodeThread> decoders;
    decoders.createThread(0, &threadData);
    decoders.createThread(1, &threadData);
    decoders.waitForAll();
}

/**
 * @brief Main function to test the prefix sum computation.
 */
int main() {
    int length = 1000 * 1000; // Array size

    // make array
    int *data = new int[length];
    for (int i = 1; i < length; i++)
        data[i] = 1;
    data[0] = 6;

    // transform array into converted/deconverted prefix sum of original
    prefixSums(data, length);

    // printed out result is 6, 6, and 2 when data[0] is 6 to start and the rest 1
    cout << "[0]: " << data[0] << endl
            << "[" << length/2 << "]: " << data[length/2] << endl
            << "[end]: " << data[length-1] << endl;

    delete[] data;
    return 0;
}
