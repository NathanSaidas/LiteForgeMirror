#pragma once
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
#include "Runtime/Asset/AssetOp.h"

namespace lf {

// This asset operation is started on the main thread and is similar to the import
// operation. It's used to remove the runtime-references of an asset type.
//
// Threads: [ Main Thread ]
// Supported Modes: [ Developer Mode, Modder Mode, Game Mode ]
//
// 1. Mark the 'AssetTypeInfo' as unloaded
// 2. Remove the 'AssetTypeInfo' from the DataController
// 3. Unload the asset runtime from the AssetPrototype
//
// Creators of the 'Asset Instances' should gracefully cleanup instances that are 'unavailable'
class LF_RUNTIME_API AssetRemoveOp : public AssetOp
{
public:

};

} // namespace lf