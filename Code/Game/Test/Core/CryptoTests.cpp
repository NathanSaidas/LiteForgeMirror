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

#include "Core/Test/Test.h"
#include "Core/Crypto/SHA256.h"
#include "Core/String/SStream.h"
#include "Core/Math/Random.h"
#include "Core/Utility/Log.h"
#include <utility>
#include <cmath>

namespace lf {
REGISTER_TEST(SHA256_Test)
{
    ByteT text1[] = { "abc" };
    ByteT text2[] = { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" };
    ByteT text3[] = { "aaaaaaaaaa" };
    ByteT hash1[Crypto::SHA256_BLOCK_SIZE] = { 0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad };
    ByteT hash2[Crypto::SHA256_BLOCK_SIZE] = { 0x24,0x8d,0x6a,0x61,0xd2,0x06,0x38,0xb8,0xe5,0xc0,0x26,0x93,0x0c,0x3e,0x60,0x39,
        0xa3,0x3c,0xe4,0x59,0x64,0xff,0x21,0x67,0xf6,0xec,0xed,0xd4,0x19,0xdb,0x06,0xc1 };
    ByteT hash3[Crypto::SHA256_BLOCK_SIZE] = { 0xcd,0xc7,0x6e,0x5c,0x99,0x14,0xfb,0x92,0x81,0xa1,0xc7,0xe2,0x84,0xd7,0x3e,0x67,
        0xf1,0x80,0x9a,0x48,0xa4,0x97,0x20,0x0e,0x04,0x6d,0x39,0xcc,0xc7,0x11,0x2c,0xd0 };
    ByteT buf[Crypto::SHA256_BLOCK_SIZE];
    Crypto::SHA256Context ctx;
    int idx;

    Crypto::SHA256Init(&ctx);
    Crypto::SHA256Update(&ctx, text1, LF_ARRAY_SIZE(text1) - 1);
    Crypto::SHA256Final(&ctx, buf);

    TEST(!memcmp(hash1, buf, Crypto::SHA256_BLOCK_SIZE));

    Crypto::SHA256Init(&ctx);
    Crypto::SHA256Update(&ctx, text2, LF_ARRAY_SIZE(text2) - 1);
    Crypto::SHA256Final(&ctx, buf);
    TEST(!memcmp(hash2, buf, Crypto::SHA256_BLOCK_SIZE));

    Crypto::SHA256Init(&ctx);
    for (idx = 0; idx < 100000; ++idx)
        Crypto::SHA256Update(&ctx, text3, LF_ARRAY_SIZE(text3) - 1);
    Crypto::SHA256Final(&ctx, buf);
    TEST(!memcmp(hash3, buf, Crypto::SHA256_BLOCK_SIZE));
}

static UInt64 CMod(UInt64 base, UInt64 exponent, UInt64 modulus)
{
    if ((base < 1) || (exponent < 0) || (modulus < 1))
    {
        return 0;
    }
    UInt64 result = 1;
    while (exponent > 0)
    {
        if ((exponent % 2) == 1)
        {
            result = (result * base) % modulus;
        }
        base = (base * base) % modulus;
        exponent = static_cast<UInt64>(floor(exponent / 2.0));
    }
    return result;
}

REGISTER_TEST(CryptoTheory)
{
    UInt64 e = 17;
    UInt64 n = 3233;
    UInt64 d = 2753;
    UInt64 k = 0;

    SStream ss;

    ByteT mask[256];
    Int32 seed = 0xDACE;
    for (int i = 0; i < LF_ARRAY_SIZE(mask); ++i)
    {
        mask[i] = static_cast<ByteT>(Random::Mod(seed, 0xFF));
    }

    ss << "\n";
    do
    {
        // c = k^e % n
        // k = c^d % n

        UInt64 encrypted = CMod(k, e, n);
        UInt64 decrypted = CMod(encrypted, d, n);
        TEST(decrypted == k);
        
        ss << StreamFillLeft(5) << encrypted << StreamFillLeft() << " : " << StreamFillLeft(5) << decrypted << StreamFillLeft();
        if (decrypted != encrypted)
        {
            ss << "====";
        }
        else
        {
            ss << "=BAD";
        }
        ss << " ==|== ";

        encrypted = CMod(k ^ mask[k], e, n);
        decrypted = CMod(encrypted, d, n) ^ mask[k];
        TEST(decrypted == k);
        ss << StreamFillLeft(5) << encrypted << StreamFillLeft() << " : " << StreamFillLeft(5) << decrypted << StreamFillLeft();
        if (decrypted == encrypted)
        {
            ss << "=BAD";
        }
        ss << "\n";


        ++k;
    } while (k < 256);

    gTestLog.Info(LogMessage(ss.Str()));

}
}