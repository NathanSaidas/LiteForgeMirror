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
#include "Core/PCH.h"
#include "HMAC.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/Utility/StaticCallback.h"

#include <openssl/hmac.h>
#include <openssl/engine.h>

namespace lf {

// Register for 'ordered static initialization'
STATIC_INIT(OnInitHMAC, SCP_PRE_INIT_CORE)
{
    ENGINE_load_builtin_engines();
    ENGINE_register_all_complete();
}

namespace Crypto {

bool HMACKey::Generate()
{
    Crypto::SecureRandomBytes(Bytes(), Size());
    return true;
}

bool HMACKey::Compute(const ByteT* data, SizeT dataLength, HMACBuffer& outBuffer) const
{
    // Still a bit confused on how to do this correctly. 
    // Going based off of https://stackoverflow.com/questions/242665/understanding-engine-initialization-in-openssl

    const int OPENSSL_TRUE = 1;

    HMAC_CTX* ctx = HMAC_CTX_new();
    bool success = false;
    UInt32 outBytesLength = static_cast<UInt32>(outBuffer.Size());
    if (!ctx)
    {
        return false;
    }

    do
    {
        if (HMAC_Init_ex(ctx, Bytes(), sizeof(HMACKey), EVP_sha256(), NULL) != OPENSSL_TRUE)
        {
            break;
        }

        if (HMAC_Update(ctx, data, dataLength) != OPENSSL_TRUE)
        {
            break;
        }

        if (HMAC_Final(ctx, outBuffer.Bytes(), &outBytesLength) != OPENSSL_TRUE)
        {
            break;
        }

        success = true;

    } while (0);

    HMAC_CTX_free(ctx);
    return success && outBytesLength == sizeof(Crypto::HMACBuffer);
}

} // namespace Crypto
} // namespace lf