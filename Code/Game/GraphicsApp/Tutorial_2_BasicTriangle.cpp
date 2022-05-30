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
#include "Tutorial_2_BasicTriangle.h"
#include "Core/Input/InputBinding.h"
#include "AbstractEngine/App/AppWindow.h"
#include "AbstractEngine/App/AppService.h"
#include "AbstractEngine/Input/InputMgr.h"
#include "AbstractEngine/Gfx/GfxSwapChain.h"
#include "Engine/DX12/DX12GfxDevice.h"
#include "Engine/Gfx/GameRenderer.h"

namespace lf {

DEFINE_CLASS(lf::Graphics_BasicTriangle) { NO_REFLECTION; }


Graphics_BasicTriangle::Graphics_BasicTriangle()
: Super()
, mAppService(nullptr)
, mGfxDevice(nullptr)
, mInputMgr(nullptr)
, mQuitBinding()
, mWindow()
{

}
Graphics_BasicTriangle::~Graphics_BasicTriangle()
{

}

APIResult<ServiceResult::Value> Graphics_BasicTriangle::OnStart()
{
    auto result = Super::OnStart();
    if (result == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return result;
    }

    mAppService = GetServices()->GetService<AppService>();
    mGfxDevice = GetServices()->GetService<DX12GfxDevice>();
    mInputMgr = GetServices()->GetService<InputMgr>();

    if (!mAppService || !mGfxDevice || !mInputMgr)
    {
        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_FAILED);
    }

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> Graphics_BasicTriangle::OnPostInitialize()
{
    auto result = Super::OnPostInitialize();
    if (result == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return result;
    }

    mWindow = mAppService->MakeWindow("MainWindow", "Tutorial 2 Basic Triangle", 640, 640);
    mSwapChain = mGfxDevice->CreateSwapChain(mWindow);
    mWindow->Show();

    mRenderer = MakeConvertibleAtomicPtr<GameRenderer>();
    mRenderer->SetAssetProvider(CreateDebugAssetProvider());
    // As soon as the renderer is hooked it up, calls are made to it, to render.
    mGfxDevice->Register(mRenderer);
    mRenderer->SetWindow(mSwapChain);

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> Graphics_BasicTriangle::OnEndFrame()
{
    auto result = Super::OnEndFrame();
    if (result == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return result;
    }

    if (!mWindow || !mWindow->IsOpen())
    {
        mGfxDevice->Unregister(mRenderer);
        mRenderer = NULL_PTR;

        mAppService->Stop();
        mSwapChain = NULL_PTR;
    }
    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

} // namespace lf