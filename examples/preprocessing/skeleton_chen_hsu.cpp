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

#include "skeleton.hpp"

namespace preprocessing
{

// Add a 1 pixel border and
// copy the inputImage changing white values (255) to 0 and
// black values (0) to 1
static QImage
preprocess(QImage const & input_image)
{
    QImage image = input_image.copy(-1, -1, input_image.width() + 2, input_image.height() + 2);
    for (int y = 1; y < image.height() - 1; ++y)
    {
        quint8 * current_pixel = static_cast<quint8 *>(image.scanLine(y) + 1);
        for (int x = 1; x < image.width() - 1; ++x, ++current_pixel)
        {
            *current_pixel = (255 - *current_pixel) / 255;
        }
    }
    return image;
}

// Remove the 1 pixel border and
// copy the inputImage changing 0 values to white (255) and
// 1 values to black (0)
static QImage
postprocess(QImage const & input_image)
{
    QImage image = input_image.copy(1, 1, input_image.width() - 2, input_image.height() - 2);
    for (int y = 0; y < image.height(); ++y)
    {
        quint8 * current_pixel = static_cast<quint8 *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x, ++current_pixel)
        {
            *current_pixel = (1 - *current_pixel) * 255;
        }
    }
    return image;
}

static bool
condition_1(quint8 const * neighbors)
{
    const int n = neighbors[0] + neighbors[1] + neighbors[2] + neighbors[3] +
                  neighbors[4] + neighbors[5] + neighbors[6] + neighbors[7];
    return n >= 2 && n <= 7;
}

static int
zero_one_turns(quint8 const * neighbors)
{
    int n = 0;
    for (int i = 0; i < 7; ++i)
    {
        if (neighbors[i] == 0 && neighbors[i + 1] == 1)
        {
            ++n;
        }
    }
    if (neighbors[7] == 0 && neighbors[0] == 1)
    {
        ++n;
    }
    return n;
}

static bool
condition_2(quint8 const * neighbors)
{
    return zero_one_turns(neighbors) == 1;
}

static bool
condition_3(int a, int b, int c)
{
    const int n = a * b * c;
    return n == 0;
}

static bool
condition_4(quint8 const * neighbors)
{
    return zero_one_turns(neighbors) == 2;
}

static bool
condition_5(int a, int b, int c, int d, int e)
{
    const int n = a * b;
    const int n2 = c + d + e;
    return n == 1 && n2 == 0;
}

// Since the image only uses values 0 and 1, we use the
// second bit to store if the pixel must be removed
static int
subiteration(QImage & input_image, quint8 const * lut, int const sub_iteration_number)
{
    int n = 0;

    for (int y = 1; y < input_image.height() - 1; ++y)
    {
        quint8 * current_pixel = static_cast<quint8 *>(input_image.scanLine(y) + 1);
        for (int x = 1; x < input_image.width() - 1; ++x, ++current_pixel)
        {
            if (*current_pixel == 0)
            {
                continue;
            }

            quint8 index = 0;
            index |= (*(current_pixel - input_image.bytesPerLine()) & 1) << 0;
            index |= (*(current_pixel - input_image.bytesPerLine() + 1) & 1) << 1;
            index |= (*(current_pixel + 1) & 1) << 2;
            index |= (*(current_pixel + input_image.bytesPerLine() + 1) & 1) << 3;
            index |= (*(current_pixel + input_image.bytesPerLine()) & 1) << 4;
            index |= (*(current_pixel + input_image.bytesPerLine() - 1) & 1) << 5;
            index |= (*(current_pixel - 1) & 1) << 6;
            index |= (*(current_pixel - input_image.bytesPerLine() - 1) & 1) << 7;

            if (lut[index] & sub_iteration_number)
            {
                *current_pixel |= 2;
                ++n;
            }
        }
    }
    return n;
}

// If the second bit is set to 1, set the pixel to 0
static void
remove_pixels(QImage & input_image)
{
    for (int y = 1; y < input_image.height() - 1; ++y)
    {
        quint8 * current_pixel = static_cast<quint8 *>(input_image.scanLine(y) + 1);
        for (int x = 1; x < input_image.width() - 1; ++x, ++current_pixel)
        {
            *current_pixel = (*current_pixel & 1) & !((*current_pixel & 2) >> 1);
        }
    }
}

static void
make_lut(quint8 * lut)
{
    quint8 neighbors[8];
    for (int i = 0; i < 256; ++i)
    {
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
        if (condition_1(neighbors) &&
            (condition_2(neighbors) &&
                condition_3(neighbors[0], neighbors[2], neighbors[4]) &&
                condition_3(neighbors[2], neighbors[4], neighbors[6])) ||
            (condition_4(neighbors) &&
                condition_5(neighbors[0], neighbors[2], neighbors[4], neighbors[5], neighbors[6]) ||
                condition_5(neighbors[2], neighbors[4], neighbors[0], neighbors[6], neighbors[7])))
        {
            lut[i] |= 1;
        }

        // Sets the second bit to 1 if the conditions for the second subiteration are met.
        if (condition_1(neighbors) &&
            (condition_2(neighbors) &&
                condition_3(neighbors[0], neighbors[2], neighbors[6]) &&
                condition_3(neighbors[0], neighbors[4], neighbors[6])) ||
            (condition_4(neighbors) &&
                condition_5(neighbors[0], neighbors[6], neighbors[2], neighbors[3], neighbors[4]) ||
                condition_5(neighbors[4], neighbors[6], neighbors[0], neighbors[1], neighbors[2])))
        {
            lut[i] |= 2;
        }
    }
}

QImage
skeleton_chen_hsu(QImage const & input_image)
{
    static quint8 lut[256];
    static bool lut_ready = false;
    if (!lut_ready) {
        make_lut(lut);
        lut_ready = true;
    }

    QImage image = preprocess(input_image);

    while (true) {
        int n1, n2;

        n1 = subiteration(image, lut, 1);
        if (n1 > 0) {
            remove_pixels(image);
        }

        n2 = subiteration(image, lut, 2);
        if (n2 > 0) {
            remove_pixels(image);
        }

        if (n1 == 0 && n2 == 0) {
            break;
        }
    }

    return postprocess(image);
}

}
