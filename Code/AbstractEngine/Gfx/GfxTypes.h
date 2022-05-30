#pragma once
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
#include "Core/Common/Enum.h"
#include "Core/String/Token.h"
#include "Core/String/String.h"
#include "Core/Utility/StdVector.h"
#include "Core/Utility/FNVHash.h"
#include "Core/Utility/APIResult.h"
#include "Core/Utility/Error.h"

namespace lf {

template<typename T>
class TAtomicStrongPointer;

namespace Gfx
{

using FrameCountType = UInt64;
enum FrameCount : FrameCountType { Value = 3 };

DECLARE_STRICT_ENUM(GraphicsApi,
    ANY,
    DX11,
    DX12
);

// ********************************************************************
//  Generic Blend Type Values.
// ********************************************************************
DECLARE_STRICT_ENUM(BlendType,
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DEST_ALPHA,
    ONE_MINUS_DEST_ALPHA,
    DEST_COLOR,
    ONE_MINUS_DEST_COLOR
);

// ********************************************************************
//  Generic Blending Operation Values.
// ********************************************************************
DECLARE_STRICT_ENUM(BlendOp,
    ADD,
    MINUS,
    INVERSE_MINUS,
    MIN,
    MAX
);

// ********************************************************************
//  Generic Blending Logic Values.
// ********************************************************************
DECLARE_STRICT_ENUM(BlendLogicOp,
    CLEAR,
    SET,
    COPY,
    COPY_INVERTED,
    NOOP,
    INVERT,
    AND,
    NAND,
    OR,
    NOR,
    XOR,
    EQUIV,
    AND_REVERSE,
    AND_INVERTED,
    OR_REVERSE,
    OR_INVERTED
);

DECLARE_STRICT_ENUM(ColorChannel,
    RED,
    GREEN,
    BLUE,
    ALPHA
);
using ColorChannelBitfield = Bitfield<ColorChannel>;

DECLARE_STRICT_ENUM(ResourceFormat,
    UNKNOWN,
    R32G32B32A32_FLOAT,
    R32G32B32A32_UINT,
    R32G32B32A32_INT,
    R32G32B32_FLOAT,
    R32G32B32_UINT,
    R32G32B32_INT,
    R32G32_FLOAT,
    R32G32_UINT,
    R32G32_INT,
    R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB,
    R8G8B8A8_UINT,
    R8G8B8A8_NORM,
    R8G8B8A8_INT
);

// ********************************************************************
// Generic Cull Mode Values.
// ********************************************************************
DECLARE_STRICT_ENUM(CullMode,
    CLOCK_WISE,
    COUNTER_CLOCK_WISE
);

// ********************************************************************
//  Generic Cull Face Values. 
// ********************************************************************
DECLARE_STRICT_ENUM(CullFace,
    NONE,
    FRONT,
    BACK
);

// ********************************************************************
// Generic Depth Func Values.
// ********************************************************************
DECLARE_STRICT_ENUM(DepthFunc,
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS
);

// ********************************************************************
// Shader Type Values.
// ********************************************************************
DECLARE_STRICT_ENUM(ShaderType,
    VERTEX,
    PIXEL
);

// **********************************
// Describes the shader attribute type..  
// **********************************
DECLARE_STRICT_ENUM(ShaderAttribType,
    VERTEX_DATA,  // OpenGL - attribute
    UNIFORM_DATA // OpenGL - uniform
);

// ********************************************************************
// Shader Property Type
// ********************************************************************
DECLARE_STRICT_ENUM(ShaderAttribFormat,
    SAF_FLOAT,
    SAF_INT,
    SAF_UINT,
    SAF_VECTOR2,
    SAF_VECTOR3,
    SAF_VECTOR4,
    SAF_MATRIX_3X3,
    SAF_MATRIX_4x4,
    SAF_TEXTURE,
    SAF_SAMPLER
);

// ********************************************************************
// Buffer Usage Type
// ********************************************************************
DECLARE_STRICT_ENUM(BufferUsage,
    // CPU and GPU have read and write
    READ_WRITE,
    // Data can be changed afterwards. GPU Read Only, CPU write only
    DYNAMIC,
    // Data is setup at start! GPU Read Only, CPU has no access.
    STATIC
);

// ********************************************************************
// Index Stride types
// ********************************************************************
DECLARE_STRICT_ENUM(IndexStride,
    SHORT,
    INT
);

// ********************************************************************
// Render Mode Types
// ********************************************************************
DECLARE_STRICT_ENUM(RenderMode,
POINTS = 1,
LINES,
LINE_STRIP,
TRIANGLES = 4,
TRIANGLE_STRIP
);

// ********************************************************************
// Render Mode Types
// ********************************************************************
DECLARE_STRICT_ENUM(StencilOp,
KEEP,
ZERO,
REPLACE,
INCR_SAT,
DECR_SAT,
INVERT,
INCR,
DECR);

using ShaderHash = FNV::HashT;

// ********************************************************************
// Size in bytes of each Attrib Format type
// ********************************************************************
const SizeT SHADER_ATTRIB_FORMAT_TO_SIZE[] =
{
    4,
    4,
    4,
    8,
    12,
    16,
    36,
    64,
    INVALID, // Texture has no size
    INVALID  // Sampler has no size
};

// **********************************
// This is a description for a shader attribute query.
// Can lookup per-vertex variable reference or uniform variable reference by name.
// **********************************
struct ShaderAttribDesc
{
    LF_INLINE ShaderAttribDesc() 
    : mType(ShaderAttribType::VERTEX_DATA)
    , mTypeName()
    , mName()
    , mSemantic()
    , mIndex(0)
    {}
    ShaderAttribType mType;
    Token mTypeName;
    Token mName;
    Token mSemantic;
    SizeT mIndex;
};
typedef TVector<ShaderAttribDesc> ShaderAttribs;

const Bitfield<Gfx::ShaderAttribFormat> ACCEPTED_VERTEX_FORMATS({
        Gfx::ShaderAttribFormat::SAF_FLOAT,
        Gfx::ShaderAttribFormat::SAF_INT,
        Gfx::ShaderAttribFormat::SAF_UINT,
        Gfx::ShaderAttribFormat::SAF_VECTOR2,
        Gfx::ShaderAttribFormat::SAF_VECTOR3,
        Gfx::ShaderAttribFormat::SAF_VECTOR4
});

struct GfxVertexElement
{
public:
    Gfx::ShaderAttribFormat mFormat;
    Token                   mSemantic;
    Token                   mUniformName;
    SizeT                   mIndex;
    SizeT                   mInputSlot;
    SizeT                   mByteOffset;
};

template<SizeT TSize>
struct GfxVertexFormat
{
public:
    GfxVertexFormat()
        : mCurrentInputSlot(0)
        , mByteOffset(0)
        , mElements()
    {}

