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
#include "Engine/PCH.h"
#include "DX11GfxDevice.h"
#include "Core/Math/Color.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Error.h"
#include "Runtime/Reflection/ReflectionMgr.h"
#include "AbstractEngine/Gfx/GfxShader.h"
#include "AbstractEngine/Gfx/GfxShaderBinary.h"
#include "AbstractEngine/Gfx/GfxShaderText.h"
#include "AbstractEngine/Gfx/GfxShaderUtil.h"
#include "Engine/DX11/DX11GfxDependencyContext.h"
#include "Engine/DX11/DX11GfxWindow.h"
#include "Engine/DX11/DX11GfxMaterial.h"

namespace lf
{
#if defined(LF_DIRECTX11)

DECLARE_ATOMIC_PTR(GfxWindowAdapter);
DEFINE_CLASS(lf::DX11GfxDevice) { NO_REFLECTION; }

namespace Gfx
{
    struct DX11BufferResource : public Resource
    {
        DX11BufferResource(ResourceType resourceType)
        : Resource(resourceType)
        , mNumElements(0)
        , mUsage(Gfx::BufferUsage::STATIC)
        , mBuffer()
    {}


        SizeT                mNumElements;
        Gfx::BufferUsage     mUsage;
        ComPtr<ID3D11Buffer> mBuffer;
    };

    struct DX11VertexBuffer : public DX11BufferResource
    {
        DX11VertexBuffer() 
        : DX11BufferResource(ResourceType::RT_VERTEX_BUFFER)
        , mStride(0)
        {}

        SizeT Capacity() const { return mStride * mNumElements; }

        SizeT                mStride;
    };
    using DX11VertexBufferPtr = TStrongPointer<DX11VertexBuffer>;

    struct DX11IndexBuffer : public DX11BufferResource
    {
        DX11IndexBuffer()
        : DX11BufferResource(ResourceType::RT_INDEX_BUFFER)
        , mStride(DXGI_FORMAT_UNKNOWN)
        {}

        SizeT Capacity() const
        {
            switch (mStride)
            {
            case DXGI_FORMAT_R16_UINT: return mNumElements * sizeof(UInt16);
            case DXGI_FORMAT_R32_UINT: return mNumElements * sizeof(UInt32);
            default:
                break;
            }
            return 0;
        }

        DXGI_FORMAT          mStride;
    };
    using DX11IndexBufferPtr = TStrongPointer<DX11IndexBuffer>;

    struct DX11VertexShader : public Resource
    {
        DX11VertexShader()
        : Resource(ResourceType::RT_VERTEX_SHADER)
        {}

        ComPtr<ID3D11VertexShader> mShader;
    };
    using DX11VertexShaderPtr = TAtomicStrongPointer<DX11VertexShader>;

    struct DX11PixelShader : public Resource
    {
        DX11PixelShader()
        : Resource(ResourceType::RT_PIXEL_SHADER)
        {}

        ComPtr<ID3D11PixelShader> mShader;
    };
    using DX11PixelShaderPtr = TAtomicStrongPointer<DX11PixelShader>;

}

DX11GfxDevice::DX11GfxDevice()
: Super()
, mFactory()
, mDevice()
, mDeviceContext()
, mCurrentWindow()
, mCurrentMaterial()
{

}
DX11GfxDevice::~DX11GfxDevice()
{

}

APIResult<ServiceResult::Value> DX11GfxDevice::OnStart()
{
    auto superResult = Super::OnStart();
    if (superResult != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return superResult;
    }

    // TODO: We can read the AppService::GetConfig() for flags
    GfxDeviceFlagsBitfield flags;
    flags.Set(GfxDeviceFlags::GDF_DEBUG);

    // Initialize our instance factory.
    mFactory.Initialize();

    // Create DX11 device
    UINT creationFlags = 0;
#if defined(LF_DEBUG) || defined(LF_TEST)
    if (flags.Has(GfxDeviceFlags::GDF_DEBUG))
    {
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }
#endif
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    HRESULT result = D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        creationFlags,
        &featureLevel, 1,
        D3D11_SDK_VERSION,
        mDevice.ReleaseAndGetAddressOf(),
        NULL,
        mDeviceContext.ReleaseAndGetAddressOf());

