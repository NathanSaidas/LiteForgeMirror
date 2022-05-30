// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and / or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************
#pragma once

// TODO: Move this into project include or define in project..

#define LF_ENABLE_SSE_1
#define SH_ENABLE_SSE_4_1

#include <xmmintrin.h> // SSE
#include <immintrin.h>

// Intrinsic Includes..
//<mmintrin.h>  MMX
//<xmmintrin.h> SSE
//<emmintrin.h> SSE2
//<pmmintrin.h> SSE3
//<tmmintrin.h> SSSE3
//<smmintrin.h> SSE4.1
//<nmmintrin.h> SSE4.2
//<ammintrin.h> SSE4A
//<wmmintrin.h> AES
//<immintrin.h> AVX
//<zmmintrin.h> AVX512

namespace lf
{
#if defined(LF_ENABLE_SSE_1)
#define internal_vector __m128
#define internal_ivector __m128i

#define internal_aligned_buffer __declspec(align(16)) Float32

    /**
    * Get the components of the vector
    * out_vec = float[4]
    * in_vec = internal_vector
    */
#define vector_get(out_vec, in_vec) _mm_store_ps(out_vec, in_vec)
    /**
    * Set the components of the vector
    * x = float
    * y = float
    * z = float
    * w = float
    */
#define vector_set(x,y,z,w)         _mm_set_ps(w,z,y,x)
    /**
    * Set the components of the vector with an array.
    * arr = float[4]
    */
#define vector_set_array(arr)       vector_set(arr[0],arr[1],arr[2],arr[3])

#define vector_zero                 _mm_setzero_ps()
#define ivector_zero                _mm_setzero_si128()

    /**
    * Adds (a+b) to vectors component wise
    * @return Returns the added vectors
    */
#define vector_add(a,b)             _mm_add_ps(a,b)

    /**
    * Subtracts (a-b) component wise
    */
#define vector_sub(a,b)             _mm_sub_ps(a,b)

    /**
    * Multiplies (a*b) component wise
    */
#define vector_mul(a,b)             _mm_mul_ps(a,b)

    /**
    * Divide (a*b) component wise
    */
#define vector_div(a,b)             _mm_div_ps(a,b)

#define vector_sqrt(x)              _mm_sqrt_ss(x)
#define vector_rsqrt(x)             _mm_rsqrt_ps(x)
#define vector_to_float(x)          _mm_cvtss_f32(x)

#define vector_less                 _mm_cmplt_ps
#define vector_less_equal           _mm_cmple_ps   
#define vector_greater              _mm_cmpgt_ps
#define vector_greater_equal        _mm_cmpge_ps


#define vector_max(a,b)             _mm_max_ps(a,b)
#define vector_min(a,b)             _mm_min_ps(a,b)

#define vector_abs(x)               vector_max(vector_mul(x, vector_set(-1.0f, -1.0f, -1.0f, -1.0f)), x)

#define vector_cmp(a,b)             (_mm_movemask_ps(_mm_cmpeq_ps(a, b)) == 0xF)
#define vector_ncmp(a,b)            (_mm_movemask_ps(_mm_cmpneq_ps(a, b)) == 0xF)

// #define ivector_cmp(a,b)            (_mm_movemask_epi8(_mm_cmpeq_epi32(a, b)) == 0xFFFF)
// #define ivector_ncmp(a,b)           (_mm_movemask_epi8(_mm_cmpeq_epi32(a, b)) != 0xFFFF)
#define ivector_cmp(a,b)            (a.m128i_u64[0] == b.m128i_u64[0] && a.m128i_u64[1] == b.m128i_u64[1])
#define ivector_ncmp(a,b)           (a.m128i_u64[0] != b.m128i_u64[0] || a.m128i_u64[1] != b.m128i_u64[1])


#define vector_cross_shuffle(v,a,b,c,d) _mm_shuffle_ps(v,v,_MM_SHUFFLE(a,b,c,d))
#endif

#if defined(SH_ENABLE_SSE_4_1)
#define vector_dot(a,b,mask)             _mm_dp_ps(a,b,mask)
#endif

}