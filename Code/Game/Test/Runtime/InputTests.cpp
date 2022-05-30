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
#include "Core/Test/Test.h"
#include "Core/Input/InputBinding.h"
#include "Core/Input/InputMapping.h"
#include "Core/Input/InputEvents.h"
#include "Core/Input/KeyboardEvents.h"
#include "Core/Input/MouseEvents.h"
#include "Core/Utility/Time.h"

#include "Engine/Win32Input/Win32InputMgr.h"

namespace lf {

REGISTER_TEST(InputBinding_KeyboardEvent_Test, "Core.Input")
{
    InputMapping moveForward(Token("MoveForward"), Token("Game"));
    TEST(moveForward.Register(InputBindingData(InputEventType::BUTTON_DOWN, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::W)));
    TEST(moveForward.Register(InputBindingData(InputEventType::BUTTON_DOWN, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::UP, InputConfigBitfield({InputConfigFlags::SECONDARY}))));

    auto binding = MakeConvertibleAtomicPtr<InputBinding>();
    binding->InitializeAction(moveForward.GetScope());
    TEST(binding->CreateAction(moveForward.GetPrimary(InputDeviceType::KEYBOARD)));
    TEST(binding->CreateAction(moveForward.GetSecondary(InputDeviceType::KEYBOARD)));

    bool called = false;
    binding->OnEvent([&called](const InputEvent& event)
        {
            (event);
            called = true;
        });

    Float32 delta = ToSeconds(TimeTypes::Milliseconds(16.0f)).mValue;

    // Primary Input:
    KeyboardEvent event;
    event.mCode = InputCode::W;
    event.mLocalDeviceId = 0;
    InitBinaryInputState(event.mBinaryInputState, true, true, false);

    called = false;
    binding->ProcessEvent(event);
    TEST(called);

    called = false;
    binding->Update(delta);
    TEST(called);

    called = false;
    InitBinaryInputState(event.mBinaryInputState, false, false, true);
    binding->ProcessEvent(event);
    TEST(!called);

    called = false;
    binding->Update(delta);
    TEST(!called);

    // Secondary Input:
    event.mCode = InputCode::UP;
    event.mLocalDeviceId = 0;
    InitBinaryInputState(event.mBinaryInputState, true, true, false);

    called = false;
    binding->ProcessEvent(event);
    TEST(called);

    called = false;
    binding->Update(delta);
    TEST(called);

    called = false;
    InitBinaryInputState(event.mBinaryInputState, false, false, true);
    binding->ProcessEvent(event);
    TEST(!called);

    called = false;
    binding->Update(delta);
    TEST(!called);

    // Mix Input:
    event.mCode = InputCode::W;
    event.mLocalDeviceId = 0;
    InitBinaryInputState(event.mBinaryInputState, true, true, false);

    called = false;
    binding->ProcessEvent(event);
    TEST(called);

    called = false;
    binding->Update(delta);
    TEST(called);

    event.mCode = InputCode::UP;
    called = false;
    InitBinaryInputState(event.mBinaryInputState, false, false, true);
    binding->ProcessEvent(event);
    TEST(!called);

    called = false;
    binding->Update(delta);
    TEST(!called);

    // Mix Input:
    event.mCode = InputCode::UP;
    event.mLocalDeviceId = 0;
    InitBinaryInputState(event.mBinaryInputState, true, true, false);

    called = false;
    binding->ProcessEvent(event);
    TEST(called);

    called = false;
    binding->Update(delta);
    TEST(called);

    event.mCode = InputCode::W;
    called = false;
    InitBinaryInputState(event.mBinaryInputState, false, false, true);
    binding->ProcessEvent(event);
    TEST(!called);

    called = false;
    binding->Update(delta);
    TEST(!called);
}

REGISTER_TEST(InputBinding_KeyboardEventAxis_Test, "Core.Input")
{
    InputMapping moveForward(Token("MoveForward"), Token("Game"));
    TEST(moveForward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::W)));
    TEST(moveForward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::UP, InputConfigBitfield({ InputConfigFlags::SECONDARY }))));

    InputMapping moveBackward(Token("MoveBackward"), Token("Game"));
    TEST(moveBackward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::S)));
    TEST(moveBackward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::DOWN, InputConfigBitfield({ InputConfigFlags::SECONDARY }))));

    auto binding = MakeConvertibleAtomicPtr<InputBinding>();
    binding->InitializeAxis(moveForward.GetScope());

    BinaryAxisSettings settings;
    settings.mScaleType = BinaryAxisSettings::SCALE_LINEAR;
    settings.mSnapToDefault = true;
    settings.mPositiveIncrementDelta = 1.0f;
    settings.mPositiveDecrementDelta = 1.0f;
    settings.mNegativeIncrementDelta = 1.0f;
    settings.mNegativeDecrementDelta = 1.0f;

    TEST(binding->CreateBinaryAxis(moveForward.GetPrimary(InputDeviceType::KEYBOARD), moveBackward.GetPrimary(InputDeviceType::KEYBOARD), settings));

    bool called = false;
    Float32 value = 0.0f;
    binding->OnEvent([&called, &value](const InputEvent& event)
        {
            (event);
            called = true;
            TEST(event.mInputType == InputType::AXIS);
            value = event.mAxisInputValue.mCurrentValue;
        });

    Float32 delta = ToSeconds(TimeTypes::Milliseconds(16.0f)).mValue;

    // Primary Input:
    KeyboardEvent event;
    event.mCode = InputCode::W;
    event.mLocalDeviceId = 0;
    InitBinaryInputState(event.mBinaryInputState, true, true, false);

    binding->ProcessEvent(event);
    TEST(!called);

    binding->Update(delta);
    TEST(called);
    TEST(value > 0.0f);
}