    APIResult<bool> Add(ShaderAttribFormat format, const Token& semantic, const Token& uniform, SizeT index)
    {
        if (!ACCEPTED_VERTEX_FORMATS.Has(format))
        {
            return ReportError(false, InvalidArgumentError, "format", "Format is not an acceptable format. See ACCEPTABLE_FORMATS.");
        }
        GfxVertexElement element;
        element.mFormat = format;
        element.mSemantic = semantic;
        element.mUniformName = uniform;
        element.mIndex = index;
        element.mInputSlot = mCurrentInputSlot;
        element.mByteOffset = mByteOffset;
        mElements.push_back(element);
        mByteOffset += SHADER_ATTRIB_FORMAT_TO_SIZE[EnumValue(format)];
        return APIResult<bool>(true);
    }

    void PushInputSlot()
    {
        ++mCurrentInputSlot;
        mByteOffset = 0;
    }

    void Clear()
    {
        mCurrentInputSlot = 0;
        mByteOffset = 0;
        mElements.Clear();
    }
    const TStackVector<GfxVertexElement, TSize>& GetElements() const { return mElements; }
private:
    SizeT mCurrentInputSlot;
    SizeT mByteOffset;
    TStackVector<GfxVertexElement, TSize> mElements;
};
using VertexFormat = GfxVertexFormat<8>;

struct MaterialProperty
{
    Token   mName;
    UInt8   mType; // Gfx::ShaderAttribFormat
    UInt8   mSize;
    UInt16  mOffset;
};

using MaterialPropertyId = UInt32;
const MaterialPropertyId INVALID_MATERIAL_PROPERTY_ID = INVALID32;

// ********************************************************************
// Data structure to hold common 'abstract' data about the GfxPipelineState
// usually used by materials to build their PSO
// ********************************************************************
struct PipelineStateDesc
{
    TVector<ByteT>               mByteCode[ENUM_SIZE(Gfx::ShaderType)];
    GfxVertexFormat<8>          mVertexFormat;
    TVector<MaterialProperty>    mProperties;
};

struct RasterStateDesc
{
    // ** Describe the face to cull (if any)
    CullFace mCullFace = CullFace::BACK;
    // ** Describe the direction to cull the 'Cull Face'
    CullMode mCullMode = CullMode::CLOCK_WISE;
    // ** Render using the wireframe instead of solid.
    bool     mWireFrame = false;
    // ** True use the quadrilateral line anti-aliasing, false to use alpha line-aliasing.
    bool     mMultisampleEnabled = false;
    // ** Whether or not to enable line anti-aliasing,
    bool     mAntialiasedLineEnabled = false;
    // ** Whether or not to enable clipping based on depth distance.
    bool     mDepthClipEnabled = true;

};

struct BlendStateDesc
{
    bool mBlendEnabled = false;
    bool mLogicOpEnabled = false;
    BlendType mSrcBlend = BlendType::ONE;
    BlendType mDestBlend = BlendType::ZERO;
    BlendOp   mBlendOp = BlendOp::ADD;
    BlendType mSrcBlendAlpha = BlendType::ONE;
    BlendType mDestBlendAlpha = BlendType::ZERO;
    BlendOp   mBlendOpAlpha = BlendOp::ADD;
    BlendLogicOp mLogicOp = BlendLogicOp::NOOP;
    ColorChannelBitfield mWriteMask = ColorChannelBitfield({ColorChannel::RED, ColorChannel::GREEN, ColorChannel::BLUE, ColorChannel::ALPHA});
};

struct StencilOpDesc
{
    StencilOp mStencilFailOp = StencilOp::KEEP;
    StencilOp mStencilDepthFailOp = StencilOp::KEEP;
    StencilOp mStencilPassOp = StencilOp::KEEP;
    DepthFunc mStencilFunc = DepthFunc::ALWAYS;
};

struct DepthStencilStateDesc
{
    bool mDepthEnabled = true;
    bool mDepthWriteMaskAll = true;
    bool mStencilEnabled = false;
    UInt8 mStencilReadMask = 0xFF;
    UInt8 mStencilWriteMask = 0xFF;
    DepthFunc mDepthCompareFunc = DepthFunc::ALWAYS;
    StencilOpDesc mFrontFace;
    StencilOpDesc mBackFace;
};

struct LF_ABSTRACT_ENGINE_API VertexInputElement
{
    VertexInputElement()
        : mSemanticName()
        , mSemanticIndex(0)
        , mFormat(Gfx::ResourceFormat::R32G32B32A32_FLOAT)
        , mInputSlot(0)
        , mAlignedByteOffset(0)
        , mInstanceDataStepRate(0)
        , mPerVertexData(true)
    {}

