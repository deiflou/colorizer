#include "threshold.hpp"

namespace Preprocessing
{

QImage threshold(const QImage &inputImage, int v)
{
    QImage outpuImage(inputImage.width(), inputImage.height(), inputImage.format());
    for (int y = 0; y < inputImage.height(); ++y) {
        const quint8 *currentPixel1 = static_cast<const quint8*>(inputImage.scanLine(y));
        quint8 *currentPixel2 = static_cast<quint8*>(outpuImage.scanLine(y));
        for (int x = 0; x < inputImage.width(); ++x, ++currentPixel1, ++currentPixel2) {
            *currentPixel2 = *currentPixel1 < v ? 0 : 255;
        }
    }
    return outpuImage;
}

}
