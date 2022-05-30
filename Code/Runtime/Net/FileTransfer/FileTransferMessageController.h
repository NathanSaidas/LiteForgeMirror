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
#include "Core/Common/Enum.h"
#include "Core/Utility/StdMap.h"
#include "Core/Utility/UniqueNumber.h"
#include "Core/Utility/SmartCallback.h"
#include "Core/Platform/RWSpinLock.h"
#include "Runtime/Net/Controllers/NetMessageController.h"
#include "Runtime/Net/FileTransfer/FileResourceTypes.h"
#include "Core/Net/NetTypes.h"

namespace lf {

DECLARE_ENUM(FileTransferStatus,
    FTS_CONNECTING,
    FTS_DOWNLOADING,
    FTS_COMPLETE,
    FTS_FAILED);

DECLARE_PTR(FileResourceLocator);
DECLARE_STRUCT_ATOMIC_PTR(FileResourceHandle);
struct DownloadRequest;
struct DownloadResponse;

// ** Client-side object
class FileTransferRequest
{
    friend class FileTransferMessageController;
public:
    // (status, request)
    using DoneCallback = TCallback<void, bool, FileTransferRequest*>;

    FileTransferRequest(const FileResourceHandleAtomicPtr& handle);
    // ** Sends a DMT_DOWNLOAD_COMPLETE_REQUEST to the 'server' and closes the connection handle
    void Cancel();
    // ** Returns the name of the resource for the request
    String GetResourceName() const;
    // ** Returns the id of the request. (Only valid after FTS_DOWNLOADING)
    UInt32 GetRequestID() const;
    // ** Returns [0...1] progress on the resource. 
    Float32 GetProgress() const;
    // ** Returns the status of the request.
    FileTransferStatus::Value GetStatus() const;
    // ** Returns a pointer to the bytes received. (Only valid after FTS_COMPLETE)
    const ByteT* Bytes() const;
    // ** Returns the number of bytes received. (Only valid after FTS_COMPLETE)
    SizeT Size() const;
private:
    FileResourceHandleAtomicPtr mResourceHandle;

};
DECLARE_ATOMIC_PTR(FileTransferRequest);

class LF_RUNTIME_API FileTransferMessageController : public NetMessageController
{
    using Super = NetMessageController;
public:
    FileTransferMessageController();
    virtual ~FileTransferMessageController();

    void SetResourceLocator(const FileResourceLocatorPtr& resourceLocator);


    // ** Initiates the download of a resource.
    FileTransferRequestAtomicPtr DownloadFile(const String& download, FileTransferRequest::DoneCallback onDone);

    void OnInitialize(NetDriver* driver) override;
    void OnShutdown() override;
    void OnConnect(NetConnection* connection) override;
    void OnDisconnect(NetConnection* connection) override;
    void OnMessageData(NetMessageDataArgs& args) override;
    void OnMessageDataError(NetMessageDataErrorArgs& args) override;
private:
    using Handle = FileResourceHandle*;
    using HandlePtr = FileResourceHandleAtomicPtr;
    using RequestPtr = FileTransferRequestAtomicPtr;

    DECLARE_STRUCT_PTR(ServerRequest);
    DECLARE_STRUCT_PTR(ServerConnection);
    struct ServerRequest
    {
        enum State
        {
            QueryResourceInfo,

        };

        void SetState(State value);
        State GetState() const;


        volatile Atomic32        mState;
        UInt32                   mRequestID;
        UInt32                   mClientID;
        NetConnectionAtomicWPtr  mConnection;
        FileResourceInfo         mResourceInfo;
    };

    struct ServerConnection
    {
        NetConnectionAtomicWPtr  mConnection;
        UniqueNumber<UInt32, 16> mRequestIDGen;

        TVector<ServerRequestPtr> mRequests;
        RWSpinLock               mRequestsLock;
    };

    SizeT WriteHeader(ByteT*& cursor, SizeT& inOutSize, DownloadMessageType::Value messageType);
    bool  ReadHeader(const ByteT*& cursor, SizeT& inOutSize, DownloadMessageType::Value& messageType);
    void BeginDownload(Handle handle);
    void OnDownloadRequestSent(Handle handle);
    void OnDownloadRequestFailed(Handle handle);
    void OnDownloadRequest(NetConnection* connection, const DownloadRequest& downloadRequest);
    
    void OnDownloadResponse(const DownloadResponse& response);
    // void OnDownloadResponseSent(ServerRequest* request);
    // void OnDownloadResponseFailed(ServerRequest* request);

    // Outbound:
    // ** Allocates a file resource handle, optional connection param to allocate on a connection.
    FileResourceHandleAtomicPtr AllocateHandle(NetConnection* connection = nullptr);
    void ReleaseHandle(FileResourceHandle* handle);

    UniqueNumber<UInt32, 10>                  mOutboundRequestIDGen;
    // ** <Local ID, Handle>
    TMap<UInt32, FileResourceHandleAtomicPtr> mLocalOutboundRequests;
    RWSpinLock                                mLocalOutboundRequestsLock;
         
    // ** <Request ID, Handle>
    TMap<UInt32, FileResourceHandleAtomicPtr> mOutboundRequests;
    RWSpinLock                                mOutboundRequestsLock;

    // todo: Update Handles Maybe

    // Inbound:
    ServerConnection* AllocateConnection(NetConnection* connection);
    ServerRequest* CreateRequest(ServerConnection* connection, const DownloadRequest& downloadRequest);
    ServerRequest* GetRequest(ServerConnection* connection, UInt32 requestID);
    void DeleteRequest(ServerConnection* connection, UInt32 requestID);

    void UpdateRequest(ServerRequest* request);
    void UpdateRequestQueryInfo(ServerRequest* request);



    TMap<SessionID, ServerConnectionPtr>      mConnections;
    RWSpinLock                                mConnectionLock;

    TVector<ServerRequestPtr>                  mRequests;
    RWSpinLock                                mRequestsLock;

    NetDriver* mDriver;
    FileResourceLocatorPtr mResourceLocator;
};

}