    void Serialize(Stream& s);

    Token           mSemanticName;
    UInt32          mSemanticIndex;
    TResourceFormat mFormat;
    UInt32          mInputSlot;
    UInt32          mAlignedByteOffset;
    UInt32          mInstanceDataStepRate;
    bool            mPerVertexData;
};
LF_INLINE Stream& operator<<(Stream& s, VertexInputElement& o)
{
    o.Serialize(s);
    return s;
}

// ********************************************************************
// Parse a token into the correct shader attrib format enum.
// ********************************************************************
LF_ABSTRACT_ENGINE_API ShaderAttribFormat GetShaderAttribFormat(const Token& formatToken);

// ********************************************************************
// The base type for Graphics Resources
// 
// Resources are allocated from a GfxDevice but they are not tracked.
// Resources are low-level buffers (Textures/Vertex Buffers/Index Buffers etc)
// intended to be used by other graphics types.
//
// ********************************************************************
struct LF_ABSTRACT_ENGINE_API Resource
{
    enum ResourceType
    {
        RT_VERTEX_BUFFER,
        RT_INDEX_BUFFER,
        RT_TEXTURE_BUFFER,
        RT_VERTEX_SHADER,
        RT_PIXEL_SHADER
        // TODO: Maybe Constant Buffer
        // TODO: Maybe RW Texture
        // TODO: Maybe RW Struct
    };
    Resource(ResourceType resourceType) : mResourceType(resourceType) {}

    ResourceType GetType() const { return mResourceType; }
private:
    ResourceType mResourceType;
};
// Resources are kept alive via atomic smart pointers
using ResourcePtr = TAtomicStrongPointer<Resource>;

enum class UploadBufferType : Int32
{
    Constant,
    Structured,

    GfxUploadBufferType_MAX_VALUE
};

DECLARE_ENUM(ShaderParamType,
SPT_TEXTURE_2D,
SPT_CONSTANT_BUFFER,
SPT_STRUCTURED_BUFFER
);
DECLARE_ENUM(ShaderParamVisibility,
SPV_ALL,
SPV_PIXEL,
SPV_VERTEX
);
struct ShaderParamID
{
    using Value = UInt32;

    LF_INLINE ShaderParamID() : mID(INVALID32), mType(ShaderParamType::INVALID_ENUM) {}
    LF_INLINE explicit ShaderParamID(Value id, ShaderParamType::Value type) : mID(id), mType(type) {}

