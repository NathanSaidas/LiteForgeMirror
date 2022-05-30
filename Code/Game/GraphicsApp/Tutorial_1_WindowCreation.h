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
#pragma once

#include "Core/Utility/Time.h"
#include "AbstractEngine/App/AppWindow.h"
#include "Game/GraphicsApp/GraphicsServiceBase.h"

namespace lf {
DECLARE_ATOMIC_PTR(AppWindow);
DECLARE_ATOMIC_PTR(GfxVertexBuffer);
DECLARE_ATOMIC_PTR(GfxSwapChain);
DECLARE_ATOMIC_PTR(InputBinding);
DECLARE_PTR(DX12GfxDevice);
class AppService;
class InputMgr;

// Run with -app /type=GraphicsAppBase /tutorial=CreateWindow
class Graphics_WindowCreation : public GraphicsServiceBase
{
    DECLARE_CLASS(Graphics_WindowCreation, GraphicsServiceBase);
public:
    Graphics_WindowCreation();
    ~Graphics_WindowCreation();

    APIResult<ServiceResult::Value> OnStart() override;
    APIResult<ServiceResult::Value> OnPostInitialize() override;
    APIResult<ServiceResult::Value> OnFrameUpdate() override;

private:
    AppService*             mAppService; // Application to create window with/quit on requested.
    DX12GfxDevice*          mGfxDevice;  // Gfx Device to create gfx resource with
    InputMgr*               mInputMgr;   // InputMgr to listen for input events
    InputBindingAtomicPtr   mQuitBinding;// Input binding to quit the app
    AppWindowAtomicPtr      mWindow;     // Apps window
};

} // namespace lf