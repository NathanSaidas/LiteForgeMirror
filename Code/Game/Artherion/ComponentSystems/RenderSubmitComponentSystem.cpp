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
#include "RenderSubmitComponentSystem.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/Log.h"


namespace lf
{


void RenderSubmitComponentSystem::OnBindTuples()
{
    // Bind the tuple
    BindTuple(mTuple);

    // Accessor to a specific entity
    GetEntity(mTuple, 0, 0, [this](TransformComponentData* transform, ModelComponentData* model, BoundsComponentData* bounds)
        {
            (transform);
            (model);
            (bounds);
            LF_DEBUG_BREAK;
        });

    mUpdate = UpdateCallback::Make(this, &RenderSubmitComponentSystem::Update);
    // Iterate method to iterate on all entities.
    ForEach(mTuple, mUpdate);

    // Iterate on a specific collection (could Fork & Join w/ multiple threads)
    ForEach(mTuple, 0, mUpdate);


    ForEachEntity(mTuple, [this](EntityId entity, TransformComponentData* transform, ModelComponentData* model, BoundsComponentData* bounds) {
        (entity);
        (transform);
        (model);
        (bounds);



    });
}

void RenderSubmitComponentSystem::Update(TransformComponentData* transform, ModelComponentData* model, BoundsComponentData* bounds)
{
    (transform);
    (model);
    (bounds);
    LF_DEBUG_BREAK;

    
    
    gSysLog.Info(LogMessage("Updating transform, model, bounds..."));
}

}