    LF_INLINE bool operator==(const ShaderParamID& other) const { return mID == other.mID && mType == other.mType; }
    LF_INLINE bool operator!=(const ShaderParamID& other) const { return mID != other.mID || mType != other.mType; }
    LF_INLINE bool operator<(const ShaderParamID& other) const { return mID < other.mID; }

    LF_INLINE bool IsValid() { return ::lf::Valid(mID) && ValidEnum(mType); }
    LF_INLINE bool IsTexture2D() { return mType == ShaderParamType::SPT_TEXTURE_2D; }
    LF_INLINE bool IsConstantBuffer() { return mType == ShaderParamType::SPT_CONSTANT_BUFFER; }
    LF_INLINE bool IsStructuredBuffer() { return mType == ShaderParamType::SPT_STRUCTURED_BUFFER; }

    Value                   mID;
    ShaderParamType::Value  mType;
};
LF_STATIC_ASSERT(sizeof(ShaderParamID) == 8);

class LF_ABSTRACT_ENGINE_API ShaderParam
{
public:
    ShaderParam()
        : mName()
        , mType(ShaderParamType::INVALID_ENUM)
        , mRegister(INVALID)
        , mElementSize(INVALID)
        , mElementCount(INVALID)
        , mVisibility(ShaderParamVisibility::INVALID_ENUM)
    {}

    void Serialize(Stream& s);

    const Token& GetName() const { return mName; }
    ShaderParamType::Value GetType() const { return mType; }
    SizeT GetRegister() const { return mRegister; }
    SizeT GetElementSize() const { return mElementSize; }
    SizeT GetElementCount() const { return mElementCount; }
    ShaderParamVisibility::Value GetVisibility() const { return mVisibility; }

    bool IsValid() const
    {
        return ValidEnum<ShaderParamType::Value>(mType) && Valid(mRegister) && Valid(mElementSize) && Valid(mElementCount) && ValidEnum<ShaderParamVisibility::Value>(mVisibility);
    }

    void Clear()
    {
        mName.Clear();
        mType = ShaderParamType::INVALID_ENUM;
        mRegister = mElementSize = mElementCount = INVALID;
        mVisibility = ShaderParamVisibility::INVALID_ENUM;
    }

    ShaderParam& InitTexture2D(const Token& name, SizeT shaderRegister)
    {
        Assert(!IsValid());

        mType = ShaderParamType::SPT_TEXTURE_2D;
        mName = name;
        mRegister = shaderRegister;
        mElementSize = mElementCount = 0;
        mVisibility = ShaderParamVisibility::SPV_PIXEL;
        return *this;
    }
    ShaderParam& InitConstantBuffer(const Token& name, SizeT shaderRegister, SizeT elementSize, SizeT elementCount)
    {
        Assert(!IsValid());

        mType = ShaderParamType::SPT_CONSTANT_BUFFER;
        mName = name;
        mRegister = shaderRegister;
        mElementSize = elementSize;
        mElementCount = elementCount;
        mVisibility = ShaderParamVisibility::SPV_ALL;
        return *this;
    }

    ShaderParam& InitStructuredBuffer(const Token& name, SizeT shaderRegister, SizeT elementSize, SizeT elementCount)
    {
        Assert(!IsValid());

        mType = ShaderParamType::SPT_STRUCTURED_BUFFER;
        mName = name;
        mRegister = shaderRegister;
        mElementSize = elementSize;
        mElementCount = elementCount;
        mVisibility = ShaderParamVisibility::SPV_ALL;
        return *this;
    }

    template<typename T>
    ShaderParam& InitConstantBuffer(const Token& name, SizeT shaderRegister)
    {
        return InitConstantBuffer(name, shaderRegister, sizeof(T), 1);
    }

    template<typename T>
    ShaderParam& InitStructuredBuffer(const Token& name, SizeT shaderRegister)
    {
        return InitStructuredBuffer(name, shaderRegister, sizeof(T), 1);
    }
private:
    Token                       mName;
    TShaderParamType            mType;
    SizeT                       mRegister;
    SizeT                       mElementSize;  // constant/structured buffer
    SizeT                       mElementCount; // constant/structured buffer
    TShaderParamVisibility      mVisibility;
};
LF_INLINE Stream& operator<<(Stream& s, ShaderParam& o)
{
    o.Serialize(s);
    return s;
}

} // namespace Gfx

namespace ShaderCompilationError
{
LF_ABSTRACT_ENGINE_API ErrorBase* Create(const ErrorBase::Info& info, const char* errorMessage, const char* shader);
}

} // namespace