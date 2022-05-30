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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANYJsonStream CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************
#pragma once

#include "Core/Common/Types.h"
#include "Core/Common/Enum.h"

namespace lf
{

DECLARE_ENUM(NumericalVariantType,
VT_U32,
VT_U64,
VT_I32,
VT_I64,
VT_F32,
VT_NONE
);

struct NumericalVariant
{
    using VariantType = NumericalVariantType::Value;

    NumericalVariant()
    : mType(VariantType::VT_NONE)
    , mData{ 0 }
    {}

    explicit NumericalVariant(UInt32 value)
    : mType(VariantType::VT_U32)
    , mData{ 0 }
    {
        mData.u32 = value;
    }

    explicit NumericalVariant(UInt64 value)
    : mType(VariantType::VT_U64)
    , mData{ 0 }
    {
        mData.u64 = value;
    }

    explicit NumericalVariant(Int32 value)
    : mType(VariantType::VT_I32)
    , mData{ 0 }
    {
        mData.i32 = value;
    }

    explicit NumericalVariant(Int64 value)
    : mType(VariantType::VT_I64)
    , mData{ 0 }
    {
        mData.i64 = value;
    }

    explicit NumericalVariant(Float32 value)
    : mType(VariantType::VT_F32)
    , mData{ 0 }
    {
        mData.f32 = value;
    }

    bool operator==(UInt32 value) const { return mType == VariantType::VT_U32 && mData.u32 == value; }
    bool operator==(UInt64 value) const { return mType == VariantType::VT_U64 && mData.u64 == value; }
    bool operator==(Int32 value) const { return mType == VariantType::VT_I32 && mData.i32 == value; }
    bool operator==(Int64 value) const { return mType == VariantType::VT_I64 && mData.i64 == value; }
    bool operator==(Float32 value) const { return mType == VariantType::VT_F32 && mData.f32 == value; }

    bool operator!=(UInt32 value) const { return mType != VariantType::VT_U32 || mData.u32 != value; }
    bool operator!=(UInt64 value) const { return mType != VariantType::VT_U64 || mData.u64 != value; }
    bool operator!=(Int32 value) const { return mType != VariantType::VT_I32 || mData.i32 != value; }
    bool operator!=(Int64 value) const { return mType != VariantType::VT_I64 || mData.i64 != value; }
    bool operator!=(Float32 value) const { return mType != VariantType::VT_F32 || mData.f32 != value; }

    bool operator==(const NumericalVariant& other) const
    {
        switch (other.mType)
        {
        case VariantType::VT_U32: return *this == other.mData.u32;
        case VariantType::VT_U64: return *this == other.mData.u64;
        case VariantType::VT_I32: return *this == other.mData.i32;
        case VariantType::VT_I64: return *this == other.mData.i64;
        case VariantType::VT_F32: return *this == other.mData.f32;
        default:
            break;
        }
        return mType == other.mType;
    }

    bool operator!=(const NumericalVariant& other) const
    {
        switch (other.mType)
        {
        case VariantType::VT_U32: return *this != other.mData.u32;
        case VariantType::VT_U64: return *this != other.mData.u64;
        case VariantType::VT_I32: return *this != other.mData.i32;
        case VariantType::VT_I64: return *this != other.mData.i64;
        case VariantType::VT_F32: return *this != other.mData.f32;
        default:
            break;
        }
        return mType != other.mType;
    }

    bool operator<(const NumericalVariant& other) const
    {
        if (mType < other.mType)
        {
            return true;
        }
        switch (mType)
        {
        case VariantType::VT_U32: return mData.u32 < other.mData.u32;
        case VariantType::VT_U64: return mData.u64 < other.mData.u64;
        case VariantType::VT_I32: return mData.i32 < other.mData.i32;
        case VariantType::VT_I64: return mData.i64 < other.mData.i64;
        case VariantType::VT_F32: return mData.f32 < other.mData.f32;
        default:
            break;
        }
        return false;
    }

    bool operator>(const NumericalVariant& other) const
    {
        if (mType > other.mType)
        {
            return true;
        }
        switch (mType)
        {
        case VariantType::VT_U32: return mData.u32 > other.mData.u32;
        case VariantType::VT_U64: return mData.u64 > other.mData.u64;
        case VariantType::VT_I32: return mData.i32 > other.mData.i32;
        case VariantType::VT_I64: return mData.i64 > other.mData.i64;
        case VariantType::VT_F32: return mData.f32 > other.mData.f32;
        default:
            break;
        }
        return false;
    }

    static SizeT GetSize(VariantType type)
    {
        switch (type)
        {
        case VariantType::VT_U32: return sizeof(UInt32);
        case VariantType::VT_U64: return sizeof(UInt64);
        case VariantType::VT_I32: return sizeof(Int32);
        case VariantType::VT_I64: return sizeof(Int64);
        case VariantType::VT_F32: return sizeof(Float32);
        default:
            break;
        }
        return 0;
    }

    static NumericalVariant Cast(VariantType type, const ByteT* data)
    {
        if (data == nullptr)
        {
            return NumericalVariant();
        }
        switch (type)
        {
        case VariantType::VT_U32: return NumericalVariant(*reinterpret_cast<const UInt32*>(data));
        case VariantType::VT_U64: return NumericalVariant(*reinterpret_cast<const UInt64*>(data));
        case VariantType::VT_I32: return NumericalVariant(*reinterpret_cast<const Int32*>(data));
        case VariantType::VT_I64: return NumericalVariant(*reinterpret_cast<const Int64*>(data));
        case VariantType::VT_F32: return NumericalVariant(*reinterpret_cast<const Float32*>(data));
        default:
            break;
        }
        return NumericalVariant();
    }

    VariantType mType;
    union
    {
        UInt32  u32;
        Int32   i32;
        Float32 f32;
        UInt64  u64;
        Int64   i64;
    } mData;
};

} // namespace lf