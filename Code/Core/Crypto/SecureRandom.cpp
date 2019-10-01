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

#include "SecureRandom.h"
#include "Core/Utility/StaticCallback.h"
#include "Core/Math/Random.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/ErrorCore.h"

#if defined(LF_OS_WINDOWS)
#define NOMINMAX
#include <Windows.h>
#include <wincrypt.h>
#endif

namespace lf { namespace Crypto {

#if defined(LF_OS_WINDOWS)
static HCRYPTPROV gCryptoProvider = NULL;
static const char* gCryptoKeyName = "LiteForgeCryptoContainer";
static Int32 gCryptoInsecureRandom = 0xBADDBADD;
static bool  gUseInsecureRandom = false;
#endif

static void InitSecureRandom();
static void ShutdownSecureRandom();

// Register for 'ordered static initialization'
STATIC_INIT(OnInitSecureRandom, SCP_PRE_INIT_CORE)
{
    InitSecureRandom();
}

// Register for 'ordered static shutdown'
STATIC_DESTROY(OnShutdownSecureRandom, SCP_PRE_INIT_CORE)
{
    ShutdownSecureRandom();
}

void InitSecureRandom()
{
#if defined(LF_OS_WINDOWS)
    if (CmdLine::GetArgOption("crypto", "insecure_random", gCryptoInsecureRandom) || CmdLine::HasArgOption("crypto", "insecure_random"))
    {
        gUseInsecureRandom = true;
        return;
    }

    // Try first to acqurie the provider with the same key...
    if (!CryptAcquireContext(&gCryptoProvider, gCryptoKeyName, NULL, PROV_RSA_FULL, 0))
    {
        // If the key didn't exist create it!
        if (GetLastError() == NTE_BAD_KEYSET)
        {
            if (!CryptAcquireContext(&gCryptoProvider, gCryptoKeyName, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
            {
                CriticalAssertMsgEx("Failed to initialize Crypto::SecureRandom", LF_ERROR_INTERNAL, ERROR_API_CORE);
            }
        }
    }
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif
}
void ShutdownSecureRandom()
{
#if defined(LF_OS_WINDOWS)
    if (gUseInsecureRandom)
    {
        return;
    }

    if (gCryptoProvider)
    {
        if (!CryptReleaseContext(gCryptoProvider, 0))
        {
            ReportBugMsgEx("Failed to release Crypto::SecureRandom", LF_ERROR_INTERNAL, ERROR_API_CORE);
        }
    }
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif
}
void SecureRandomBytes(ByteT* outBytes, SizeT numBytes)
{
#if defined(LF_OS_WINDOWS)
    if(gUseInsecureRandom)
    {
        ByteT buffer[4];
        Int32* bufferptr = reinterpret_cast<Int32*>(&buffer[0]);
        while (numBytes > 0)
        {
            *bufferptr = Random::Rand(gCryptoInsecureRandom);
            for (SizeT i = 0; i < 4 && numBytes > 0; ++i)
            {
                *outBytes = buffer[i];
                ++outBytes;
                --numBytes;
            }
        }
    }
    else
    {
        CriticalAssertEx(gCryptoProvider, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        AssertEx(CryptGenRandom(gCryptoProvider, static_cast<DWORD>(numBytes), outBytes) == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
    }
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif
}

bool IsSecureRandom()
{
    return !gUseInsecureRandom;
}

} // namespace Crypto
} // namespace lf
