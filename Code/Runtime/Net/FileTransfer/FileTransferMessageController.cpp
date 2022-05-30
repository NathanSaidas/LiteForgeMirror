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
#include "Runtime/PCH.h"
#include "FileTransferMessageController.h"
#include "Core/IO/BinaryStream.h"
#include "Core/Utility/Log.h"
#include "Runtime/Net/NetDriver.h"
#include "Runtime/Net/NetConnection.h"
#include "Runtime/Net/FileTransfer/FileTransferTypes.h"
#include "Runtime/Net/FileTransfer/FileResourceLocator.h"

namespace lf {

template<typename T>
SizeT WriteAllBytes(ByteT* bytes, SizeT numBytes, T& data)
{
    MemoryBuffer buffer(bytes, numBytes);
    BinaryStream bs(Stream::MEMORY, &buffer, Stream::SM_WRITE);
    if (bs.BeginObject("f", "t"))
    {
        bs << data;
        bs.EndObject();
    }
    return buffer.GetSize();
}

template<typename T>
bool ReadAllBytes(const ByteT* bytes, SizeT numBytes, T& data)
{
    MemoryBuffer buffer(const_cast<ByteT*>(bytes), numBytes);
    Assert(buffer.Allocate(numBytes, 1));
    BinaryStream bs(Stream::MEMORY, &buffer, Stream::SM_READ);
    if (bs.BeginObject("f", "t"))
    {
        bs << data;
        bs.EndObject();
        return true;
    }
    return false;
}

FileTransferRequest::FileTransferRequest(const FileResourceHandleAtomicPtr& handle)
: mResourceHandle(handle)
{}
void FileTransferRequest::Cancel()
{
    
}
String FileTransferRequest::GetResourceName() const
{
    return mResourceHandle->mInfo.mName;
}
UInt32 FileTransferRequest::GetRequestID() const
{
    return mResourceHandle->mRequestID;
}
Float32 FileTransferRequest::GetProgress() const
{
    SizeT written = mResourceHandle->mBuffer.GetSize();
    SizeT total = mResourceHandle->mInfo.mSize;
    return static_cast<Float32>(static_cast<Float64>(written) / static_cast<Float64>(total));
}
FileTransferStatus::Value FileTransferRequest::GetStatus() const
{
    if (Invalid(mResourceHandle->mRequestID))
    {
        return FileTransferStatus::FTS_CONNECTING;
    }
    if (mResourceHandle->mBuffer.GetSize() == mResourceHandle->mInfo.mSize)
    {
        return FileTransferStatus::FTS_COMPLETE;
    }
    if (mResourceHandle->mResponseStatus != DownloadResponseStatus::DRS_SUCCESS)
    {
        return FileTransferStatus::FTS_FAILED;
    }
    return FileTransferStatus::FTS_DOWNLOADING;
}
const ByteT* FileTransferRequest::Bytes() const
{
    return GetStatus() == FileTransferStatus::FTS_COMPLETE ? static_cast<const ByteT*>(mResourceHandle->mBuffer.GetData()) : nullptr;
}
SizeT FileTransferRequest::Size() const
{
    return GetStatus() == FileTransferStatus::FTS_COMPLETE ? mResourceHandle->mBuffer.GetSize() : 0;
}

FileTransferMessageController::FileTransferMessageController()
: Super()
, mLocalOutboundRequests()
, mLocalOutboundRequestsLock()
, mOutboundRequests()
, mOutboundRequestsLock()
, mDriver(nullptr)
{

}
FileTransferMessageController::~FileTransferMessageController()
{

}

void FileTransferMessageController::SetResourceLocator(const FileResourceLocatorPtr& resourceLocator)
{
    mResourceLocator = resourceLocator;
}

FileTransferRequestAtomicPtr FileTransferMessageController::DownloadFile(const String& download, FileTransferRequest::DoneCallback onDone)
{
    FileResourceHandleAtomicPtr handle = AllocateHandle();
    handle->mInfo.mName = download;
    FileTransferRequestAtomicPtr request(LFNew<FileTransferRequest>(handle));

    if (download.Empty())
    {
        onDone.Invoke(false, request);
        ReleaseHandle(handle);
        return request;
    }
    // todo: Fire off DownloadRequest message
    // BeginDownload(handle);

    return request;
}

void FileTransferMessageController::OnInitialize(NetDriver* driver)
{
    Assert(!mDriver);
    mDriver = driver;
}
void FileTransferMessageController::OnShutdown()
{
    mDriver = nullptr;
}
void FileTransferMessageController::OnConnect(NetConnection* connection)
{
    // WriteLock ( Connections ) -> AllocateConnection

    (connection);
}
void FileTransferMessageController::OnDisconnect(NetConnection* connection)
{
    // WriteLock ( Connections ) -> DeleteConnection
    (connection);
}
void FileTransferMessageController::OnMessageData(NetMessageDataArgs& args) 
{
    const ByteT* cursor = args.mAppData;
    SizeT        cursorSize = args.mAppDataSize;

    DownloadMessageType::Value messageType;
    if (!ReadHeader(cursor, cursorSize, messageType))
    {
        return;
    }

    switch (messageType)
    {
        case DownloadMessageType::DMT_DOWNLOAD_REQUEST: 
        {
            DownloadRequest requestData;
            if (ReadAllBytes(cursor, cursorSize, requestData))
            {
                OnDownloadRequest(args.mConnection, requestData);
            }
            // server-side only
        } break;
        case DownloadMessageType::DMT_DOWNLOAD_RESPONSE: 
        {
            // client-side only
            DownloadResponse responseData;
            if (ReadAllBytes(cursor, cursorSize, responseData))
            {
                OnDownloadResponse(responseData);
            }

        } break;
        case DownloadMessageType::DMT_DOWNLOAD_FETCH_REQUEST: break;
        case DownloadMessageType::DMT_DOWNLOAD_FETCH_FRAGMENT_REQUEST: break;
        case DownloadMessageType::DMT_DOWNLOAD_FETCH_STOP_REQUEST: break;
        case DownloadMessageType::DMT_DOWNLOAD_COMPLETE_REQUEST: break;
        case DownloadMessageType::DMT_DOWNLOAD_FETCH_COMPLETE_RESPONSE: break;
        case DownloadMessageType::DMT_DOWNLOAD_FETCH_DATA_RESPONSE: break;
        case DownloadMessageType::DMT_DOWNLOAD_FETCH_STOPPED_RESPONSE: break;
        default:
            break;
    }

    // ReadLock ( Connections ) -> GetConnection
    //  
    // if DownloadRequest
    //      WriteLock ( Connection->Requests ) -> AllocateRequest
    // if DownloadCompleteRequest
    //      WriteLock ( Connection->Requests ) -> DeleteRequest
    //
    // else 
    //      ReadLock ( Connection->Requests ) -> GetRequest()

    (args);

    // OnRequest:
    // 

    // mDriver->Send(NetDriver::MESSAGE_GENERIC, NetDriver::Options(), bytes, numBytes, connection, callbackX, callbackY);

}
void FileTransferMessageController::OnMessageDataError(NetMessageDataErrorArgs& args)
{
    (args);
}

void FileTransferMessageController::ServerRequest::SetState(State value)
{
    AtomicStore(&mState, value);
}
FileTransferMessageController::ServerRequest::State FileTransferMessageController::ServerRequest::GetState() const
{
    return static_cast<State>(AtomicLoad(&mState));
}

SizeT FileTransferMessageController::WriteHeader(ByteT*& cursor, SizeT& inOutSize, DownloadMessageType::Value messageType)
{
    CriticalAssert(inOutSize > 1);
    cursor[0] = static_cast<ByteT>(messageType);
    ++cursor; --inOutSize;
    return 1;
}

bool FileTransferMessageController::ReadHeader(const ByteT*& cursor, SizeT& inOutSize, DownloadMessageType::Value& messageType)
{
    if (inOutSize < 1)
    {
        return false;
    }

    messageType = static_cast<DownloadMessageType::Value>(cursor[0]);
    ++cursor; --inOutSize;
    return true;
}

void FileTransferMessageController::BeginDownload(Handle handle)
{
    if (handle->GetState() != FileResourceHandle::Initialization)
    {
        handle->SetState(FileResourceHandle::Failed);
        ReportBugMsg("BeginDownload should only be called on a FileResourceHandle that is initializing.");
        return;
    }

    if (mDriver->IsServer())
    {
        // todo: We might support connections later on, eg Server downloads file from client...
        ReportBugMsg("Servers cannot make requests without a connection.");
        return;
    }

    // Initialize request data
    DownloadRequest requestData;
    requestData.mRequestID = handle->mLocalRequstID;
    requestData.mResourceIdentifier = handle->mInfo.mName;
    requestData.mVersion = 0; // todo: unused

    // Serialize request data into bytes
    ByteT staticMemory[512] = { 0 };
    ByteT* cursor = staticMemory;
    SizeT cursorSize = sizeof(staticMemory);
    SizeT headerSize = WriteHeader(cursor, cursorSize, DownloadMessageType::DMT_DOWNLOAD_REQUEST);
    SizeT bodySize = WriteAllBytes(cursor, cursorSize, requestData);

    HandlePtr pinnedHandle(GetAtomicPointer(handle));
    NetDriver::Options options = NetDriver::OPTION_RELIABLE | NetDriver::OPTION_ENCRYPT | NetDriver::OPTION_SIGNED;
    if (!mDriver->Send(
            NetDriver::MESSAGE_FILE_TRANSFER, 
            options, 
            staticMemory, 
            headerSize + bodySize, 
            NetDriver::OnSendSuccess::Make([this, pinnedHandle]() { OnDownloadRequestSent(pinnedHandle); }),
            NetDriver::OnSendFailed::Make([this, pinnedHandle]() { OnDownloadRequestFailed(pinnedHandle); })
        )
    )
    {
        // The driver Send should only fail if it's not in the correct state.
        gNetLog.Error(LogMessage("NetDriver failed to send DownloadRequest. ResourceName=") << handle->mInfo.mName);
        handle->SetState(FileResourceHandle::Failed);
        return;
    }
    handle->SetState(FileResourceHandle::RequestDownload);
    ++handle->mActiveNetworkCalls;
}

void FileTransferMessageController::OnDownloadRequestSent(Handle handle)
{
    // We've received the ack.. now we wait for the response...
    --handle->mActiveNetworkCalls;
    if (handle->GetState() == FileResourceHandle::Downloading)
    {
        // If packet's we're received out of order.
        gNetLog.Warning(LogMessage("Receiving a DownloadRequest ack after DownloadResponse possible packet loss."));
    }
    else if (handle->GetState() == FileResourceHandle::Failed)
    {
        // do nothing since we failed some network/internal operation
    }
    else
    {
        // expected behavior:
    }

}

void FileTransferMessageController::OnDownloadRequestFailed(Handle handle)
{
    // We've haven't received an ack... but if we did get a response we can advance.
    --handle->mActiveNetworkCalls;
}

void FileTransferMessageController::OnDownloadRequest(NetConnection* connection, const DownloadRequest& downloadRequest)
{
    ServerConnection* serverConnection = AllocateConnection(connection);
    ServerRequest* request = CreateRequest(serverConnection, downloadRequest);
    request->mResourceInfo.mName = downloadRequest.mResourceIdentifier;
    request->mClientID = downloadRequest.mRequestID;
    request->SetState(ServerRequest::QueryResourceInfo);
}

void FileTransferMessageController::OnDownloadResponse(const DownloadResponse& response)
{
    // handle = FindHandle(response.mRequestID);
    HandlePtr handle;
    // Remove handle from 'local handle' map...
    {
        ScopeRWSpinLockWrite lock(mLocalOutboundRequestsLock);
        auto iter = mLocalOutboundRequests.find(response.mRequestID);
        if (iter == mLocalOutboundRequests.end())
        {
            // Duplicate packet?
            ReportBugMsg("Invalid DownloadResponse");
            return;
        }
        handle = iter->second;
        mLocalOutboundRequests.erase(iter);
    }
    handle->mRequestID = response.mResourceHandle;
    
    // Register with 'handle map'
    {
        ScopeRWSpinLockWrite lock(mOutboundRequestsLock);
        mOutboundRequests.insert(std::make_pair(response.mResourceHandle, handle));
    }
    

    // Update the handle with the new details, then begin file transfer ops.
    handle->mInfo.mChunkCount = response.mChunkCount;
    handle->mInfo.mFragmentCount = response.mFragmentCount;
    memcpy(handle->mInfo.mHash, response.mHash.mBytes, sizeof(response.mHash));
    handle->mInfo.mSize = static_cast<SizeT>(response.mResourceSize);
    handle->mResponseStatus = response.mStatus;
    if (response.mStatus != DownloadResponseStatus::DRS_SUCCESS)
    {
        handle->SetState(FileResourceHandle::Failed);
        return;
    }
    handle->SetState(FileResourceHandle::Downloading);
}

FileResourceHandleAtomicPtr FileTransferMessageController::AllocateHandle(NetConnection* connection)
{
    FileResourceHandleAtomicPtr handle = MakeConvertibleAtomicPtr<FileResourceHandle>();

    if (connection)
    {
        // todo: Allocate from the 'connection'
    }
    else
    {

        UInt32 id = mOutboundRequestIDGen.Allocate();
        handle->mLocalRequstID = id;
        ScopeRWSpinLockWrite lock(mLocalOutboundRequestsLock);
        mLocalOutboundRequests[id] = handle;
    }
    return handle;
}

void FileTransferMessageController::ReleaseHandle(FileResourceHandle* handle)
{
    NetConnectionAtomicPtr connection = handle->mConnection;
    if (connection)
    {
        // todo:
    }
    else
    {
        if (Valid(handle->mLocalRequstID))
        {
            ScopeRWSpinLockWrite lock(mLocalOutboundRequestsLock);
            Assert(mLocalOutboundRequests.erase(handle->mLocalRequstID) == 1);
            handle->mLocalRequstID = INVALID32;
        }
        else if(Valid(handle->mRequestID))
        {
            ScopeRWSpinLockWrite lock(mOutboundRequestsLock);
            Assert(mOutboundRequests.erase(handle->mLocalRequstID) == 1);
            handle->mRequestID = INVALID32;
        }
    }
}

FileTransferMessageController::ServerConnection* FileTransferMessageController::AllocateConnection(NetConnection* connection)
{
    ScopeRWSpinLockWrite lock(mConnectionLock);
    auto iter = mConnections.find(connection->GetConnectionID());
    if (iter != mConnections.end())
    {
        return iter->second;
    }

    ServerConnectionPtr serverConnection(LFNew<ServerConnection>());
    serverConnection->mConnection = GetAtomicPointer(connection);

    std::pair<SessionID, ServerConnectionPtr> pair;
    mConnections.insert(std::make_pair(connection->GetConnectionID(), serverConnection));
    return serverConnection;
}
FileTransferMessageController::ServerRequest* FileTransferMessageController::CreateRequest(ServerConnection* connection, const DownloadRequest& downloadRequest)
{
    (downloadRequest); // todo:

    UInt32 id = connection->mRequestIDGen.Allocate(); // todo: In theory we could use the id as index.. then theres no need to search!

    ServerRequestPtr request(LFNew<ServerRequest>());
    request->mRequestID = id;
    request->mConnection = connection->mConnection;
    {
        ScopeRWSpinLockWrite lock(connection->mRequestsLock);
        connection->mRequests.push_back(request);
    }
    {
        ScopeRWSpinLockWrite lock(mRequestsLock);
        mRequests.push_back(request);
    }
    return request;
}
FileTransferMessageController::ServerRequest* FileTransferMessageController::GetRequest(ServerConnection* connection, UInt32 requestID)
{
    ScopeRWSpinLockRead lock(connection->mRequestsLock);
    for (ServerRequest* request : connection->mRequests)
    {
        if (request->mRequestID == requestID)
        {
            return request;
        }
    }
    return nullptr;
}
void FileTransferMessageController::DeleteRequest(ServerConnection* connection, UInt32 requestID)
{
    ServerRequestPtr request;
    {
        ScopeRWSpinLockWrite lock(connection->mRequestsLock);
        for (auto it = connection->mRequests.begin(); it != connection->mRequests.end(); ++it)
        {
            if ((*it)->mRequestID == requestID)
            {
                request = *it;
                connection->mRequests.erase(it);
                break;
            }
        }
    }

    if(request)
    {
        ScopeRWSpinLockWrite lock(mRequestsLock);
        for (auto it = mRequests.begin(); it != mRequests.end(); ++it)
        {
            if (*it == request)
            {
                mRequests.swap_erase(it);
                break;
            }
        }
    }
}

void FileTransferMessageController::UpdateRequest(ServerRequest* request)
{
    (request);
}
void FileTransferMessageController::UpdateRequestQueryInfo(ServerRequest* request)
{
    DownloadResponse response;

    String name = request->mResourceInfo.mName;
    if (!mResourceLocator->QueryResourceInfo(name, request->mResourceInfo))
    {
        response.mChunkCount = static_cast<UInt32>(request->mResourceInfo.mChunkCount);
        response.mFragmentCount = static_cast<UInt32>(request->mResourceInfo.mFragmentCount);
        memcpy(response.mHash.mBytes, request->mResourceInfo.mHash, sizeof(response.mHash.mBytes));
        response.mRequestID = request->mClientID;
        response.mResourceHandle = request->mRequestID;
        response.mResourceSize = static_cast<UInt32>(request->mResourceInfo.mSize);
        response.mStatus = DownloadResponseStatus::DRS_SUCCESS;
    }
    else
    {
        response.mChunkCount = INVALID32;
        response.mFragmentCount = INVALID32;
        memset(response.mHash.mBytes, 0, sizeof(response.mHash.mBytes));
        response.mRequestID = request->mClientID;
        response.mResourceHandle = INVALID32;
        response.mResourceSize = 0;
        response.mStatus = DownloadResponseStatus::DRS_RESOURCE_NOT_FOUND;
    }

    ByteT buffer[512] = { 0 };
    ByteT* cursor = buffer;
    SizeT cursorSize = sizeof(buffer);
    SizeT headerSize = WriteHeader(cursor, cursorSize, DownloadMessageType::DMT_DOWNLOAD_RESPONSE);
    SizeT bodySize = WriteAllBytes(buffer, sizeof(buffer), response);
    (bodySize);
    (headerSize);

    // todo: we must clean this up...
    // HandlePtr pinnedHandle(GetAtomicPointer(handle));
    // NetDriver::Options options = NetDriver::OPTION_RELIABLE | NetDriver::OPTION_ENCRYPT | NetDriver::OPTION_SIGNED;
    // if (!mDriver->Send(
    //     NetDriver::MESSAGE_FILE_TRANSFER,
    //     options,
    //     buffer,
    //     headerSize + bodySize,
    //     NetDriver::OnSendSuccess::Make([this, pinnedHandle]() { OnDownloadResponseSent(pinnedHandle); }),
    //     NetDriver::OnSendFailed::Make([this, pinnedHandle]() { OnDownloadResponseFailed(pinnedHandle); })
    // )
    //     )
    // {
    //     // The driver Send should only fail if it's not in the correct state.
    //     gNetLog.Error(LogMessage("NetDriver failed to send DownloadRequest. ResourceName=") << handle->mInfo.mName);
    //     handle->SetState(FileResourceHandle::Failed);
    //     return;
    // }


}

}