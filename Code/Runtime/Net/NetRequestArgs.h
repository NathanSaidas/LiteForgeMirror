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

// todo: Get rid of this file...
#include "Core/Net/NetTypes.h"

namespace lf {
class String;

namespace NetRequestBodyFormat
{
    enum Value
    {
        RBF_TEXT,
        RBF_BINARY,

        MAX_VALUE,
        INVALID_ENUM = MAX_VALUE
    };
}

// **********************************
// This data structure hopes to aim at 
// defining strict constraints on what a 
// request can be and help translate from
// a programmable request to bytes.
// **********************************
class LF_RUNTIME_API NetRequestArgs
{
public:
    // The maximum length of a string describing the 'route'
    static const SizeT MAX_ROUTE_NAME = 80;
    // The maximum length of a string for the arguments present in the 'route'
    static const SizeT MAX_ROUTE_ARGS = 150;
    // The maximum length of bytes for the 'body'
    static const SizeT MAX_ROUTE_BODY = 3500;

    NetRequestArgs();

    // **********************************
    // Initialize the request args by route name.
    // @return Returns false if the NetRequestArgs fails to initialize (bad arguments)
    // **********************************
    bool Set(
        const String& routeName,
        const String& routeArgs,
        NetRequestBodyFormat::Value bodyFormat,
        const void* body,
        SizeT bodyLength);

    // **********************************
    // Initialize the request args by route index.
    // @return Returns false if the NetRequestArgs fails to initialize (bad arguments)
    // **********************************
    bool Set(
        RouteIndex routeIndex,
        const String& routeArgs,
        NetRequestBodyFormat::Value bodyFormat,
        const void* body,
        SizeT bodyLength);

    // **********************************
    // Clear all fields in the request args effectively setting it back to default state.
    // **********************************
    void Clear();
    // **********************************
    // Returns true if the NetRequestArgs holds no state
    // **********************************
    bool Empty() const;
    // **********************************
    // Returns true if the NetRequestArgs should use the route index instead of route name for 
    // for routing.
    // **********************************
    bool UseRouteIndex() const;
    // **********************************
    // Sets the buffer pointer that gets written to when the NetRequestArgs reads data.
    // **********************************
    void ReserveBody(void* body, SizeT bodyLength);

    String GetRouteName() const;
    RouteIndex GetRouteIndex() const;
    String GetRouteArgs() const;
    NetRequestBodyFormat::Value GetBodyFormat() const;
    const void* GetBody() const;
    SizeT GetBodyLength() const;

    // **********************************
    // Writes the NetRequestArgs out to bytes
    // @return Returns the number of bytes written
    // **********************************
    SizeT Write(ByteT* buffer, const SizeT bufferLength) const;
    // **********************************
    // Reads the NetRequestArgs out from bytes (if there is a body the NetRequestArgs must have it reserved before read)
    // @return Returns true if the request could successfully parse. (The request might be invalid, use Valid to check afterwards)
    // **********************************
    bool  Read(const ByteT* buffer, const SizeT bufferLength);
    // **********************************
    // Checks if a request contains valid data
    // **********************************
    bool  Valid() const;

private:
    // The route name (what is supposed to process the request)
    char                        mRouteName[MAX_ROUTE_NAME];
    // The route index (what is supposed to process the request) 
    // note: A valid route name or a valid route index is required. If both are valid the request is considered 'invalid'
    RouteIndex                  mRouteIndex;
    // Optional additional arguments to provide in the request. (Intention is for them to be short/fast to parse)
    char                        mRouteArgs[MAX_ROUTE_ARGS];
    // Format description of how to interpret the body data using a defined format.
    NetRequestBodyFormat::Value mBodyFormat;
    // Pointer to an external buffer of data. (The NetRequestArgs takes no ownership of the buffer and is not responsible for freeing.)
    // It's also understood that the buffer will remain valid for the duration of the NetRequestArgs object lifetime.
    const void*                 mBody;
    // The size of the buffer (either capacity or true size)
    SizeT                       mBodyLength;
};

}
