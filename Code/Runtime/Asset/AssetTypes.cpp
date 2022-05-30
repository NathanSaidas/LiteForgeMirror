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
#include "Runtime/PCH.h"
#include "AssetTypes.h"
#include "Core/IO/Stream.h"

namespace lf {

AssetHash::AssetHash() 
: mValue()
{
    // data type must be byte
    SetZero();
}
AssetHash::AssetHash(const Crypto::MD5Hash& hash)
: mValue(hash)
{}


void AssetHash::Serialize(Stream& s)
{
    s.SerializeGuid(mValue.Bytes(), mValue.Size());
}
bool AssetHash::Parse(const String& string)
{
    const SizeT HEX_LENGTH = mValue.Size() * 2;
    if (string.Size() != HEX_LENGTH)
    {
        return false;
    }

    SizeT byteIndex = 0;
    for (SizeT i = 0; i < string.Size(); i += 2)
    {
        ByteT first = HexToByte(string[i]);
        ByteT second = HexToByte(string[i + 1]);
        if (Invalid(first) || Invalid(second))
        {
            SetZero();
            return false;
        }
        mValue.Bytes()[byteIndex] = (first << 4) | second;
        ++byteIndex;
    }
    return true;
}
String AssetHash::ToString() const
{
    const SizeT HEX_LENGTH = mValue.Size() * 2;
    String hex;
    hex.Reserve(HEX_LENGTH);
    for (SizeT i = 0; i < mValue.Size(); ++i)
    {
        Char8 first = ByteToHex((mValue.Bytes()[i] >> 4) & 0x0F);
        Char8 second = ByteToHex(mValue.Bytes()[i] & 0x0F);
        if (first == '\0' || second == '\0')
        {
            return String();
        }
        hex.Append(first);
        hex.Append(second);
    }
    return hex;
}

void AssetHash::SetZero()
{
    mValue = Crypto::MD5Hash();
}
bool AssetHash::IsZero() const
{
    return mValue.Empty();
}

bool AssetLoadState::IsCreated(AssetLoadState::Value value)
{
    return value >= ALS_CREATED && value <= ALS_LOADED;
}
bool AssetLoadState::IsPropertyLoaded(AssetLoadState::Value value)
{
    return value >= ALS_SERIALIZED_PROPERTIES && value <= ALS_LOADED;
}
bool AssetLoadState::IsDependencyLoaded(AssetLoadState::Value value)
{
    return value >= ALS_SERIALIZED_DEPENDENCIES && value <= ALS_LOADED;
}

// AssetTypeData::AssetTypeData() :
// mFullName(),
// mConcreteType(),
// mCacheName(),
// mUID(INVALID32),
// mParentUID(INVALID32),
// mVersion(0),
// mAttributes(0),
// mFlags(0),
// mCategory(0),
// mHash()
// {
// 
// }
// void AssetTypeData::Serialize(Stream& s)
// {
//     SERIALIZE(s, mCacheName, "");
//     SERIALIZE(s, mUID, "");
//     SERIALIZE(s, mParentUID, "");
//     SERIALIZE(s, mVersion, "");
//     SERIALIZE(s, mAttributes, "");
//     SERIALIZE(s, mFlags, "");
//     SERIALIZE(s, mCategory, "");
//     SERIALIZE(s, mHash, "");
// }

} // namespace lf