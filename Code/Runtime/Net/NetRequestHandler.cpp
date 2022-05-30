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
#include "Runtime/PCH.h"
#include "NetRequestHandler.h"
#include "Core/Reflection/Type.h"
#include "Core/IO/TextStream.h"
#include "Runtime/Net/NetRequest.h"
#include "Runtime/Reflection/ReflectionMgr.h"

namespace lf
{
DEFINE_ABSTRACT_CLASS(lf::NetRequestHandler) { NO_REFLECTION; }

NetRequestHandler::NetRequestHandler()
: Super()
{

}
NetRequestHandler::~NetRequestHandler()
{

}

NetRequestAtomicPtr NetRequestHandler::CreateRequest(const ByteT* requestBytes, SizeT requestByteLength) const
{
    String text(requestByteLength, reinterpret_cast<const char*>(requestBytes), COPY_ON_WRITE);
    TextStream ts(Stream::TEXT, &text, Stream::SM_READ);
    for (const Type* type : mAcceptableTypes)
    {
        String typeName(type->GetFullName().CStr(), COPY_ON_WRITE);
        String superName(type->GetSuper()->GetFullName().CStr(), COPY_ON_WRITE);
        if (ts.BeginObject(typeName, superName))
        {
            NetRequestAtomicPtr request = GetReflectionMgr().CreateAtomic<NetRequest>(type);
            if (request)
            {
                request->Serialize(ts);
            }
            ts.EndObject();
            return request;
        }
    }
    return NULL_PTR;
}

bool NetRequestHandler::AcceptType(const Type* type)
{
    if (!type || !type->IsA(typeof(NetRequest)))
    {
        return false;
    }

    auto iter = std::find(mAcceptableTypes.begin(), mAcceptableTypes.end(), type);
    if (iter != mAcceptableTypes.end())
    {
        return false;
    }
    mAcceptableTypes.push_back(type);
    return true;
}

} // namespace lf