// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "StressDataAsset.h"
#include "Core/Math/Random.h"
#include "Runtime/Asset/AssetTypeInfo.h"

namespace lf {

DEFINE_CLASS(lf::StressDataAsset) { NO_REFLECTION; }

StressDataAsset::StressDataAsset()
: mData{ 0 }
, mReferenceType()
{}
StressDataAsset::~StressDataAsset()
{}

void StressDataAsset::Serialize(Stream& s)
{
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mData); ++i)
    {
        char name[32];
        snprintf(name, sizeof(name), "Data_%d", (int)i);
        SERIALIZE_NAMED(s, name, mData[i], "");
    }
}

void StressDataAsset::Generate(Int32& seed)
{
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mData); ++i)
    {
        mData[i] = static_cast<Int32>(Random::Mod(seed, 1000));
    }
}


}