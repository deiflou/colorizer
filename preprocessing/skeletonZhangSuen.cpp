#include "skeleton.hpp"

namespace Preprocessing
{

// Add a 1 pixel border and
// copy the inputImage changing white values (255) to 0 and
// black values (0) to 1
static QImage preprocess(const QImage &inputImage)
{
    QImage image = inputImage.copy(-1, -1, inputImage.width() + 2, inputImage.height() + 2);
    for (int y = 1; y < image.height() - 1; ++y) {
        quint8 *currentPixel = static_cast<quint8*>(image.scanLine(y) + 1);
        for (int x = 1; x < image.width() - 1; ++x, ++currentPixel) {
            *currentPixel = (255 - *currentPixel) / 255;
        }
    }
    return image;
}

// Remove the 1 pixel border and
// copy the inputImage changing 0 values to white (255) and
// 1 values to black (0)
static QImage postprocess(const QImage &inputImage)
{
    QImage image = inputImage.copy(1, 1, inputImage.width() - 2, inputImage.height() - 2);
    for (int y = 0; y < image.height(); ++y) {
        quint8 *currentPixel = static_cast<quint8*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x, ++currentPixel) {
            *currentPixel = (1 - *currentPixel) * 255;
        }
    }
    return image;
}

static bool condition1(const quint8 *neighbors)
{
    const int n = neighbors[0] + neighbors[1] + neighbors[2] + neighbors[3] +
                  neighbors[4] + neighbors[5] + neighbors[6] + neighbors[7];
    return n >= 2 && n <= 6;
}

static bool condition2(const quint8 *neighbors)
{
    int n = 0;
    for (int i = 0; i < 7; ++i) {
        if (neighbors[i] == 0 && neighbors[i + 1] == 1) {
            ++n;
        }
    }
    if (neighbors[7] == 0 && neighbors[0] == 1) {
        ++n;
    }
    return n == 1;
}

static bool condition3(int a, int b, int c)
{
    const int n = a * b * c;
    return n == 0;
}

// Since the image only uses values 0 and 1, we use the
// second bit to store if the pixel must be removed
static int subIteration1(QImage &inputImage)
{
    quint8 neighbors[8];
    int n = 0;

    for (int y = 1; y < inputImage.height() - 1; ++y) {
        quint8 *currentPixel = static_cast<quint8*>(inputImage.scanLine(y) + 1);
        for (int x = 1; x < inputImage.width() - 1; ++x, ++currentPixel) {
            if (*currentPixel == 0) {
                continue;
            }

            neighbors[0] = *(currentPixel - inputImage.bytesPerLine()) & 1;
            neighbors[1] = *(currentPixel - inputImage.bytesPerLine() + 1) & 1;
            neighbors[2] = *(currentPixel + 1) & 1;
            neighbors[3] = *(currentPixel + inputImage.bytesPerLine() + 1) & 1;
            neighbors[4] = *(currentPixel + inputImage.bytesPerLine()) & 1;
            neighbors[5] = *(currentPixel + inputImage.bytesPerLine() - 1) & 1;
            neighbors[6] = *(currentPixel - 1) & 1;
            neighbors[7] = *(currentPixel - inputImage.bytesPerLine() - 1) & 1;

            if (condition1(neighbors) && condition2(neighbors) &&
                condition3(neighbors[0], neighbors[2], neighbors[4]) &&
                condition3(neighbors[2], neighbors[4], neighbors[6])) {
                *currentPixel |= 2;
                ++n;
            }
        }
    }
    return n;
}

static int subIteration2(QImage &inputImage)
{
    quint8 neighbors[8];
    int n = 0;

    for (int y = 1; y < inputImage.height() - 1; ++y) {
        quint8 *currentPixel = static_cast<quint8*>(inputImage.scanLine(y) + 1);
        for (int x = 1; x < inputImage.width() - 1; ++x, ++currentPixel) {
            if (*currentPixel == 0) {
                continue;
            }
            
            neighbors[0] = *(currentPixel - inputImage.bytesPerLine()) & 1;
            neighbors[1] = *(currentPixel - inputImage.bytesPerLine() + 1) & 1;
            neighbors[2] = *(currentPixel + 1) & 1;
            neighbors[3] = *(currentPixel + inputImage.bytesPerLine() + 1) & 1;
            neighbors[4] = *(currentPixel + inputImage.bytesPerLine()) & 1;
            neighbors[5] = *(currentPixel + inputImage.bytesPerLine() - 1) & 1;
            neighbors[6] = *(currentPixel - 1) & 1;
            neighbors[7] = *(currentPixel - inputImage.bytesPerLine() - 1) & 1;

            if (condition1(neighbors) && condition2(neighbors) &&
                condition3(neighbors[0], neighbors[2], neighbors[6]) &&
                condition3(neighbors[0], neighbors[4], neighbors[6])) {
                *currentPixel |= 2;
                ++n;
            }
        }
    }
    return n;
}

// If the second bit is set to 1, set the pixel to 0
static void removePixels(QImage &inputImage)
{
    for (int y = 1; y < inputImage.height() - 1; ++y) {
        quint8 *currentPixel = static_cast<quint8*>(inputImage.scanLine(y) + 1);
        for (int x = 1; x < inputImage.width() - 1; ++x, ++currentPixel) {
            *currentPixel = (*currentPixel & 1) & !((*currentPixel & 2) >> 1);
        }
    }
}

QImage skeletonZhangSuen(const QImage &inputImage)
{
    QImage image = preprocess(inputImage);
    
    while (true) {
        int n1, n2;

        n1 = subIteration1(image);
        if (n1 > 0) {
            removePixels(image);
        }

        n2 = subIteration2(image);
        if (n2 > 0) {
            removePixels(image);
        }
        
        if (n1 == 0 && n2 == 0) {
            break;
        }
    }

    return postprocess(image);
}

}
