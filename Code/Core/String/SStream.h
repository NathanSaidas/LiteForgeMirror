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

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/String/String.h"
#include "Core/Utility/StreamTypes.h"

namespace lf {

class Token;

class LF_CORE_API SStream
{
public:
    enum FillMode
    {
        // Content is submitted starting from the right to fit within a certain space.
        FM_RIGHT,
        // Content is submitted starting from the left to fit within a certain space.
        FM_LEFT
    };
    enum Options
    {
        // booleans are written as 'false' and 'true' as opposed to 0 and 1
        OPTION_BOOL_ALPHA = 1 << 0,
        // characters are written in their numerical value 'a' => '54'
        OPTION_CHAR_ALPHA = 1 << 1
    };
    struct State
    {
        UInt16   mPrecision;
        UInt16   mFillAmount;
        Options  mOptions;
        FillMode mFillMode;
        char     mFillChar;
    };

    SStream();
    SStream(const SStream& other);
    SStream(SStream&& other);
    ~SStream();

    SStream& operator=(const SStream& other);
    SStream& operator=(SStream&& other);

    SStream& operator<<(bool value);
    SStream& operator<<(Int8 value);
    SStream& operator<<(Int16 value);
    SStream& operator<<(Int32 value);
    SStream& operator<<(Int64 value);
    SStream& operator<<(UInt8 value);
    SStream& operator<<(UInt16 value);
    SStream& operator<<(UInt32 value);
    SStream& operator<<(UInt64 value);
    SStream& operator<<(Float32 value);
    SStream& operator<<(Float64 value);
    SStream& operator<<(const char* value);
    SStream& operator<<(const String& value);
    SStream& operator<<(const Token& value);

    SStream& operator<<(const StreamFillRight& fill);
    SStream& operator<<(const StreamFillLeft& fill);
    SStream& operator<<(const StreamFillChar& fill);
    SStream& operator<<(const StreamPrecision& precision);
    SStream& operator<<(const StreamBoolAlpha& option);
    SStream& operator<<(const StreamCharAlpha& option);

    // **********************************
    // Clears all content in the string buffer
    // @param {bool} resetOptions - Resets the internal options of the SStream to the defaults.
    // **********************************
    void Clear(bool resetOptions = true);
    // **********************************
    // Reserves a specified amount of memory. (Retains any content)
    // @param {SizeT} amount - Size in bytes to reserve.
    // **********************************
    void Reserve(SizeT amount);
    // **********************************
    // Pushes on a new state. (Preserving the value of the current state is optional)
    // Returns the state object. Its on the user to return the state via pop.
    // 
    // Example Usage:
    // auto state = ss.Push();
    // ...
    // ss.Pop(state);
    //
    // @param {bool} preserve - Whether or not the current state values are copied to the pushed state.
    // **********************************
    State Push(bool preserve = false);
    // **********************************
    // Pops the state restoring this.state with 'state'
    // @param {State} state - Values of the previous state.
    // **********************************
    void Pop(const State& state);

    LF_INLINE const String& Str() const { return mBuffer; }
    LF_INLINE const char* CStr() const { return mBuffer.CStr(); }
    LF_INLINE bool  Empty() const { return mBuffer.Empty(); }
    LF_INLINE SizeT Size() const { return mBuffer.Size(); }
    LF_INLINE SizeT Capacity() const { return mBuffer.Capacity(); }

private:
    // **********************************
    // Helper function runs through common content submission routines. (formatting)
    // @param {const char*} content - Pointer to character string buffer that has the content.
    // @param {SizeT} length - How many characters to copy from the buffer.
    // **********************************
    void WriteCommon(const char* content, SizeT length);

    // Buffer that contains all written data.
    String   mBuffer;
    // How much precision to use when converting numbers to string.
    // default: '5'
    UInt16   mPrecision;
    // How much of 'fill' character should be put in base don fill mode.
    // default: '0'
    UInt16   mFillAmount;
    // Active stream options
    // default: OPTION_BOOL_ALPHA
    Options  mOptions;
    // What direction content should be filled from.
    // default: FM_LEFT
    FillMode mFillMode;

    // What type of character should be used to fill.
    // default: ' '
    char     mFillChar;
    
};



} // namespace lf