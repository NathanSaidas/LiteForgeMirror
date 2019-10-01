// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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
#include "SHA256.h"
#include <utility>

#include <openssl/sha.h>

namespace lf {

// todo: This is a VERY basic implementation is probably VERY slow compared to something that uses more advanced compile time/constant tricks + SIMD

#define RotateLeft(a, b) (((a) << (b)) | ((a) >> (32 - (b))))

#define RotateRight(a, b) (((a) >> (b)) | ((a) << (32 - (b))))

// aka Choose
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
// aka Majority
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
// ??
#define EP0(x) (RotateRight(x,2) ^ RotateRight(x,13) ^ RotateRight(x, 22))
// ??
#define EP1(x) (RotateRight(x, 6) ^ RotateRight(x, 11) ^ RotateRight(x, 25))
// aka Sigma0
#define SIG0(x) (RotateRight(x, 7) ^ RotateRight(x, 18) ^ ((x) >> 3))
// aka Sigma1
#define SIG1(x) (RotateRight(x, 17) ^ RotateRight(x, 19) ^ ((x) >> 10))

static const UInt32 SHA256_K[64] = 
{
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void SHA256Transform(Crypto::SHA256Context* context, const ByteT* data)
{
    UInt32 a, b, c, d, e, f, g, h;
    UInt32 i, j; // loops
    UInt32 t1, t2;
    UInt32 m[64];

    for (i = 0, j = 0; i < 16; ++i, j += 4)
    {
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    }

    for (; i < 64; ++i)
    {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }

    a = context->mState[0];
    b = context->mState[1];
    c = context->mState[2];
    d = context->mState[3];
    e = context->mState[4];
    f = context->mState[5];
    g = context->mState[6];
    h = context->mState[7];

    for (i = 0; i < 64; ++i)
    {
        auto ep1 = EP1(e);
        auto ch = CH(e, f, g);
        (ep1);
        (ch);

        t1 = h + EP1(e) + CH(e, f, g) + SHA256_K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    context->mState[0] += a;
    context->mState[1] += b;
    context->mState[2] += c;
    context->mState[3] += d;
    context->mState[4] += e;
    context->mState[5] += f;
    context->mState[6] += g;
    context->mState[7] += h;
}

void Crypto::SHA256Init(SHA256Context* context)
{
    context->mDataLength = 0;
    context->mBitLen = 0;
    context->mState[0] = 0x6a09e667;
    context->mState[1] = 0xbb67ae85;
    context->mState[2] = 0x3c6ef372;
    context->mState[3] = 0xa54ff53a;
    context->mState[4] = 0x510e527f;
    context->mState[5] = 0x9b05688c;
    context->mState[6] = 0x1f83d9ab;
    context->mState[7] = 0x5be0cd19;
}

void Crypto::SHA256Update(SHA256Context* context, const ByteT* data, SizeT dataLength)
{
    for (SizeT i = 0; i < dataLength; ++i)
    {
        context->mData[context->mDataLength] = data[i];
        context->mDataLength++;
        if (context->mDataLength == 64)
        {
            SHA256Transform(context, context->mData);
            context->mBitLen += 512;
            context->mDataLength = 0;
        }
    }
}

void Crypto::SHA256Final(SHA256Context* context, ByteT* hash)
{
    UInt32 i = 0;
    i = context->mDataLength;

    if (context->mDataLength < 56)
    {
        context->mData[i++] = 0x80;
        while (i < 56)
        {
            context->mData[i++] = 0x00;
        }
    }
    else
    {
        context->mData[i++] = 0x80;
        while (i < 64)
        {
            context->mData[i++] = 0x00;
        }
        SHA256Transform(context, context->mData);
        memset(context->mData, 0, 56);
    }

    context->mBitLen += context->mDataLength * 8;
    context->mData[63] = static_cast<ByteT>(context->mBitLen & 0xFF);
    context->mData[62] = static_cast<ByteT>(context->mBitLen >> 8 & 0xFF);
    context->mData[61] = static_cast<ByteT>(context->mBitLen >> 16 & 0xFF);
    context->mData[60] = static_cast<ByteT>(context->mBitLen >> 24 & 0xFF);
    context->mData[59] = static_cast<ByteT>(context->mBitLen >> 32 & 0xFF);
    context->mData[58] = static_cast<ByteT>(context->mBitLen >> 40 & 0xFF);
    context->mData[57] = static_cast<ByteT>(context->mBitLen >> 48 & 0xFF);
    context->mData[56] = static_cast<ByteT>(context->mBitLen >> 56 & 0xFF);
    SHA256Transform(context, context->mData);


    // Since this implementation uses little endian byte ordering and SHA uses big endian,
    // reverse all the bytes when copying the final state to the output hash.
    for (i = 0; i < 4; ++i) {
        hash[i] = (context->mState[0] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 4] = (context->mState[1] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 8] = (context->mState[2] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 12] = (context->mState[3] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 16] = (context->mState[4] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 20] = (context->mState[5] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 24] = (context->mState[6] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 28] = (context->mState[7] >> (24 - i * 8)) & 0x000000ff;
    }
}

Crypto::SHA256HashType Crypto::SHA256Hash(const ByteT* data, SizeT dataLength)
{
    // Crypto::SHA256HashType hash;
    // Crypto::SHA256Context context;
    // SHA256Init(&context);
    // SHA256Update(&context, data, dataLength);
    // SHA256Final(&context, hash.mData);

    Crypto::SHA256HashType hash;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, dataLength);
    SHA256_Final(hash.mData, &ctx);
    return hash;
}

Crypto::SHA256HashType Crypto::SHA256Hash(const ByteT* data, SizeT dataLength, const ByteT* salt, SizeT saltLength)
{
    Crypto::SHA256HashType hash;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, salt, saltLength);
    SHA256_Update(&ctx, data, dataLength);
    SHA256_Final(hash.mData, &ctx);
    return hash;
}

}