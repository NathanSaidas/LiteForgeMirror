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
#include "Engine/Gfx/ComponentTypes/MeshRendererComponent.h"
#include "Engine/Gfx/ComponentTypes/MeshRendererFlagsComponent.h"
#include "Engine/Gfx/ComponentTypes/MeshSimpleComponent.h"
#include "Engine/Gfx/ComponentTypes/MeshTextureComponent.h"
#include "Engine/Gfx/ComponentTypes/MeshStandardComponent.h"


namespace lf {
DECLARE_ATOMIC_PTR(GameRenderer);

class LF_ENGINE_API MeshSetupComponentSystem : public ComponentSystem
{
    DECLARE_CLASS(MeshSetupComponentSystem, ComponentSystem);
public:
    struct SimpleTuple
    {
        using TupleType = TComponentSystemTuple<MeshRendererFlagsComponent, MeshRendererComponent, MeshSimpleComponent>;

        TComponentTupleType<MeshRendererFlagsComponent> mRendererFlagsComponents;
        TComponentTupleType<MeshRendererComponent>      mRendererComponents;
        TComponentTupleType<MeshSimpleComponent>        mMeshComponents;
        TVector<EntityCollection*>                      mEntities;
    };

    struct TextureTuple
    {
        using TupleType = TComponentSystemTuple<MeshRendererFlagsComponent, MeshRendererComponent, MeshTextureComponent>;

        TComponentTupleType<MeshRendererFlagsComponent> mRendererFlagsComponents;
        TComponentTupleType<MeshRendererComponent>      mRendererComponents;
        TComponentTupleType<MeshTextureComponent>       mMeshComponents;
        TVector<EntityCollection*>                      mEntities;
    };

    struct StandardTuple
    {
        using TupleType = TComponentSystemTuple<MeshRendererFlagsComponent, MeshRendererComponent, MeshStandardComponent>;

        TComponentTupleType<MeshRendererFlagsComponent> mRendererFlagsComponents;
        TComponentTupleType<MeshRendererComponent>      mRendererComponents;
        TComponentTupleType<MeshStandardComponent>      mMeshComponents;
        TVector<EntityCollection*>                      mEntities;
    };

    MeshSetupComponentSystem();
    ~MeshSetupComponentSystem();

    void SetGameRenderer(const GameRendererAtomicPtr& gameRenderer);
protected:
    bool OnInitialize() override;
    void OnBindTuples() override;
    void OnScheduleUpdates() override;

private:

    using SimpleMeshSetupCallback = TCallback<void, MeshRendererFlagsComponentData*, MeshRendererComponentData*, MeshSimpleComponentData*>;
    void UpdateSimpleMesh();
    void UpdateSimpleMeshEntity(MeshRendererFlagsComponentData* flags, MeshRendererComponentData* renderer, MeshSimpleComponentData* mesh);

    using TextureMeshSetupCallback = TCallback<void, MeshRendererFlagsComponentData*, MeshRendererComponentData*, MeshTextureComponentData*>;
    void UpdateTextureMesh();
    void UpdateTextureMeshEntity(MeshRendererFlagsComponentData* flags, MeshRendererComponentData* renderer, MeshTextureComponentData* mesh);

    using StandardMeshSetupCallback = TCallback<void, MeshRendererFlagsComponentData*, MeshRendererComponentData*, MeshStandardComponentData*>;
    void UpdateStandardMesh();
    void UpdateStandardMeshEntity(MeshRendererFlagsComponentData* flags, MeshRendererComponentData* renderer, MeshStandardComponentData* mesh);



    SimpleTuple mSimpleMeshTuple;
    TextureTuple mTextureMeshTuple;
    StandardTuple mStandardMeshTuple;

    GameRendererAtomicPtr mGameRenderer;
    bool mRegistered;

};

} // namespace lf
