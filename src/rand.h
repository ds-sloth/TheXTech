/*
 * TheXTech - A platform game engine ported from old source code for VB6
 *
 * Copyright (c) 2009-2011 Andrew Spinks, original VB6 code
 * Copyright (c) 2020-2021 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef RAND_H
#define RAND_H

/**
 * @brief Seeds the random number generator with argument seed for reproducible results
 */
extern void seedRandom(int seed);

/**
 * @brief Random number generator in float format, between 0.0f to 1.0f (exclusive)
 * @return random float value
 */
extern float fRand();

/**
 * @brief Random number generator in double format, between 0.0 to 1.0 (exclusive)
 * @return random double value
 */
extern double dRand();

/**
 * @brief Random number generator in integer format, between 0.0 to RAND_MAX
 * @return random integer value
 */
extern int iRand();

/**
 * @brief Random number generator in integer format, between 0 to argument max (exclusive)
 * @return random integer value
 */
extern int iRand(int max);


#endif // RAND_H
