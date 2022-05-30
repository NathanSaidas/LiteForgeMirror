// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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
#include "GfxResourceObject.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "AbstractEngine/Gfx/GfxBase.h"

namespace lf {

DEFINE_CLASS(lf::GfxResourceObject) { NO_REFLECTION; }

GfxResourceObject::GfxResourceObject()
: Super()
, mService(nullptr)
, mAssetType()
{}

GfxResourceObject::~GfxResourceObject()
{}
bool GfxResourceObject::Initialize(GfxDependencyContext& context)
{
    mService = context.GetGfxDevice();
    return mService != nullptr;
}
void GfxResourceObject::Release()
{
    mService = nullptr;
}

SizeT GfxResourceObject::GetRequestedDescriptors() const
{
    return 0;
}

void GfxResourceObject::SetAssetType(const AssetTypeInfo* type)
{
    mAssetType = AssetTypeInfoCPtr(type);
}

Int32 GfxResourceObject::RegisterInvalidate(const InvalidateCallback& callback)
{
    return mInvalidateEventBus.Register(callback);
}

void GfxResourceObject::UnregisterInvalidate(Int32 id)
{
    mInvalidateEventBus.Unregister(id);
}

void GfxResourceObject::Commit(GfxDevice& , GfxCommandContext& )
{

}

void GfxResourceObject::Invalidate()
{
    mInvalidateEventBus.Invoke(this);
}


} // namespace lf 