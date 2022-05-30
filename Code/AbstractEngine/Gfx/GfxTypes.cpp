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
#include "AbstractEngine/PCH.h"
#include "GfxTypes.h"
#include "Core/Utility/StandardError.h"

namespace lf {

STATIC_TOKEN(hlslFloat,"float");
STATIC_TOKEN(hlslInt,"int");
STATIC_TOKEN(hlslUInt,"uint");
STATIC_TOKEN(hlslVec2,"float2");
STATIC_TOKEN(hlslVec3,"float3");
STATIC_TOKEN(hlslVec4,"float4");
STATIC_TOKEN(hlslMat3,"float3x3");
STATIC_TOKEN(hlslMat4,"float4x4");
STATIC_TOKEN(hlslTexture,"Texture2D");
STATIC_TOKEN(hlslSampler,"SamplerState");

STATIC_TOKEN(glslFloat,"float");
STATIC_TOKEN(glslInt,"int");
STATIC_TOKEN(glslUInt, "uint");
STATIC_TOKEN(glslVec2, "vec2");
STATIC_TOKEN(glslVec3, "vec3");
STATIC_TOKEN(glslVec4, "vec4");
STATIC_TOKEN(glslMat3, "mat3x3");
STATIC_TOKEN(glslMat4, "mat4x4");
STATIC_TOKEN(glslTexture,"sampler2D");

Gfx::ShaderAttribFormat Gfx::GetShaderAttribFormat(const Token& formatToken)
{
    if (formatToken == hlslFloat || formatToken == hlslFloat)
    {
        return ShaderAttribFormat::SAF_FLOAT;
    }
    else if (formatToken == hlslInt || formatToken == glslInt)
    {
        return ShaderAttribFormat::SAF_INT;
    }
    else if (formatToken == hlslUInt || formatToken == glslUInt)
    {
        return ShaderAttribFormat::SAF_UINT;
    }
    else if (formatToken == hlslVec2 || formatToken == glslVec2)
    {
        return ShaderAttribFormat::SAF_VECTOR2;
    }
    else if (formatToken == hlslVec3 || formatToken == glslVec3)
    {
        return ShaderAttribFormat::SAF_VECTOR3;
    }
    else if (formatToken == hlslVec4 || formatToken == glslVec4)
    {
        return ShaderAttribFormat::SAF_VECTOR4;
    }
    else if (formatToken == hlslMat3 || formatToken == glslMat3)
    {
        return ShaderAttribFormat::SAF_MATRIX_3X3;
    }
    else if (formatToken == hlslMat4 || formatToken == glslMat4)
    {
        return ShaderAttribFormat::SAF_MATRIX_4x4;
    }
    else if (formatToken == hlslTexture || formatToken == glslTexture)
    {
        return ShaderAttribFormat::SAF_TEXTURE;
    }
    else if (formatToken == hlslSampler)
    {
        return ShaderAttribFormat::SAF_SAMPLER;
    }
    return ShaderAttribFormat::INVALID_ENUM;
}

void Gfx::VertexInputElement::Serialize(Stream& s)
{
    SERIALIZE(s, mSemanticName, "");
    SERIALIZE(s, mSemanticIndex, "");
    SERIALIZE(s, mFormat, "");
    SERIALIZE(s, mInputSlot, "");
    SERIALIZE(s, mAlignedByteOffset, "");
    SERIALIZE(s, mInstanceDataStepRate, "");
    SERIALIZE(s, mPerVertexData, "");
}


void Gfx::ShaderParam::Serialize(Stream& s)
{
    SERIALIZE(s, mName, "");
    SERIALIZE(s, mType, "");
    SERIALIZE(s, mRegister, "");
    SERIALIZE(s, mElementCount, "");
    SERIALIZE(s, mElementSize, "");
    SERIALIZE(s, mVisibility, "");
}

class ShaderShaderCompilationErrorType : public StandardError
{
public:
    ShaderShaderCompilationErrorType(const char* compilationError, const char* shader) : StandardError()
    {
        // Failed to compile shader "%s"\n
        static const auto errorMessage = StaticString("Failed to compile shader ");
        static const auto formatMsg = StaticString("\"\"\n");
        const SizeT stringSize =
            errorMessage.Size()
            + StrLen(shader)
            + formatMsg.Size()
            + StrLen(compilationError)
            + 1;

        PrintError(stringSize, "%s\"%s\"\n%s", errorMessage.CStr(), shader, compilationError);
    }
};
ErrorBase* ShaderCompilationError::Create(const ErrorBase::Info& info, const char* errorMessage, const char* shader)
{
    return ErrorUtil::MakeError<ShaderShaderCompilationErrorType>(info, errorMessage, shader);
}

} // namespace lf