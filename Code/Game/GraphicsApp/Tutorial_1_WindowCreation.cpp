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
#include "Tutorial_1_WindowCreation.h"
#include "Core/Input/InputBinding.h"
#include "Core/Input/InputEvents.h"
#include "AbstractEngine/App/AppService.h"
#include "AbstractEngine/Input/InputMgr.h"
#include "Engine/DX12/DX12GfxDevice.h"

namespace lf {

DEFINE_CLASS(lf::Graphics_WindowCreation) { NO_REFLECTION; }

Graphics_WindowCreation::Graphics_WindowCreation()
: Super()
, mAppService(nullptr)
, mGfxDevice(nullptr)
, mInputMgr(nullptr)
, mQuitBinding()
, mWindow()
{

}
Graphics_WindowCreation::~Graphics_WindowCreation()
{

}

APIResult<ServiceResult::Value> Graphics_WindowCreation::OnStart()
{
    APIResult<ServiceResult::Value> result = Super::OnStart();
    if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
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

APIResult<ServiceResult::Value> Graphics_WindowCreation::OnPostInitialize()
{
    APIResult<ServiceResult::Value> result = Super::OnPostInitialize();
    if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return result;
    }

    // Create our window
    mWindow = mAppService->MakeWindow("GameWindow", "Atherion", 640, 640);
    // Associate a swap chain for rendering
    mGfxDevice->CreateSwapChain(mWindow);

    // Initialize input binding (press q to quit)
    const Token filter(DefaultInputFilter());
    InputBindingData bindingData(InputEventType::BUTTON_PRESSED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::Q);
    mQuitBinding = MakeConvertibleAtomicPtr<InputBinding>(); 
    if (mQuitBinding->InitializeAction(filter) && mQuitBinding->CreateAction(bindingData))
    {
        mQuitBinding->OnEvent([this](const InputEvent& event)
            {
                Assert(event.mInputCode == InputCode::Q);
                Assert(event.mInputType == InputType::BINARY);
                Assert(event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::PRESSED)] == true);

                // Stop the application gracefully if we press Q
                mAppService->Stop();
            });

        mInputMgr->RegisterBinding(Token("Quit"), filter, mQuitBinding);
        mInputMgr->PushInputFilter(filter);
    }

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> Graphics_WindowCreation::OnFrameUpdate()
{
    APIResult<ServiceResult::Value> result = Super::OnFrameUpdate();
    if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return result;
    }

    // Stop the application should the window be closed.
    if (!mWindow || !mWindow->IsOpen())
    {
        mWindow.Release();
        mAppService->Stop();
    }


    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}


} // namespace lf