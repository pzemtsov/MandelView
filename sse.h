#ifndef _SSE_H
#define _SSE_H

#include <stdint.h>

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#include <emmintrin.h>
#include <immintrin.h>

#define _128_shuffle(x, y, n0, n1, n2, n3) _mm_shuffle_ps(x, y, combine_4_2bits (n0, n1, n2, n3))
#define _128i_shuffle(x, y, n0, n1, n2, n3) _mm_castps_si128(_128_shuffle(_mm_castsi128_ps(x), _mm_castsi128_ps(y), n0, n1, n2, n3))
#define combine_4_2bits(n0, n1, n2, n3) (n0 + (n1<<2) + (n2<<4) + (n3<<6))

#endif