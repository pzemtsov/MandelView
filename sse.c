#include <stdint.h>
#include "sse.h"

void calculate_sse_float (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY)
{
    __m128 dd = _mm_set1_ps ((float) scale);
    __m128 XX0 = _mm_set1_ps ((float) X0);

    for (unsigned j = YSTART; j < SY; j++)	{
        __m128 y0 = _mm_set1_ps (j * (float) scale + (float) Y0);
        for (unsigned i = 0; i < SX; i += 4)	{

            __m128i ind = _mm_setr_epi32 (i, i + 1, i + 2, i + 3);
            __m128 x0 = _mm_add_ps (XX0, _mm_mul_ps (dd, _mm_cvtepi32_ps (ind)));
            __m128 x = x0;
            __m128 y = y0;
            __m128i counts = _mm_setzero_si128 ();
            __m128i cmp_mask = _mm_set1_epi32 (0xFFFFFFFFu);

            for (unsigned n = 0; n < 255; n++)	{
                __m128 x2 = _mm_mul_ps (x, x);
                __m128 y2 = _mm_mul_ps (y, y);
                __m128 abs = _mm_add_ps (x2, y2);
                __m128i cmp = _mm_castps_si128 (_mm_cmplt_ps (abs, _mm_set1_ps (4)));
                cmp_mask = _mm_and_si128 (cmp_mask, cmp);
                if (_mm_testz_si128 (cmp_mask, cmp_mask)) { // SSE4.1 instruction
                    break;
                }
                counts = _mm_sub_epi32 (counts, cmp_mask);
                __m128 t = _mm_mul_ps (x, y);
                t = _mm_add_ps (t, t);
                y = _mm_add_ps (t, y0);
                x = _mm_add_ps (_mm_sub_ps (x2, y2), x0);
            }
            __m128i result = _mm_shuffle_epi8 (counts, _mm_setr_epi8 (0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12)); // SSSE3 instruction
            *(uint32_t*) out = _mm_extract_epi32 (result, 0);
            out += 4;
        }
    }
}

void calculate_sse_double (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY)
{
    __m128d dd = _mm_set1_pd (scale);
    __m128d XX0 = _mm_set1_pd (X0);

    for (unsigned j = YSTART; j < SY; j++)	{
        __m128d y0 = _mm_set1_pd (j*scale + Y0);
        for (unsigned i = 0; i < SX; i += 2)	{

            __m128i ind = _mm_setr_epi32 (i, i + 1, 0, 0);
            __m128d x0 = _mm_add_pd (XX0, _mm_mul_pd (dd, _mm_cvtepi32_pd (ind)));
            __m128d x = x0;
            __m128d y = y0;
            __m128i counts = _mm_setzero_si128 ();
            __m128i cmp_mask = _mm_set1_epi32 (0xFFFFFFFFu);

            for (unsigned n = 0; n < 255; n++)	{
                __m128d x2 = _mm_mul_pd (x, x);
                __m128d y2 = _mm_mul_pd (y, y);
                __m128d abs = _mm_add_pd (x2, y2);
                __m128i cmp = _mm_castpd_si128 (_mm_cmplt_pd (abs, _mm_set1_pd (4)));
                cmp_mask = _mm_and_si128 (cmp_mask, cmp);
                if (_mm_testz_si128 (cmp_mask, cmp_mask)) {
                    break;
                }
                counts = _mm_sub_epi64 (counts, cmp_mask);
                __m128d t = _mm_mul_pd (x, y);
                t = _mm_add_pd (t, t);
                y = _mm_add_pd (t, y0);
                x = _mm_add_pd (_mm_sub_pd (x2, y2), x0);
            }
            __m128i result = _mm_shuffle_epi8 (counts, _mm_setr_epi8 (0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8));
            *(uint16_t*) out = _mm_extract_epi16 (result, 0);
            out += 2;
        }
    }
}
