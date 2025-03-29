/**
 * @file MyMNISTPixel.h - A class that encapsulates an MNIST image
 * @author Zhou Liu
 */
#include <vector>
#include <random>
#include <algorithm>
#include <array>
#include <iostream>
#include <mpi.h>
#include "MNISTPixel.h"
using namespace std;

MNISTPixel::MNISTPixel(const Pixels pixels) : pixels(pixels) {}

string MNISTPixel::getPixelHex(int row, int col) const {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) {
        cerr << "Error: Index (" << row << ", " << col << ") out of bounds.\n";
        return "000000";  // Default to black
    }
    unsigned char pixel = pixels[ROWS * row + col];
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "%02x%02x%02x", pixel, pixel, pixel);
    return string {buffer};
}

unsigned char MNISTPixel::getPixelValue(int row, int col) const {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) {
        std::cerr << "Error: Index (" << row << ", " << col << ") out of bounds.\n";
        return 0;  // Return default black pixel
    }
    return pixels[row * COLS + col];
}

double MNISTPixel::calculateEuclideanDistance(const MNISTPixel& other) const {
    double result = 0.0;
    for (int i = 0; i < PIXELS_N; ++i) {
        double value = static_cast<double>(pixels[i]) - static_cast<double>(other.pixels[i]);
        result += value * value;
    }
    return sqrt(result);
}






