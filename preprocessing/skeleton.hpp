#ifndef SKELETON_HPP
#define SKELETON_HPP

#include <QImage>

namespace Preprocessing
{

// Input images to these algorithms are supposed to be 8 bits grayscale

// Thinning algorithm as explained in
// "A Fast Parallel Algorithm for Thinning Digital Patterns"
// by T. Y. ZHANG and C. Y. SUEN
// inputImage is expected to be binary (0 = black, 255 = white)
QImage skeletonZhangSuen(const QImage &inputImage);

// Thinning algorithm as explained in
// "A Modified Fast Parallel Algorithm for Thinning Digital Patterns"
// by Y. S. CHEN and W. H. HSU
// inputImage is expected to be binary (0 = black, 255 = white)
QImage skeletonChenHsu(const QImage &inputImage);

}

#endif