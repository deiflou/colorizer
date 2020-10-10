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

#include "threshold.hpp"

namespace preprocessing
{

QImage
threshold(QImage const & input_image, int v)
{
    QImage outpu_image(input_image.width(), input_image.height(), input_image.format());
    for (int y = 0; y < input_image.height(); ++y)
    {
        quint8 const * current_pixel_1 = static_cast<const quint8 *>(input_image.scanLine(y));
        quint8 * current_pixel_2 = static_cast<quint8 *>(outpu_image.scanLine(y));
        for (int x = 0; x < input_image.width(); ++x, ++current_pixel_1, ++current_pixel_2)
        {
            *current_pixel_2 = *current_pixel_1 < v ? 0 : 255;
        }
    }
    return outpu_image;
}

}
