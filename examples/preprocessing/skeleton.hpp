// Copyright (C) 2020 deiflou
// 
// This file is part of colorizer.
// 
// colorizer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// colorizer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with colorizer.  If not, see <http://www.gnu.org/licenses/>.

#ifndef SKELETON_HPP
#define SKELETON_HPP

#include <QImage>

namespace preprocessing
{

// Input images to these algorithms are supposed to be 8 bits grayscale

// Thinning algorithm as explained in
// "A Fast Parallel Algorithm for Thinning Digital Patterns"
// by T. Y. ZHANG and C. Y. SUEN
// inputImage is expected to be binary (0 = black, 255 = white)
QImage
skeleton_zhang_suen(QImage const & input_image);

// Thinning algorithm as explained in
// "A Modified Fast Parallel Algorithm for Thinning Digital Patterns"
// by Y. S. CHEN and W. H. HSU
// inputImage is expected to be binary (0 = black, 255 = white)
QImage
skeleton_chen_hsu(QImage const & input_image);

}

#endif