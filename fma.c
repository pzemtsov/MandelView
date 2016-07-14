#include <stdint.h>
#include "sse.h"

void calculate_fma_float (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY)
{
    __m256 dd = _mm256_set1_ps ((float) scale);
    __m256 XX0 = _mm256_set1_ps ((float) X0);

    for (unsigned j = YSTART; j < SY; j++)	{
        __m256 y0 = _mm256_set1_ps (j*(float) scale + (float) Y0);
        for (unsigned i = 0; i < SX; i += 8)	{
            __m256i ind = _mm256_setr_epi32 (i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6, i + 7);
            __m256 x0 = _mm256_fmadd_ps (dd, _mm256_cvtepi32_ps (ind), XX0);
            __m256 x = x0;
            __m256 y = y0;
            __m256i counts = _mm256_setzero_si256 ();
            __m256i cmp_mask = _mm256_set1_epi32 (0xFFFFFFFFu);

            for (unsigned n = 0; n < 255; n++)	{
                __m256 x2 = _mm256_mul_ps (x, x);
                __m256 y2 = _mm256_mul_ps (y, y);
                __m256 abs = _mm256_add_ps (x2, y2);
                __m256i cmp = _mm256_castps_si256 (_mm256_cmp_ps (abs, _mm256_set1_ps (4), 1));
                cmp_mask = _mm256_and_si256 (cmp_mask, cmp);
                if (_mm256_testz_si256 (cmp_mask, cmp_mask)) {
                    break;
                }
                counts = _mm256_sub_epi32 (counts, cmp_mask);
                __m256 t = _mm256_add_ps (x, x);
                y = _mm256_fmadd_ps (t, y, y0);
                x = _mm256_add_ps (_mm256_sub_ps (x2, y2), x0);
            }
            __m256i result = _mm256_shuffle_epi8 (counts, _mm256_setr_epi8 (0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12));
            __m128i result128 = _128i_shuffle (_mm256_extractf128_si256 (result, 0), _mm256_extractf128_si256 (result, 1), 0, 0, 0, 0);
            result128 = _mm_shuffle_epi32 (result128, combine_4_2bits (0, 2, 0, 2));
            _mm_storel_epi64 ((__m128i*) out, result128);
            out += 8;
        }
    }
}

void calculate_fma_double (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY)
{
    __m256d dd = _mm256_set1_pd (scale);
    __m256d XX0 = _mm256_set1_pd (X0);

    for (unsigned j = YSTART; j < SY; j++)	{
        __m256d y0 = _mm256_set1_pd (j*scale + Y0);
        for (unsigned i = 0; i < SX; i += 4)	{

            __m128i ind = _mm_setr_epi32 (i, i + 1, i + 2, i + 3);
            __m256d x0 = _mm256_fmadd_pd (dd, _mm256_cvtepi32_pd (ind), XX0);
            __m256d x = x0;
            __m256d y = y0;
            __m256i counts = _mm256_setzero_si256 ();
            __m256i cmp_mask = _mm256_set1_epi32 (0xFFFFFFFFu);

            for (unsigned n = 0; n < 255; n++)	{
                __m256d x2 = _mm256_mul_pd (x, x);
                __m256d y2 = _mm256_mul_pd (y, y);
                __m256d abs = _mm256_add_pd (x2, y2);
                __m256i cmp = _mm256_castpd_si256 (_mm256_cmp_pd (abs, _mm256_set1_pd (4), 1));
                cmp_mask = _mm256_and_si256 (cmp_mask, cmp);
                if (_mm256_testz_si256 (cmp_mask, cmp_mask)) {
                    break;
                }
                counts = _mm256_sub_epi64 (counts, cmp_mask);
                __m256d t = _mm256_add_pd (x, x);
                y = _mm256_fmadd_pd (t, y, y0);
                x = _mm256_add_pd (_mm256_sub_pd (x2, y2), x0);
            }
            __m256i result = _mm256_shuffle_epi8 (counts, _mm256_setr_epi8 (0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8));
            *(uint32_t*) out = _mm_extract_epi16 (_mm256_extracti128_si256 (result, 0), 0) | (_mm_extract_epi16 (_mm256_extracti128_si256 (result, 1), 0) << 16);
            out += 4;
        }
    }
}