REGISTER_TEST(InputBinding_MouseEvent_Test, "Core.Input")
{

}

REGISTER_TEST(InputBinding_MouseEventAxis_Test, "Core.Input")
{
    InputMapping moveForward(Token("MoveForward"), Token("Game"));
    TEST(moveForward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::W)));
    TEST(moveForward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::UP, InputConfigBitfield({ InputConfigFlags::SECONDARY }))));
    TEST(moveForward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::MOUSE, InputCode::MOUSE_BUTTON_LEFT)));
    
    
    InputMapping moveBackward(Token("MoveBackward"), Token("Game"));
    TEST(moveBackward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::S)));
    TEST(moveBackward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::DOWN, InputConfigBitfield({ InputConfigFlags::SECONDARY }))));
    TEST(moveBackward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::MOUSE, InputCode::MOUSE_BUTTON_RIGHT)));


    auto binding = MakeConvertibleAtomicPtr<InputBinding>();
    binding->InitializeAxis(moveForward.GetScope());

    BinaryAxisSettings settings;
    settings.mScaleType = BinaryAxisSettings::SCALE_LINEAR;
    settings.mSnapToDefault = true;
    settings.mPositiveIncrementDelta = 1.0f;
    settings.mPositiveDecrementDelta = 1.0f;
    settings.mNegativeIncrementDelta = 1.0f;
    settings.mNegativeDecrementDelta = 1.0f;

    TEST(binding->CreateBinaryAxis(moveForward.GetPrimary(InputDeviceType::MOUSE), moveBackward.GetPrimary(InputDeviceType::MOUSE), settings));

    bool called = false;
    Float32 value = 0.0f;
    binding->OnEvent([&called, &value](const InputEvent& event)
        {
            (event);
            called = true;
            TEST(event.mInputType == InputType::AXIS);
            value = event.mAxisInputValue.mCurrentValue;
        });

    Float32 delta = ToSeconds(TimeTypes::Milliseconds(16.0f)).mValue;

    MouseButtonEvent event;
    event.mCode = InputCode::MOUSE_BUTTON_LEFT;
    event.mLocalDeviceId = 1;
    InitBinaryInputState(event.mBinaryInputState, true, true, false);
    event.mCursorX = 50;
    event.mCursorY = 100;
    event.mWindowData.mCursorX = 0;
    event.mWindowData.mCursorY = 0;

    binding->ProcessEvent(event);
    TEST(!called);

    binding->Update(delta);
    TEST(called);
    TEST(value > 0.0f);
}

