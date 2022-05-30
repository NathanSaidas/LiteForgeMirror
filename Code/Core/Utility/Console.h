#pragma once
#include "Core/Common/API.h"
#include <Windows.h>
#include "Core/String/String.h"
namespace lf {
    class LF_CORE_API Console
    {
    public:
        Console();

        bool Create();
        void Destroy();
        
        void Write(const String& string);
        String Read();

        HANDLE mInputHandle;
        HANDLE mOutputHandle;
    };
}