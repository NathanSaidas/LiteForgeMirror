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
#include "GraphicsAppBase.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/Log.h"
#include "AbstractEngine/App/AppService.h"
#include "Engine/Win32Input/Win32InputMgr.h"
#include "Engine/World/WorldImpl.h"
#include "Engine/DX12/DX12GfxDevice.h"
#include "Game/GraphicsApp/Tutorial_1_WindowCreation.h"
#include "Game/GraphicsApp/Tutorial_2_BasicTriangle.h"
#include "Game/GraphicsApp/Tutorial_3_Texturing.h"

namespace lf {

    DEFINE_CLASS(lf::GraphicsAppBase) { NO_REFLECTION; }

    ServiceResult::Value GraphicsAppBase::RegisterServices()
    {
        auto appService = MakeService<AppService>();
        if (!GetServices().Register(appService))
        {
            return ServiceResult::SERVICE_RESULT_FAILED;
        }
        if (!GetServices().Register(MakeService<Win32InputMgr>()))
        {
            return ServiceResult::SERVICE_RESULT_FAILED;
        }
        if (!GetServices().Register(MakeService<DX12GfxDevice>()))
        {
            return ServiceResult::SERVICE_RESULT_FAILED;
        }
        if (!GetServices().Register(MakeService<WorldImpl>()))
        {
            return ServiceResult::SERVICE_RESULT_FAILED;
        }

        // See headers for how to run with a specific service
        String tutorial;
        if (CmdLine::GetArgOption("app", "tutorial", tutorial))
        {
            if (StrCompareAgnostic(tutorial, "windowcreation"))
            {
                if (!GetServices().Register(MakeService<Graphics_WindowCreation>()))
                {
                    return ServiceResult::SERVICE_RESULT_FAILED;
                }
            }
            else if (StrCompareAgnostic(tutorial, "basictriangle"))
            {
                if (!GetServices().Register(MakeService<Graphics_BasicTriangle>()))
                {
                    return ServiceResult::SERVICE_RESULT_FAILED;
                }
            }
            else if (StrCompareAgnostic(tutorial, "texturing"))
            {
                if (!GetServices().Register(MakeService<Graphics_Texturing>()))
                {
                    return ServiceResult::SERVICE_RESULT_FAILED;
                }
            }
            else
            {
                gSysLog.Error(LogMessage("Unsupported tutorial specified \"") << tutorial << "\"");
                return ServiceResult::SERVICE_RESULT_FAILED;
            }
        }
        else
        {
            gSysLog.Error(LogMessage("Missing tutorial option. Please specify a tutorial option with \"-app /tutorial=<tutorial name here>\""));
            return ServiceResult::SERVICE_RESULT_FAILED;
        }
        

        appService->SetRunning();

        return ServiceResult::SERVICE_RESULT_SUCCESS;
    }

} // namespace lf