REGISTER_TEST(InputBinding_GameExamples)
{
    // Imagine a game where you got a player character in the world.
    // They can move (left-right) (forward-backward)
    // They can jump
    // They can crouch
    // They can dash
    // They can activate mainhand (attack)
    // They can activate offhand (defend or attack)
    // They can use items (4)
    // They can use spells (4)
    // They can look around.

    BinaryAxisSettings settings;
    settings.mScaleType = BinaryAxisSettings::SCALE_LINEAR;
    settings.mSnapToDefault = true;
    settings.mPositiveIncrementDelta = 1.0f;
    settings.mPositiveDecrementDelta = 1.0f;
    settings.mNegativeIncrementDelta = 1.0f;
    settings.mNegativeDecrementDelta = 1.0f;

    Token GAME_FILTER("Game");

    InputMapping moveRight(Token("MoveRight"), GAME_FILTER);
    moveRight.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::D));
    moveRight.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::RIGHT, InputConfigBitfield({ InputConfigFlags::SECONDARY })));
    InputMapping moveLeft(Token("MoveLeft"), GAME_FILTER);
    moveLeft.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::A));
    moveLeft.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::LEFT, InputConfigBitfield({ InputConfigFlags::SECONDARY })));
    auto moveHorizontalBinding = MakeConvertibleAtomicPtr<InputBinding>();
    moveHorizontalBinding->InitializeAxis(GAME_FILTER);
    moveHorizontalBinding->CreateBinaryAxis(moveRight.GetPrimary(InputDeviceType::KEYBOARD), moveLeft.GetPrimary(InputDeviceType::KEYBOARD), settings);
    moveHorizontalBinding->CreateBinaryAxis(moveRight.GetSecondary(InputDeviceType::KEYBOARD), moveLeft.GetSecondary(InputDeviceType::KEYBOARD), settings);

    InputMapping moveForward(Token("MoveForward"), GAME_FILTER);
    moveForward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::W));
    moveForward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::UP, InputConfigBitfield({ InputConfigFlags::SECONDARY })));
    InputMapping moveBackward(Token("MoveBackward"), GAME_FILTER);
    moveBackward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::S));
    moveBackward.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::DOWN, InputConfigBitfield({ InputConfigFlags::SECONDARY })));
    auto moveVerticalBinding = MakeConvertibleAtomicPtr<InputBinding>();
    moveVerticalBinding->InitializeAxis(GAME_FILTER);
    moveVerticalBinding->CreateBinaryAxis(moveForward.GetPrimary(InputDeviceType::KEYBOARD), moveBackward.GetPrimary(InputDeviceType::KEYBOARD), settings);
    moveVerticalBinding->CreateBinaryAxis(moveForward.GetSecondary(InputDeviceType::KEYBOARD), moveBackward.GetSecondary(InputDeviceType::KEYBOARD), settings);

    InputMapping jump(Token("Jump"), GAME_FILTER);
    jump.Register(InputBindingData(InputEventType::BUTTON_PRESSED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::SPACE));
    auto jumpBinding = MakeConvertibleAtomicPtr<InputBinding>();
    jumpBinding->InitializeAction(GAME_FILTER);
    jumpBinding->CreateAction(jump.GetPrimary(InputDeviceType::KEYBOARD));

    InputMapping crouch(Token("CrouchToggle"), GAME_FILTER);
    crouch.Register(InputBindingData(InputEventType::BUTTON_PRESSED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::C));
    auto crouchBinding = MakeConvertibleAtomicPtr<InputBinding>();
    crouchBinding->InitializeAction(GAME_FILTER);
    crouchBinding->CreateAction(crouch.GetPrimary(InputDeviceType::KEYBOARD));

    InputMapping dash(Token("Dash"), GAME_FILTER);
    dash.Register(InputBindingData(InputEventType::BUTTON_PRESSED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::SHIFT));
    auto dashBinding = MakeConvertibleAtomicPtr<InputBinding>();
    dashBinding->InitializeAction(GAME_FILTER);
    dashBinding->CreateAction(dash.GetPrimary(InputDeviceType::KEYBOARD));

    InputMapping activateMainHand(Token("ActivateMainHand"), GAME_FILTER);
    activateMainHand.Register(InputBindingData(InputEventType::BUTTON_PRESSED, InputType::BINARY, InputDeviceType::MOUSE, InputCode::MOUSE_BUTTON_LEFT));
    auto activateMainHandBinding = MakeConvertibleAtomicPtr<InputBinding>();
    activateMainHandBinding->InitializeAction(GAME_FILTER);
    activateMainHandBinding->CreateAction(activateMainHand.GetPrimary(InputDeviceType::MOUSE));

    InputMapping activateOffHand(Token("ActivateOffHand"), GAME_FILTER);
    activateOffHand.Register(InputBindingData(InputEventType::BUTTON_PRESSED, InputType::BINARY, InputDeviceType::MOUSE, InputCode::MOUSE_BUTTON_RIGHT));
    auto activateOffHandBinding = MakeConvertibleAtomicPtr<InputBinding>();
    activateOffHandBinding->InitializeAction(GAME_FILTER);
    activateOffHandBinding->CreateAction(activateOffHand.GetPrimary(InputDeviceType::MOUSE));

    InputCode useItemCodes[] = { InputCode::ALPHA_1, InputCode::ALPHA_2, InputCode::ALPHA_3, InputCode::ALPHA_4 };
    TStackVector<InputMapping, 4> useItem;
    TStackVector<TAtomicStrongPointer<InputBinding>, 4> useItemBinding;


    for (SizeT i = 0; i < LF_ARRAY_SIZE(useItemCodes); ++i)
    {
        char inputName[24];
        sprintf_s(inputName, "UseItem%d", static_cast<int>(i));
        useItem.push_back(InputMapping(inputName, GAME_FILTER));
        InputMapping& mapping = useItem.back();
        mapping.Register(InputBindingData(InputEventType::BUTTON_PRESSED, InputType::BINARY, InputDeviceType::KEYBOARD, useItemCodes[i]));
        auto binding = MakeConvertibleAtomicPtr<InputBinding>();
        binding->InitializeAction(GAME_FILTER);
        binding->CreateAction(mapping.GetPrimary(InputDeviceType::KEYBOARD));
        useItemBinding.push_back(binding);
    }

    InputCode useSpellCodes[] = { InputCode::ALPHA_1, InputCode::ALPHA_2, InputCode::ALPHA_3, InputCode::ALPHA_4 };
    TStackVector<InputMapping, 4> useSpell;
    TStackVector<TAtomicStrongPointer<InputBinding>, 4> useSpellBinding;
    for (SizeT i = 0; i < LF_ARRAY_SIZE(useSpellCodes); ++i)
    {
        char inputName[24];
        sprintf_s(inputName, "UseSpell%d", static_cast<int>(i));
        useSpell.push_back(InputMapping(inputName, GAME_FILTER));
        InputMapping& mapping = useSpell.back();
        mapping.Register(InputBindingData(InputEventType::BUTTON_PRESSED, InputType::BINARY, InputDeviceType::KEYBOARD, useItemCodes[i]));
        auto binding = MakeConvertibleAtomicPtr<InputBinding>();
        binding->InitializeAction(GAME_FILTER);
        binding->CreateAction(mapping.GetPrimary(InputDeviceType::KEYBOARD));
        useSpellBinding.push_back(binding);
    }

    InputMapping lookX(Token("LookX"), GAME_FILTER);
    lookX.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::CURSOR, InputDeviceType::MOUSE, InputCode::CURSOR_MOUSE_X));
    auto lookBinding = MakeConvertibleAtomicPtr<InputBinding>();
    lookBinding->InitializeCursor(GAME_FILTER);
    lookBinding->CreateCursor(lookX.GetPrimary(InputDeviceType::MOUSE));

    InputMapping lookY(Token("LookY"), GAME_FILTER);
    lookY.Register(InputBindingData(InputEventType::DATA_CHANGED, InputType::CURSOR, InputDeviceType::MOUSE, InputCode::CURSOR_MOUSE_Y));
    lookBinding->CreateCursor(lookY.GetPrimary(InputDeviceType::MOUSE));

    Win32InputMgr inputMgr;
    inputMgr.RegisterBinding(Token("MoveHorizontal"), GAME_FILTER, moveHorizontalBinding);
    inputMgr.RegisterBinding(Token("MoveVertical"), GAME_FILTER, moveVerticalBinding);
    inputMgr.RegisterBinding(jump.GetName(), jump.GetScope(), jumpBinding);
    inputMgr.RegisterBinding(dash.GetName(), dash.GetScope(), dashBinding);
    inputMgr.RegisterBinding(activateMainHand.GetName(), activateMainHand.GetScope(), activateMainHandBinding);
    inputMgr.RegisterBinding(activateOffHand.GetName(), activateOffHand.GetScope(), activateOffHandBinding);
    for (SizeT i = 0; i < useItem.size(); ++i)
    {
        inputMgr.RegisterBinding(useItem[i].GetName(), useItem[i].GetScope(), useItemBinding[i]);
    }
    for (SizeT i = 0; i < useSpell.size(); ++i)
    {
        inputMgr.RegisterBinding(useSpell[i].GetName(), useSpell[i].GetScope(), useSpellBinding[i]);
    }
    inputMgr.RegisterBinding(Token("Look"), GAME_FILTER, lookBinding);

    inputMgr.GetInputBinding(Token("MoveHorizontal"), GAME_FILTER)->OnEvent(
        [](const InputEvent& event)
        {
            (event);
            // TODO: BLAH
        });

    // inputMgr.Initialize(defaultFilter)
    // 
    //
    // inputMgr.SendEvent(keyboard);
    // inputMgr.SendEvent(mouseButton)
    // inputMgr.SendEvent(mouseMove);
    //
    // inputMgr.PushFilter(name, additive|exclusive)
    // inputMgr.PopFilter()
    //
    // inputMgr.GetDevice(id)
    // inputMgr.GetKeyboard()
    // inputMgr.GetMouse()
    // 
}

} // namespace lf