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
#ifndef LF_CORE_SECURE_RANDOM_H
#define LF_CORE_SECURE_RANDOM_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf { namespace Crypto {

// **********************************
// Generates cryptographically secure random data. (As long as IsSecureRandom is true)
// 
// @param outBytes  -- The bytes to write
// @param numBytes  -- The number of bytes to generate.
// **********************************
LF_CORE_API void SecureRandomBytes(ByteT* outBytes, SizeT numBytes);
// **********************************
// Returns true if we're using a cryptographically secure random data service.
// **********************************
LF_CORE_API bool IsSecureRandom();

} // namespace Crypto
} // namespace lf 

#endif // LF_CORE_SECURE_RANDOM_H