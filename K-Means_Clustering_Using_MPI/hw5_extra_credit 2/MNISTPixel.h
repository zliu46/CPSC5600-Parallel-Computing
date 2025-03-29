/**
* @file MNISTPixel.h - a class that encapsulates an MNIST image
 * @author Zhou Liu
 */

#pragma once
#include <string>
#include <array>
#include <iostream>
using namespace std;
/**
 * @class Wrapper class for an MNIST image
 */
class MNISTPixel {
public:
    static const int ROWS = 28;
    static const int COLS = 28;
    static const int PIXELS_N = COLS * ROWS;
    using Pixels = array<unsigned char, PIXELS_N>;

private:
    Pixels pixels;

public:
    /**
    * Constructors
    */

    MNISTPixel() {}
    MNISTPixel(Pixels pixels);

    /**
     * Converts a pixel value to a hexadecimal string.
     * @param row The row index of the pixel.
     * @param col The column index of the pixel.
     * @return Hexadecimal representation of the grayscale pixel value.
     */
    string getPixelHex(int row, int col) const;

    /**
     * Retrieves the pixel value at a specific row and column.
     * @param row The row index of the pixel.
     * @param col The column index of the pixel.
     * @return The grayscale pixel value (0-255).
     */
    unsigned char getPixelValue(int row, int col) const;

    /**
    * Computes the Euclidean distance between two MNIST images.
    * @param other Another MyMNISTPixel instance to compare against.
    * @return Euclidean distance between the current image and the other.
    */
    double calculateEuclideanDistance(const MNISTPixel& other) const;

    /**
     * Static accessors for MNIST image dimensions.
     */
    constexpr static int getNumRows() { return ROWS; }
    constexpr static int getNumCols() { return COLS; }
    constexpr static int getNumPixels() { return PIXELS_N; }
};


