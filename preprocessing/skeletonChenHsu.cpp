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
    return n >= 2 && n <= 7;
}

static int zeroOneTurns(const quint8 *neighbors)
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
    return n;
}

static bool condition2(const quint8 *neighbors)
{
    return zeroOneTurns(neighbors) == 1;
}

static bool condition3(int a, int b, int c)
{
    const int n = a * b * c;
    return n == 0;
}

static bool condition4(const quint8 *neighbors)
{
    return zeroOneTurns(neighbors) == 2;
}

static bool condition5(int a, int b, int c, int d, int e)
{
    const int n = a * b;
    const int n2 = c + d + e;
    return n == 1 && n2 == 0;
}

// Since the image only uses values 0 and 1, we use the
// second bit to store if the pixel must be removed
static int subIteration(QImage &inputImage, const quint8 *lut, int subiterationNumber)
{
    int n = 0;

    for (int y = 1; y < inputImage.height() - 1; ++y) {
        quint8 *currentPixel = static_cast<quint8*>(inputImage.scanLine(y) + 1);
        for (int x = 1; x < inputImage.width() - 1; ++x, ++currentPixel) {
            if (*currentPixel == 0) {
                continue;
            }

            quint8 index = 0;
            index |= (*(currentPixel - inputImage.bytesPerLine()) & 1) << 0;
            index |= (*(currentPixel - inputImage.bytesPerLine() + 1) & 1) << 1;
            index |= (*(currentPixel + 1) & 1) << 2;
            index |= (*(currentPixel + inputImage.bytesPerLine() + 1) & 1) << 3;
            index |= (*(currentPixel + inputImage.bytesPerLine()) & 1) << 4;
            index |= (*(currentPixel + inputImage.bytesPerLine() - 1) & 1) << 5;
            index |= (*(currentPixel - 1) & 1) << 6;
            index |= (*(currentPixel - inputImage.bytesPerLine() - 1) & 1) << 7;

            if (lut[index] & subiterationNumber) {
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

static void makeLut(quint8 *lut)
{
    quint8 neighbors[8];
    for (int i = 0; i < 256; ++i) {
        lut[i] = 0;

        // The 8 neighbors are encoded in the index.
        neighbors[0] = (i & (1 << 0)) >> 0;
        neighbors[1] = (i & (1 << 1)) >> 1;
        neighbors[2] = (i & (1 << 2)) >> 2;
        neighbors[3] = (i & (1 << 3)) >> 3;
        neighbors[4] = (i & (1 << 4)) >> 4;
        neighbors[5] = (i & (1 << 5)) >> 5;
        neighbors[6] = (i & (1 << 6)) >> 6;
        neighbors[7] = (i & (1 << 7)) >> 7;

        // Sets the first bit to 1 if the conditions for the first subiteration are met.
        if (condition1(neighbors) &&
            (condition2(neighbors) &&
                condition3(neighbors[0], neighbors[2], neighbors[4]) &&
                condition3(neighbors[2], neighbors[4], neighbors[6])) ||
            (condition4(neighbors) &&
                condition5(neighbors[0], neighbors[2], neighbors[4], neighbors[5], neighbors[6]) ||
                condition5(neighbors[2], neighbors[4], neighbors[0], neighbors[6], neighbors[7]))) {
            lut[i] |= 1;
        }

        // Sets the second bit to 1 if the conditions for the second subiteration are met.
        if (condition1(neighbors) &&
            (condition2(neighbors) &&
                condition3(neighbors[0], neighbors[2], neighbors[6]) &&
                condition3(neighbors[0], neighbors[4], neighbors[6])) ||
            (condition4(neighbors) &&
                condition5(neighbors[0], neighbors[6], neighbors[2], neighbors[3], neighbors[4]) ||
                condition5(neighbors[4], neighbors[6], neighbors[0], neighbors[1], neighbors[2]))) {
            lut[i] |= 2;
        }
    }
}

QImage skeletonChenHsu(const QImage &inputImage)
{
    static quint8 lut[256];
    static bool lutReady = false;
    if (!lutReady) {
        makeLut(lut);
        lutReady = true;
    }

    QImage image = preprocess(inputImage);

    while (true) {
        int n1, n2;

        n1 = subIteration(image, lut, 1);
        if (n1 > 0) {
            removePixels(image);
        }

        n2 = subIteration(image, lut, 2);
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
