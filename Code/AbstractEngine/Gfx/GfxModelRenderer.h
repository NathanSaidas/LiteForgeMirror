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
#include "Core/Reflection/Object.h"
#include "Runtime/Reflection/ReflectionTypes.h"


namespace lf {
class GfxCommandContext;
class GfxDevice;
class GfxRenderer;

// **********************************
// Represents a physical/graphical model in game to be rendered. This is not intended to be 'particle effect/visual effect/post effect/light' but a very simple object.
// 
// - A model renderer is created from the 'GfxRenderer'. GfxRenderer->CreateModelRenderer< Type >();
// - A model renderer has virtual methods and conceptual methods.
// 
// Conceptually a model renderer has these main functions. (Sys Call = A call made from the internal system, User Call = A call made from the user at any point)
// - [Sys Call] SetupResource( Device, CommandContext ) -- Called during frame rendering to determine just how many resources the descriptor heap will need to allocate for.
// - [Sys Call] SetupFrame( Device, CommandContext ) -- Called during frame rendering to determine what is rendered.
// - [Sys Call] RenderFrame( Device, CommandContext ) -- Called during frame rendering to submit draw calls 
// - [User Call]SetData_xxx -- A generic function to set any type of data.
// 
// Guidelines:
// - We should try to maintain a static amount of resources (the memory used by the resources can be dynamic)
// 
// Model Renderer Concepts & Memory
// - ProceduralModelRenderer - SetData( vertices, incides ); [user_mem =|copied|=> temp_mem =|copied|=> upload_buffer =|copied|=> final_buffer;] (4 buffers, 3 copies)
// - StaticModelRenderer - SetData( Asset ) [runtime_asset_mem =|copied|=> =|upload_buffer|=> final_buffer]; (3 buffers, 2 copies) (could be optimized to 1 copy, 2 buffers)
// 
// **********************************
class LF_ABSTRACT_ENGINE_API GfxModelRenderer : public Object
{
    DECLARE_CLASS(GfxModelRenderer, Object);
public:
    GfxModelRenderer();

    // **********************************
    // Called during creation of the model renderer. Sets the owning renderer.
    // **********************************
    void SetRenderer(GfxRenderer* renderer);

    virtual void SetupResource(GfxDevice& device, GfxCommandContext& context);
    // **********************************
    // Called upon request usually to acquire resources.
    // 
    // @note - GameRenderer will invoke this at the BeginFrame on the Main Thread in a single threaded fashion
    // 
    // !DEPRECATED!
    // **********************************
    virtual void OnUpdate(GfxDevice& device);
    // **********************************
    // Called each frame to submit to the graphics command list.
    // **********************************
    virtual void OnRender(GfxDevice& device, GfxCommandContext& context);

    void SetTransparent(bool value) { mTransparent = value; }
    bool IsTransparent() const { return mTransparent; }
    void SetVisible(bool value) { mVisible = value; }
    bool IsVisible() const { return mVisible; }
protected:
    GfxRenderer& Renderer()
    {
        CriticalAssert(mRenderer != nullptr);
        return *mRenderer;
    }

private:
    GfxRenderer* mRenderer; // Renderer owns its models therefore this pointer is always valid as long as the model is valid.

    bool mTransparent;
    bool mVisible;
};

} // namespace lf