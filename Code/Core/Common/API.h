// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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

#include "Types.h"

#if defined(LF_INTERNAL_EXPORT_CORE)
#define LF_CORE_API LF_EXPORT_DLL
#elif defined(LF_INTERNAL_IMPORT_CORE)
#define LF_CORE_API LF_IMPORT_DLL
#else
#define LF_CORE_API
#endif

#if defined(LF_INTERNAL_EXPORT_RUNTIME)
#define LF_RUNTIME_API LF_EXPORT_DLL
#elif defined(LF_INTERNAL_IMPORT_RUNTIME)
#define LF_RUNTIME_API LF_IMPORT_DLL
#else
#define LF_RUNTIME_API
#endif

#if defined(LF_INTERNAL_EXPORT_ABSTRACT_ENGINE)
#define LF_ABSTRACT_ENGINE_API LF_EXPORT_DLL
#elif defined(LF_INTERNAL_IMPORT_ABSTRACT_ENGINE)
#define LF_ABSTRACT_ENGINE_API LF_IMPORT_DLL
#else
#define LF_ABSTRACT_ENGINE_API
#endif

#if defined(LF_INTERNAL_EXPORT_SERVICE)
#define LF_SERVICE_API LF_EXPORT_DLL
#elif defined(LF_INTERNAL_IMPORT_SERVICE)
#define LF_SERVICE_API LF_IMPORT_DLL
#else
#define LF_SERVICE_API
#endif

#if defined(LF_INTERNAL_EXPORT_ENGINE)
#define LF_ENGINE_API LF_EXPORT_DLL
#elif defined(LF_INTERNAL_IMPORT_ENGINE)
#define LF_ENGINE_API LF_IMPORT_DLL
#else
#define LF_ENGINE_API
#endif
