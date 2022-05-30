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
#pragma once
#include "AbstractEngine/World/ComponentSystem.h"
#include "Engine/World/ComponentSystemTuple.h"

#include "AbstractEngine/World/World.h"

#include "Game/Artherion/ComponentTypes/TransformComponent.h"
#include "Game/Artherion/ComponentTypes/BoundsComponent.h"
#include "Game/Artherion/ComponentTypes/ModelComponent.h"

namespace lf
{



class RenderSubmitComponentSystem : public ComponentSystem
{
public:
    using UpdateCallback = TCallback<void, TransformComponentData*, ModelComponentData*, BoundsComponentData*>;

    // Define the data we want
    struct RenderableEntityTuple
    {
        // define the builder type (used internally to bind data for us!)
        using TupleType = TComponentSystemTuple<TransformComponent, ModelComponent, BoundsComponent>;

        TComponentTupleType<TransformComponent> mTransforms;
        TComponentTupleType<ModelComponent>     mModels;
        TComponentTupleType<BoundsComponent>    mBounds;
        TVector<EntityCollection*>               mEntities;
    };

    void OnBindTuples() override;
    void Update(TransformComponentData* transform, ModelComponentData* model, BoundsComponentData* bounds);

    RenderableEntityTuple mTuple;
    UpdateCallback mUpdate;
};
}
