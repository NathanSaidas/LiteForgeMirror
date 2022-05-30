// ********************************************************************
// Copyright (c) 2022 Nathan Hanlan
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
#include "Engine/World/ComponentTypes/WorldDataComponent.h"
#include "Engine/Gfx/ComponentTypes/ProceduralMeshComponent.h"

namespace lf {
DECLARE_ATOMIC_PTR(GameRenderer);

class LF_ENGINE_API GfxModelRenderSetupComponentSystem : public ComponentSystem
{
    DECLARE_CLASS(GfxModelRenderSetupComponentSystem, ComponentSystem);
public:
    GfxModelRenderSetupComponentSystem();
    ~GfxModelRenderSetupComponentSystem();

    // { Position, TexCoord, Color, Normal }

   


    //
    // Procedural_position
    // Procedural_texture
    // Procedural_basic

    struct ProceduralTuple
    {
        using TupleType = TComponentSystemTuple<WorldDataComponent, ProceduralMeshComponent>;

        TComponentTupleType<WorldDataComponent>      mWorldDatas;
        TComponentTupleType<ProceduralMeshComponent> mProceduralMeshes;
        TVector<EntityCollection*>                   mEntities;
    };

    void SetDebugByteCode(const MemoryBuffer& vertexByteCode, const MemoryBuffer& pixelByteCode);
    void SetGameRenderer(const GameRendererAtomicPtr& gameRenderer);
protected:
    bool OnInitialize() override;
    void OnBindTuples() override;
    void OnScheduleUpdates() override;

private:
    void Update();

    using ProceduralUpdate = TCallback<void, WorldDataComponentData*, ProceduralMeshComponentData*>;
    void UpdateProcedural(WorldDataComponentData* worldData, ProceduralMeshComponentData* procedural);

    ProceduralTuple mProceduralTuple;

    bool mRegistered;

    MemoryBuffer mDebugPixelByteCode;
    MemoryBuffer mDebugVertexByteCode;
    GameRendererAtomicPtr mGameRenderer;

};

}