    if (FAILED(result))
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Device creation error, D3D11CreateDevice failed with error ", ToHexString(result).CStr());
    }
    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> DX11GfxDevice::OnFrameUpdate()
{
    auto superResult = Super::OnFrameUpdate();
    if(superResult != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return superResult;
    }

    mFactory.Update();

    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> DX11GfxDevice::OnShutdown(ServiceShutdownMode mode)
{
    mFactory.Shutdown();

    mDeviceContext.Reset();
    mDevice.Reset();

    auto superResult = Super::OnShutdown(mode);
    if (superResult != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return superResult;
    }
    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

bool DX11GfxDevice::Initialize(GfxDeviceFlagsBitfield )
{
    return false;
}
void DX11GfxDevice::Shutdown()
{

}

void DX11GfxDevice::Update()
{

}

APIResult<GfxWindowAdapterAtomicPtr> DX11GfxDevice::CreateWindow(const String& title, SizeT width, SizeT height, const String& classID)
{
    static const char* STANDARD_PLATFORM_CLASS = "DefaultLiteForgeWindow";
    APIResult<GfxAdapterAtomicPtr> result = CreateAdapter(typeof(GfxWindow));
    GfxWindowAdapterAtomicPtr window = DynamicCast<GfxWindowAdapterAtomicPtr>(result.GetItem());
    if (!window)
    {
        return APIResult<GfxWindowAdapterAtomicPtr>(GfxWindowAdapterAtomicPtr(), result);
    }

    DX11GfxDependencyContext dependencies(mDevice, mDeviceContext, GetServices());
    window->Initialize(dependencies, this, GfxObservedObject());

    // Set dynamic data
    window->SetTitle(title);
    window->SetWidth(static_cast<Int32>(width));
    window->SetHeight(static_cast<Int32>(height));
    window->SetClassName(classID.Empty() ? String(STANDARD_PLATFORM_CLASS, COPY_ON_WRITE) : classID);

    mFactory.RegisterAdapter(window, false);

    return APIResult<GfxWindowAdapterAtomicPtr>(window);
}

bool DX11GfxDevice::IsDebug() const
{
    return false;
}

APIResult<GfxObjectAtomicPtr> DX11GfxDevice::CreateObject(const Type* baseType)
{
    if (baseType == nullptr)
    {
        ReportBugMsg("Argument is null 'object'");
        return ReportError(GfxObjectAtomicPtr(), ArgumentNullError, "baseType");
    }

    if (!baseType->IsA(typeof(GfxObject)))
    {
        return ReportError(GfxObjectAtomicPtr(), InvalidArgumentError, "baseType", "Creating a graphics object requires a GfxObject");
    }

    GfxTypeFactory::TypeMapping mapping;
    if (!mFactory.GetMappedTypes(baseType, mapping))
    {
        return ReportError(GfxObjectAtomicPtr(), OperationFailureError, "Unable to create graphics object, the type is not mapped.", "baseType");
    }

    GfxAdapterAtomicPtr adapter = GetReflectionMgr().CreateAtomic<GfxAdapter>(mapping.mAdapterType, MMT_GRAPHICS);
    if (!adapter)
    {
        return APIResult<GfxObjectAtomicPtr>(GfxObjectAtomicPtr());
    }

    GfxObjectAtomicPtr object = GetReflectionMgr().CreateAtomic<GfxObject>(mapping.mImplementationType, MMT_GRAPHICS);
    if (!object)
    {
        return APIResult<GfxObjectAtomicPtr>(GfxObjectAtomicPtr());
    }

    // Hookup device
    object->SetDevice(this);
    object->SetAdapter(adapter);

    // Initialize Adapter
    DX11GfxDependencyContext dependencies(mDevice, mDeviceContext, GetServices());
    GfxObservedObject observed;
    observed.mInstance = object;
    adapter->Initialize(dependencies, this, observed);

    const bool attached = true;
    mFactory.RegisterAdapter(adapter, attached);

    return APIResult<GfxObjectAtomicPtr>(object);
}

APIResult<bool> DX11GfxDevice::CreateAdapter(GfxObject* object)
{
    if (object == nullptr)
    {
        ReportBugMsg("Argument is null 'object'");
        return ReportError(false, ArgumentNullError, "object");
    }

    if (object->GetAssetType() == nullptr)
    {
        return ReportError(false, InvalidArgumentError, "object", "Creating an adapter with a GfxObject that is not an asset is not allowed, to create a detached object use CreateAdapter(const Type*) instead.");
    }

    if (object->GetAdapter() || object->GetDevice())
    {
        return ReportError(false, InvalidArgumentError, "object", "Graphics object already has Adapter or Device");
    }

    if (!object->GetType())
    {
        return ReportError(false, InvalidArgumentError, "object", "Missing runtime type information.");
    }

    GfxTypeFactory::TypeMapping mapping;
    if (!mFactory.GetMappedTypes(object->GetType(), mapping))
    {
        gGfxLog.Warning(LogMessage("Attempting to initialize unmapped GfxObject. Type=") << object->GetType()->GetFullName());
        return APIResult<bool>(false);
    }

    GfxAdapterAtomicPtr adapter = GetReflectionMgr().CreateAtomic<GfxAdapter>(mapping.mAdapterType, MMT_GRAPHICS);
    if (!adapter)
    {
        return APIResult<bool>(false);;
    }

    // Hookup device
    object->SetDevice(this);
    object->SetAdapter(adapter);

    // Initialize Adapter
    DX11GfxDependencyContext dependencies(mDevice, mDeviceContext, GetServices());
    GfxObservedObject observed;
    if (object->GetAssetType() && object->GetAssetType()->IsPrototype(object))
    {
        observed.mType = GfxObjectAssetType(object->GetAssetType());
    }
    else
    {
        observed.mInstance = GetAtomicPointer(object);
    }
    adapter->Initialize(dependencies, this, observed);

    const bool attached = true;
    mFactory.RegisterAdapter(adapter, attached);
    
    return APIResult<bool>(true);
}

APIResult<GfxAdapterAtomicPtr> DX11GfxDevice::CreateAdapter(const Type* baseType)
{
    if (baseType == nullptr)
    {
        return ReportError(GfxAdapterAtomicPtr(), ArgumentNullError, "baseType");
    }

    GfxTypeFactory::TypeMapping mapping;
    if (!mFactory.GetMappedTypes(baseType, mapping))
    {
        gGfxLog.Warning(LogMessage("Attempting to initialize unmapped GfxObject. Type=") << baseType->GetFullName());
        return APIResult<GfxAdapterAtomicPtr>(GfxAdapterAtomicPtr());
    }

    GfxAdapterAtomicPtr adapter = GetReflectionMgr().CreateAtomic<GfxAdapter>(mapping.mAdapterType, MMT_GRAPHICS);
    if (!adapter)
    {
        return APIResult<GfxAdapterAtomicPtr>(NULL_PTR);;
    }

    // Initialize Adapter
    DX11GfxDependencyContext dependencies(mDevice, mDeviceContext, GetServices());
    adapter->Initialize(dependencies, this, GfxObservedObject());

    const bool attached = false;
    mFactory.RegisterAdapter(adapter, attached);

    return APIResult<GfxAdapterAtomicPtr>(adapter);
}

APIResult<Gfx::ResourcePtr> DX11GfxDevice::CreateVertexBuffer(SizeT numElements, SizeT stride, Gfx::BufferUsage usage, const void* initialData, SizeT initialDataSize)
{
    if (numElements == 0)
    {
        return ReportError(Gfx::ResourcePtr(), InvalidArgumentError, "numElements", "CreateVertexBuffer expects there to be at least one element.");
    }

    if (stride == 0)
    {
        return ReportError(Gfx::ResourcePtr(), InvalidArgumentError, "stride", "CreateVertexBuffer expects the stride to be at least 1 byte.");
    }

    if (InvalidEnum(usage))
    {
        return ReportError(Gfx::ResourcePtr(), InvalidArgumentError, "usage", "Invalid Enum");
    }

    if (initialData != nullptr && initialDataSize == 0)
    {
        gGfxLog.Warning(LogMessage("Attempting to create vertex buffer with initial data but the data size is 0. Ignoring initial data."));
        initialData = nullptr;
    }

    
    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.ByteWidth = static_cast<UINT>(numElements * stride);
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = Gfx::DX11CPUUsage(usage);
    desc.Usage = usage == Gfx::BufferUsage::DYNAMIC ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.MiscFlags = 0; // Unused 
    desc.StructureByteStride = 0; // Unused

    D3D11_SUBRESOURCE_DATA mappedRes;
    mappedRes.pSysMem = initialData;
    mappedRes.SysMemPitch = 0;
    mappedRes.SysMemSlicePitch = 0;

    ComPtr<ID3D11Buffer> buffer;
    HRESULT hr = mDevice->CreateBuffer(&desc, initialData ? &mappedRes : nullptr, &buffer);
    if (FAILED(hr))
    {
        return ReportError(Gfx::ResourcePtr(), OperationFailureError, "Failed to create vertex buffer", "<NONE>");
    }

    Gfx::DX11VertexBuffer* vertexBuffer = LFNew<Gfx::DX11VertexBuffer>();
    vertexBuffer->mBuffer = buffer;
    vertexBuffer->mNumElements = numElements;
    vertexBuffer->mStride = stride;
    vertexBuffer->mUsage = usage;
    return APIResult<Gfx::ResourcePtr>(Gfx::ResourcePtr(vertexBuffer));
}

APIResult<Gfx::ResourcePtr> DX11GfxDevice::CreateIndexBuffer(SizeT numElements, Gfx::IndexStride stride, Gfx::BufferUsage usage, const void* initialData, SizeT initialDataSize)
{
    if (numElements == 0)
    {
        return ReportError(Gfx::ResourcePtr(), InvalidArgumentError, "numElements", "CreateIndexBuffer expects there to be at least one element.");
    }

    if (InvalidEnum(stride))
    {
        return ReportError(Gfx::ResourcePtr(), InvalidArgumentError, "stride", "Invalid Enum");
    }

    if (InvalidEnum(usage))
    {
        return ReportError(Gfx::ResourcePtr(), InvalidArgumentError, "usage", "Invalid Enum");
    }

    if (initialData != nullptr && initialDataSize == 0)
    {
        gGfxLog.Warning(LogMessage("Attempting to create vertex buffer with initial data but the data size is 0. Ignoring initial data."));
        initialData = nullptr;
    }

    SizeT strideBytes = 0;
    DXGI_FORMAT dx11Stride = DXGI_FORMAT_UNKNOWN;
    switch (stride)
    {
        case Gfx::IndexStride::SHORT: 
            strideBytes = 2; 
            dx11Stride = DXGI_FORMAT_R16_UINT;
            break;

        case Gfx::IndexStride::INT: 
            strideBytes = 4; 
            dx11Stride = DXGI_FORMAT_R32_UINT;
            break;
        default:
            CriticalAssertMsg("Invalid enum");
            break;
    }

    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.ByteWidth = static_cast<UINT>(numElements * strideBytes);
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = Gfx::DX11CPUUsage(usage);
    desc.Usage = usage == Gfx::BufferUsage::DYNAMIC ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.MiscFlags = 0; // Unused 
    desc.StructureByteStride = 0; // Unused

    D3D11_SUBRESOURCE_DATA mappedRes;
    mappedRes.pSysMem = initialData;
    mappedRes.SysMemPitch = 0;
    mappedRes.SysMemSlicePitch = 0;

    ComPtr<ID3D11Buffer> buffer;
    HRESULT hr = mDevice->CreateBuffer(&desc, initialData ? &mappedRes : nullptr, &buffer);
    if (FAILED(hr))
    {
        return ReportError(Gfx::ResourcePtr(), OperationFailureError, "Failed to create index buffer", "<NONE>");
    }

    Gfx::DX11IndexBuffer* indexBuffer(LFNew<Gfx::DX11IndexBuffer>());
    indexBuffer->mBuffer = buffer;
    indexBuffer->mNumElements = numElements;
    indexBuffer->mStride = dx11Stride;
    indexBuffer->mUsage = usage;
    return APIResult<Gfx::ResourcePtr>(Gfx::ResourcePtr(indexBuffer));
}

APIResult<bool> DX11GfxDevice::CopyVertexBuffer(Gfx::Resource* buffer, SizeT numElements, SizeT stride, const void* vertexData)
{
    if (!buffer)
    {
        return ReportError(false, ArgumentNullError, "vertexBuffer");
    }

    if (!vertexData)
    {
        return ReportError(false, ArgumentNullError, "vertexData");
    }

    if (buffer->GetType() != Gfx::Resource::RT_VERTEX_BUFFER)
    {
        return ReportError(false, InvalidArgumentError, "vertexBuffer", "Resource is not a vertex buffer.");
    }

    if (numElements == 0 || stride == 0)
    {
        return APIResult<bool>(false);
    }

    Gfx::DX11VertexBuffer* vertexBuffer = static_cast<Gfx::DX11VertexBuffer*>(buffer);
    if (!vertexBuffer->mBuffer.Get())
    {
        return APIResult<bool>(false); // TODO: Allocate resource.
    }

    if (vertexBuffer->mUsage == Gfx::BufferUsage::STATIC)
    {
        return ReportError(false, OperationFailureError, "CopyVertexBuffer failed, cannot modify read-only vertex buffer.", "<NONE>");
    }

    if (vertexBuffer->Capacity() < (numElements * stride))
    {
        return ReportError(false, OperationFailureError, "CopyVertexBuffer failed, Not enough memory", "<NONE>");
    }

    D3D11_MAPPED_SUBRESOURCE mappedRes;
    ZeroMemory(&mappedRes, sizeof(mappedRes));
    HRESULT hr = mDeviceContext->Map(vertexBuffer->mBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedRes);
    if (FAILED(hr))
    {
        return ReportError(false, OperationFailureError, "CopyVertexBuffer failed, API Error (ID3D11Context::Map)", "<NONE>");
    }
    memcpy(mappedRes.pData, vertexData, numElements * stride);
    mDeviceContext->Unmap(vertexBuffer->mBuffer.Get(), 0);
    return APIResult<bool>(true);
}

APIResult<bool> DX11GfxDevice::CopyIndexBuffer(Gfx::Resource* buffer, SizeT numElements, Gfx::IndexStride stride, const void* indexData)
{
    if (!buffer)
    {
        return ReportError(false, ArgumentNullError, "indexBuffer");
    }

    if (!indexData)
    {
        return ReportError(false, ArgumentNullError, "indexData");
    }

    if (buffer->GetType() != Gfx::Resource::RT_INDEX_BUFFER)
    {
        return ReportError(false, InvalidArgumentError, "indexBuffer", "Resource is not a index buffer.");
    }

    if (numElements == 0 || InvalidEnum(stride))
    {
        return APIResult<bool>(false);
    }

    Gfx::DX11IndexBuffer* indexBuffer = static_cast<Gfx::DX11IndexBuffer*>(buffer);
    if (!indexBuffer->mBuffer.Get())
    {
        return APIResult<bool>(false); // TODO: Allocate resource.
    }

    if (indexBuffer->mUsage == Gfx::BufferUsage::STATIC)
    {
        return ReportError(false, OperationFailureError, "CopyIndexBuffer failed, cannot modify read-only index buffer.", "<NONE>");
    }

    SizeT strideBytes = 0;
    DXGI_FORMAT dx11Stride = DXGI_FORMAT_UNKNOWN;
    switch (stride)
    {
    case Gfx::IndexStride::SHORT:
        strideBytes = 2;
        dx11Stride = DXGI_FORMAT_R16_UINT;
        break;

    case Gfx::IndexStride::INT:
        strideBytes = 4;
        dx11Stride = DXGI_FORMAT_R32_UINT;
        break;
    default:
        CriticalAssertMsg("Invalid enum");
        break;
    }

    if (indexBuffer->Capacity() < (numElements * strideBytes))
    {
        return ReportError(false, OperationFailureError, "CopyIndexBuffer failed, Not enough memory", "<NONE>");
    }

    D3D11_MAPPED_SUBRESOURCE mappedRes;
    ZeroMemory(&mappedRes, sizeof(mappedRes));
    HRESULT hr = mDeviceContext->Map(indexBuffer->mBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedRes);
    if (FAILED(hr))
    {
        return ReportError(false, OperationFailureError, "CopyIndexBuffer failed, API Error (ID3D11Context::Map)", "<NONE>");
    }
    memcpy(mappedRes.pData, indexData, numElements * strideBytes);
    mDeviceContext->Unmap(indexBuffer->mBuffer.Get(), 0);
    return APIResult<bool>(true);
}

void DX11GfxDevice::BeginFrame(const GfxWindowAdapterAtomicWPtr& window)
{
    if (mCurrentWindow)
    {
        ReportError(false, OperationFailureError, "BeginFrame failed, device already rendering. Possible missing call to EndFrame?", "<NONE>");
        return;
    }
    mCurrentWindow = window;

    Float32 width = static_cast<Float32>(window->GetWidth());
    Float32 height = static_cast<Float32>(window->GetHeight());

    SetViewport(0.0f, 0.0f, width, height, 0.0f, 1.0f);
    SetScissorRect(0.0f, 0.0f, width, height);
}
void DX11GfxDevice::EndFrame()
{
    mCurrentWindow = NULL_PTR;
}

void DX11GfxDevice::BindPipelineState(GfxMaterialAdapter* adapter)
{
    if (!adapter)
    {
        return;
    }

    if (!mCurrentWindow)
    {
        ReportError(false, OperationFailureError, "Cannot bind pipeline state without an output target. (Possibly forgot BeginFrame call?)", "<NONE>");
        return;
    }

    DX11GfxWindowAdapter* window = static_cast<DX11GfxWindowAdapter*>(mCurrentWindow.AsPtr());
    ID3D11RenderTargetView* view = window->GetRenderTargetView();
    if (!view)
    {
        return;
    }

    mCurrentMaterial = GetAtomicPointer(adapter);
    if (mCurrentMaterial)
    {
        DX11GfxMaterialAdapter* material = static_cast<DX11GfxMaterialAdapter*>(mCurrentMaterial.AsPtr());
        DX11GfxMaterialAdapter::PSO& pso = material->GetPipelineState();
        if (!material->UploadProperties())
        {
            gGfxLog.Warning(LogMessage("Failed to upload properties for material. (Binding without updated properties.)"));
        }

        // IA:
        mDeviceContext->IASetInputLayout(pso.mInputLayout.Get());
        mDeviceContext->IASetPrimitiveTopology(pso.mTopology);
        // mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        // VS:
        mDeviceContext->VSSetShader(pso.mVertexShader.Get(), nullptr, 0);
        mDeviceContext->VSSetConstantBuffers(0, 1, pso.mConstantBuffer.GetAddressOf());
        // RS:
        mDeviceContext->RSSetState(pso.mRasterState.Get());
        // PS:
        mDeviceContext->PSSetShader(pso.mPixelShader.Get(), nullptr, 0);
        mDeviceContext->PSSetConstantBuffers(0, 1, pso.mConstantBuffer.GetAddressOf());
        // OM:
        mDeviceContext->OMSetDepthStencilState(pso.mDepthState.Get(), 1); // TODO: Figure out why 1?
        if (pso.mUseDepth)
        {
            mDeviceContext->OMSetRenderTargets(1, &view, window->GetDepthStencilView());
        }
        else
        {
            mDeviceContext->OMSetRenderTargets(1, &view, nullptr);
        }
        const Float32 BLEND_MASK[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        mDeviceContext->OMSetBlendState(pso.mBlendState.Get(), BLEND_MASK, 0xFFFFFFFF);

    }
}
void DX11GfxDevice::UnbindPipelineState()
{
    mDeviceContext->VSSetShader(nullptr, nullptr, 0);
    mDeviceContext->PSSetShader(nullptr, nullptr, 0);
    mCurrentMaterial = NULL_PTR;
}

void DX11GfxDevice::BindVertexBuffer(Gfx::Resource* buffer)
{
    if (!buffer || buffer->GetType() != Gfx::Resource::RT_VERTEX_BUFFER)
    {
        return;
    }

    Gfx::DX11VertexBuffer* vertexBuffer = static_cast<Gfx::DX11VertexBuffer*>(buffer);
    UINT strides[] = { static_cast<UINT>(vertexBuffer->mStride) };
    UINT offsets[] = { 0 };
    ID3D11Buffer* buffers[] = { vertexBuffer->mBuffer.Get() };
    mDeviceContext->IASetVertexBuffers(0, 1, buffers, strides, offsets);
}

void DX11GfxDevice::UnbindVertexBuffer()
{
    mDeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
}

void DX11GfxDevice::BindIndexBuffer(Gfx::Resource* buffer)
{
    if (!buffer || buffer->GetType() != Gfx::Resource::RT_INDEX_BUFFER)
    {
        return;
    }

    Gfx::DX11IndexBuffer* indexBuffer = static_cast<Gfx::DX11IndexBuffer*>(buffer);
    mDeviceContext->IASetIndexBuffer(indexBuffer->mBuffer.Get(), indexBuffer->mStride, 0);
}

void DX11GfxDevice::UnbindIndexBuffer()
{
    mDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
}

void DX11GfxDevice::SetViewport(Float32 x, Float32 y, Float32 width, Float32 height, Float32 minDepth, Float32 maxDepth)
{
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.MinDepth = minDepth;
    viewport.MaxDepth = maxDepth;
    viewport.Width = width;
    viewport.Height = height;
    mDeviceContext->RSSetViewports(1, &viewport);
}
void DX11GfxDevice::SetScissorRect(Float32 x, Float32 y, Float32 width, Float32 height)
{
    RECT rect;
    rect.left = static_cast<LONG>(x);
    rect.right = static_cast<LONG>(width);
    rect.top = static_cast<LONG>(y);
    rect.bottom = static_cast<LONG>(height);

    mDeviceContext->RSSetScissorRects(1, &rect);
}

void DX11GfxDevice::ClearColor(const Color& color)
{
    if (mCurrentWindow)
    {
        DX11GfxWindowAdapter* window = static_cast<DX11GfxWindowAdapter*>(mCurrentWindow.AsPtr());
        if (window->GetRenderTargetView())
        {
            const Float32 clearColor[] = { color.r, color.g, color.b, color.a };
            mDeviceContext->ClearRenderTargetView(window->GetRenderTargetView(), clearColor);
        }
    }
}
void DX11GfxDevice::ClearDepth()
{
    if (mCurrentWindow)
    {
        DX11GfxWindowAdapter* window = static_cast<DX11GfxWindowAdapter*>(mCurrentWindow.AsPtr());
        if (window->GetDepthStencilView())
        {
            mDeviceContext->ClearDepthStencilView(window->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        }
    }
}
void DX11GfxDevice::SwapBuffers()
{
    if (mCurrentWindow)
    {
        DX11GfxWindowAdapter* window = static_cast<DX11GfxWindowAdapter*>(mCurrentWindow.AsPtr());
        if (window->GetSwapChain())
        {
            window->GetSwapChain()->Present(0, 0);
        }
    }
}

void DX11GfxDevice::Draw(SizeT vertexCount)
{
    mDeviceContext->Draw(static_cast<UINT>(vertexCount), 0);
}

void DX11GfxDevice::DrawIndexed(SizeT indexCount)
{
    mDeviceContext->DrawIndexed(static_cast<UINT>(indexCount), 0, 0);
}

bool DX11GfxDevice::QueryMappedTypes(const Type* baseType, const Type** implementation, const Type** adapter)
{
    if (baseType == nullptr)
    {
        return ReportError(false, ArgumentNullError, "baseType");
    }
    if (!baseType->IsA(typeof(GfxObject)))
    {
        return ReportError(false, InvalidTypeArgumentError, "baseType", typeof(GfxObject), baseType);
    }
    if (implementation == nullptr && adapter == nullptr)
    {
        return false;
    }

    GfxTypeFactory::TypeMapping mapping;
    if (!mFactory.GetMappedTypes(baseType, mapping))
    {
        return false;
    }
    if (implementation)
    {
        *implementation = mapping.mImplementationType;
    }
    if (adapter)
    {
        *adapter = mapping.mAdapterType;
    }
    return true;
}

APIResult<Gfx::ResourcePtr> DX11GfxDevice::CompileAndLoadShader(Gfx::ShaderType shaderType, const GfxShaderAsset& shader, const TVector<Token>& defines)
{
    if (!shader.IsLoaded())
    {
        return ReportError(Gfx::ResourcePtr(), InvalidArgumentError, "shader", "Shader not loaded.");
    }

    Gfx::ShaderHash hash = Gfx::ComputeHash(shaderType, shader.GetPath(), defines);

    auto& infoMap = mShaders[shader.GetPath()];
    auto shaderInfoIter = infoMap.find(hash);
    if (shaderInfoIter != infoMap.end())
    {
        auto& shaderInfo = shaderInfoIter->second;
        if (!shaderInfo.mResourceHandle)
        {
            return APIResult<Gfx::ResourcePtr>(LoadShader(shaderInfo));
        }
        return APIResult< Gfx::ResourcePtr>(shaderInfo.mResourceHandle);
    }
    else
    {
        ShaderInfo* info = CompileShaderInfo(hash, shaderType, shader, defines);
        if (!info)
        {
            return ReportError(Gfx::ResourcePtr(), OperationFailureError, "Failed to compile shader info.", "shader");
        }
        return APIResult<Gfx::ResourcePtr>(LoadShader(*info));
    }
}

APIResult<Gfx::ResourcePtr> DX11GfxDevice::LoadShader(Gfx::ShaderType shaderType, const GfxShaderAsset& shader, const TVector<Token>& defines)
{
    if (!shader.IsLoaded())
    {
        return ReportError(Gfx::ResourcePtr(), InvalidArgumentError, "shader", "Shader not loaded.");
    }

    Gfx::ShaderHash hash = Gfx::ComputeHash(shaderType, shader.GetPath(), defines);

    auto infoMapIter = mShaders.find(shader.GetPath());
    if (infoMapIter != mShaders.end())
    {
        auto infoIter = infoMapIter->second.find(hash);
        if (infoIter != infoMapIter->second.end())
        {
            auto& info = infoIter->second;
            return APIResult<Gfx::ResourcePtr>(LoadShader(info));
        }
    }
    return APIResult<Gfx::ResourcePtr>(NULL_PTR);
}

DX11GfxDevice::ShaderInfo* DX11GfxDevice::CompileShaderInfo(Gfx::ShaderHash hash, Gfx::ShaderType shaderType, const GfxShaderAsset& shader, const TVector<Token>& defines)
{
    CriticalAssert(shader.IsLoaded());

    String basePath = Gfx::ComputePath(shaderType, Gfx::GraphicsApi::DX11, shader.GetPath(), hash);
    AssetPath infoPath(basePath + ".shaderinfo");
    AssetPath dataPath(basePath + ".shaderdata");

    AssetTypeInfoCPtr infoType = GetAssetMgr().FindType(infoPath);
    AssetTypeInfoCPtr dataType = GetAssetMgr().FindType(dataPath);
    if (infoType != nullptr)
    {
        return nullptr; // Data is already created, we must purge from cache first.
    }

    if (dataType != nullptr)
    {
        return nullptr; // Data is already created, we must purge from cache first.
    }

    const GfxShaderTextAsset& shaderText = shader->GetText(Gfx::GraphicsApi::DX11);

    MemoryBuffer buffer;
    const String text = GenerateShaderText(shaderType, shaderText, defines);
    if (!CompileBinary(shaderType, text, buffer))
    {
        return nullptr;
    }

    auto binaryInfo = GetAssetMgr().CreateEditable<GfxShaderBinaryInfo>();
    auto binaryData = GetAssetMgr().CreateEditable<GfxShaderBinaryData>();

    binaryInfo->SetShaderType(shaderType);
    binaryInfo->SetShader(shader);
    binaryInfo->SetHash(hash);
    binaryInfo->SetDefines(defines);

    binaryData->SetBuffer(std::move(buffer));

    auto createInfoOp = GetAssetMgr().Create(infoPath, binaryInfo, nullptr);
    auto createDataOp = GetAssetMgr().Create(dataPath, binaryData, nullptr);

    if (!GetAssetMgr().Wait(createInfoOp) || !GetAssetMgr().Wait(createDataOp))
    {
        // TODO: Purge
        return nullptr;
    }

    auto& info = mShaders[shader.GetPath()][hash];
    info.mInfo = GfxShaderBinaryInfoAsset(infoPath, AssetLoadFlags::LF_RECURSIVE_PROPERTIES);
    info.mData = GfxShaderBinaryDataAssetType(dataPath);
    Assert(info.mInfo.IsLoaded());
    return &info;
}

String DX11GfxDevice::GenerateShaderText(Gfx::ShaderType shaderType, const GfxShaderTextAsset& text, const TVector<Token>& defines)
{
    CriticalAssert(text.IsLoaded());

    (shaderType);
    (defines); // TODO: Implement Parsing etc.
    return text->GetText();
}

bool DX11GfxDevice::CompileBinary(Gfx::ShaderType shaderType, const String& text, MemoryBuffer& buffer)
{
    (shaderType);
    (text);
    (buffer);
    return false;
}

Gfx::ResourcePtr DX11GfxDevice::LoadShader(ShaderInfo& shaderInfo)
{
    CriticalAssert(shaderInfo.mInfo.IsLoaded());
    GfxShaderBinaryDataAsset binary(shaderInfo.mData, AssetLoadFlags::LF_RECURSIVE_PROPERTIES);
    CriticalAssert(binary.IsLoaded());

    if (shaderInfo.mResourceHandle)
    {
        return shaderInfo.mResourceHandle;
    }

    switch (shaderInfo.mInfo->GetShaderType())
    {
        case Gfx::ShaderType::VERTEX:
        {
            Gfx::DX11VertexShaderPtr shader(LFNew<Gfx::DX11VertexShader>());
            HRESULT result = mDevice->CreateVertexShader(binary->GetBuffer().GetData(), binary->GetBuffer().GetSize(), nullptr, shader->mShader.ReleaseAndGetAddressOf());
            if (FAILED(result))
            {
                gGfxLog.Error(LogMessage("Failed to create vertex shader. DataPath=") << binary.GetPath().CStr());
            }
            else
            {
                shaderInfo.mResourceHandle = shader;
            }
        } break;
        case Gfx::ShaderType::PIXEL:
        {
            Gfx::DX11PixelShaderPtr shader(LFNew<Gfx::DX11PixelShader>());
            HRESULT result = mDevice->CreatePixelShader(binary->GetBuffer().GetData(), binary->GetBuffer().GetSize(), nullptr, shader->mShader.ReleaseAndGetAddressOf());
            if (FAILED(result))
            {
                gGfxLog.Error(LogMessage("Failed to create pixel shader. DataPath=") << binary.GetPath().CStr());
            }
            else
            {
                shaderInfo.mResourceHandle = shader;
            }
        } break;
        default:
            CriticalAssertMsg("Invalid Shader Type!");
            break;
    }

    return shaderInfo.mResourceHandle;
}
#endif
}