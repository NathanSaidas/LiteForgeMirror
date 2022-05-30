Lite Forge:

### Project Structure:

Engine Development:
Solution             : File
Code                 : Directory
Tools                : Directory
Libraries            : Directory
Builds               :
Asset                :

Standalone Editor:

Standalone Game:


Q: How to build standalone game?
A: Run the LiteForgeUtility.exe and enter the command. -publish /file="PublishFileName"

The architecture of the engine overall.

+--------------------------+
|         Game/App         |
+--------------------------+
|          Engine          |
+--------------------------+
|          Service         |
+--------------------------+
|     Abstract Engine      |
+--------------------------+
|          Runtime         |
+--------------------------+
|           Core           |
+--------------------------+



Core: A collection of types/functions that help with low-level "unsafe" operations.

    Assert     -- Error handling functions
    Math       -- Primitive types and functions to calculate/solve math problems
    String     -- Primitive type and functions to handle common string operations
    Token      -- Primitive type that internalizes strings to save on memory and improve comparison performance
    Enum       -- Primitive template type that provides string <--> enum value lookup.
    Guid       -- Functions for converting to/from guid. Guid generation is abstract
    DateTime   -- Primitive type and functions to processing time.
    Timer      -- Primitive type for measuring real-time
    Array      -- Custom implementation of Array that allows for inplace memory allocation
   *Callback   -- Primitive template library that allows us to wrap functions/methods into a callback that can be invoked
    Memory     -- Provides memory allocation functions. Keeps track of memory allocated by "region name" for debugging.
    Platform   -- Provides extensions for common platform routines.
    Thread     -- Provides wrappers around platform specific thread features.
    Atomic     -- Provides wrappers around platform specific atomic features
    StackTrace -- Utility for capturing a stack trace.
    FileSystem -- Provides wrappers around platform specific filesystem features.
    Socket     -- Provides wrappers around network-sockets.
    Test       -- Unit testing framework
    TokenMgr   -- Manages string management

Runtime: A collection of types/functions that are essential to run the app. Consists of management type constructs.

    Program    -- Manages information/static-init callbacks of the program
    Reflection -- Manages runtime type information
    ASync      -- Manages asynchronous programming tasks.. ForkThread(...) RunTask(...), I think TaskMgr should be seperate.. 
               -- Think of Async of as a manager of Promises (long tasks), and TaskMgr built on synchronizing arounding fences. (fast tasks)
    MemoryMgr  -- Manages information about memory.
    Config     -- Provides an interface for configuration files
    FileIO     -- Provides an interface for serializing data in different formats.
    Test       -- A framework for testing code.
    Profiling  --
    NetSocket  --

Abstract Engine:
    ServiceContainer  -- A container of services.
    AssetService      -- Manages creation/deletion/loading/unloading of "asset" resources.
    AudioService      -- Manages sounds
    GraphicsService   -- Manages graphics
    PhysicsService    -- Manages physics operations
    NetService        -- Manages connection/network data replication between host/clients
    WebService        -- Manages connection to web-servers
    WorldService      -- Manages entities contained in a world
    EditorService     -- Manages editor operations
    InputService      -- Manages input operations
    DebugService      -- Manages debug features
    ScriptService     -- Manages scripts
    PluginService     -- Manages plugins

Service:
    AssetServiceImpl
    AudioServiceImpl

Engine:
    Bootstrapper      -- In charge of starting up the engine, initialize runtime and invoke application callbacks
    SimpleRenderer

Game:



Configure:
x32 MinimalRelease | MinimalDebug
x64 MinimalRelease | MinimalDebug

lf_shell /make_project /dest="C:\\FullPath" /name=TestProject

C:\\FullPath\\TestProject
                ..\\Engine\\Bin     -- Binaries for the engine are here for linkings against.
                ..\\Engine\\Content -- Content for the engine are here to be loaded
                ..\\Engine\\Headers -- Header files here to be viewed/referenced
                ..\\App\\Bin        -- User generated binaries.
                ..\\App\\Content    -- User generated content
                ..\\App\\Builds     -- User fullbuilds
                ..\\App\\UserContent-- Temporary User content shared between editor/builds
                ..\\App\\Plugins    -- Plugins used within the engine/app
                ..\\Temp            -- Folder for temporary data. (Considered safe to delete)

lf_shell /make_build /project="C:\\FullPath" /config="C:\\FullPath"
lf_shell /exec_tests /project="C:\\FullPath" /test="test_name"
lf_shell /pack="files;delimited" /pack /pack_config="pack config file" -- Package content into runtime package.
lf_shell /unpack= -- Split packaged file into source content.


### Globals
* CmdLine       [Core]
* TokenTable    [Core]
* ReflectionMgr [Runtime]

### Initialization
See Program::Execute for the entry point of the engine.

#### PreInit.Core ####
Set data critical to low-level primitives
* Set MainThreadVariable
* Set CrashHandler Callbacks
* Initialize Cmd Line
* Initialize Stack Trace
* Load Initialization File
* Initialize Token Table
* Initialize SCP_PRE_INIT_CORE static callbacks (static tokens)

#### PreInit.Runtime ####
* Initialize ReflectionMgr
* Initialize ThreadMgr
* Initialize TaskMgr
* Initialize Async
* Initialize Profiler
* Initialize SCP_PRE_INIT_RUNTIME static callbacks (register runtime-type data)
* Build runtime types



## Graphics

GfxService* service = GetService<GraphicsService>();
GfxResourceMgr* resMgr = service->GetResourceMgr();

GfxTextureAllocator* textureAllocator = resMgr->GetTextureAllocator();
textureAllocator->Create

Object
+-- Instance.............Anything with 'dynamic-state' is considered an instance.
+-- Resource.............Anything with without state or has 'static-state' is considered a resource.

Adapters


Texture:
    GfxTexture............Abstract interface of a texture
    GfxImageTexture.......Implementation of a texture that is used to fill pixels on an output buffer.
    GfxRenderTexture......Implementation of a texture that is used to capture pixels on an output buffer as well as display to output.

Mesh:
    GfxMesh...............Abstract interface of a mesh
    GfxStaticMesh.........Mesh created from data once
    GfxSkeletalMesh.......Mesh with bones
    GfxDynamicMesh........Mesh created that can be modified
Font:
    GfxFont...............Contains information 
    GfxFontTexture........
    GfxFontBuffer.........
Shader:

Material:



## Asset

Content Fetching/Caching

/engine/......................./data/e_0.cached
/user/characters/markus/
        ../textures/
            ../face.png
            ../face.lfpkg
        ../models/
            ../head.fbx
            ../head.lfpkg
            ../body.fbx
            ../body.lfpkg
        ../voice/
            ../dialog00.wav
            ../dialog00.lfpkg
        ../audio/
            ../footstep00.wav
            ../footstep00.lfpkg
        ../animation/
            ../walk.fbx
            ../walk.lfpkg
        ../scripts/
            ../markus.lua
            ../markus.lfpkg
        ../markus.lfpkg
        ../markusEvil.lfpkg

/user_internal/exports/
    ../export_characters_markus
        
$exports_characters_markus=PackageExport
{
    Tag="Character"
    Bundle="GameBase"
    Assets=[
        "/user/characters/markus/textures/face.png"
        "/user/characters/markus/textures/face.lfpkg"
        "/user/characters/markus/models/head.fbx"
        "/user/characters/markus/models/head.lfpkg"
        "/user/characters/markus/models/body.fbx"
        "/user/characters/markus/models/body.lfpkg"
        "/user/characters/markus/voice/dialog00.wav"
        "/user/characters/markus/voice/dialog00.lfpkg"
        "/user/characters/markus/scripts/markus.lua"
        "/user/characters/markus/scripts/markus.lfpkg"
        "/user/characters/markus/markus.lfpkg"
    ],
    ContentStreamableBlacklist=[
        "/user/characters/markus/scripts/markus.lua"
        "/user/characters/markus/markus.lfpkg"
    ]
}

content-to-cache-name:
    exports = GetPackageExport("/user/characters/markus/models/head.fbx")
    tagLookUp = exports.Bundle + "." exports.Tag;
    binName = GetExportFile(tagLookUp); // gb_c
    binTypePostFix = GetTypePostFix(asset.ConcreteType);
    binName += binTypePostFix; // gb_c_m_00;
    prepare_manifest[binName].push(asset);

content-resolve-bin-id:
    bin;
    foreach(asset : prepare_manifest[binName])
        if(bin.Current.IsFull())
        {
            bin.Push();
        }
        bin.Current.PushAssetData(asset);


StreamAdapter:
    EditorStreamLoader:
    CacheStreamLoader:






LOADED "/user/characters/markus/markus.lfpkg:Health=100"
LOADED "/user/characters/markus/markus.lfpkg:OnDamagedScript=/user/characters/markus/scripts/markus.lua:OnDamaged"
UPDATE "/user/characters/markus/markus.lfpkg:MaxHealth=75"

(^this will require a system restart or a sync point.)

Level::Load(<level>)
    => AssetCache->UnloadCriticalStaleContent()
    => list = AssetCache->SyncCriticalStaleContent()
    => AssetMgr->ReloadCriticalContent(list)

Level::StreamIn(<level>)
Level::StreamOut(<level>)

.props

[/user/characters/markus/markus.lfpkg]
Health=250;


struct MarkusProps
{
    void Load(IniStream& s)
    {
        m_Health = s.GetFloat("m_Health");
    }

    float Health;
}


"GameBase.Character"

$ExportTagsData=ExportTags
{
    Tags=[
        {
            BinName="gb_c",
            Export="GameBase.Character"
        }
    ]
}

$ExportManifestData=ExportManifest
{
    Exports=[
        {
            # The name of the asset used for look-ups
            Asset="/user/characters/markus/textures/face.png"
            # The export file it will be stored in
            File="/data/gb_c_t_00.lfcache"
            # The object id within the file it's stored in (cache only)
            Object=0
            # The type of object
            Type="lf::GfxImageTextureData"
            # File version
            Version=172
            # File Integrity
            Hash="de9f2c7fd25e1b3afad3e85a0bd17d9b100db4b3"
        }
        {
            # The name of the asset used for look-ups
            Asset="/user/characters/markus/textures/face.lfpkg"
            # The export file it will be stored in
            File="/data/gb_c_00.lfcache"
            # The object id within the file it's stored in (cache only)
            Object=37
            # The type of object
            Type="lf::GfxImageTexture"
            # File version
            Version=172
            # File Integrity
            Hash="de9f2c7fd25e1b3afad3e85a0bd17d9b100db4b3"
        }
    ]
}


Texture texture("/user/characters/markus/textures/face");
#Release any resources the texture holds onto
1. Clear();
#Attempt to load in synchronous fashion (async operations might be kicked and we'll have to .Wait)
2. GfxTextureRef::mTextureRef::LoadSync("/user/characters/markus/textures/face")
3. AssetMgr::LoadSync("/user/characters/markus/textures/face")
#Correct the path, in this case an extension wasn't provided .lfpkg assumed
4. var path = "/user/characters/markus/textures/face.lfpkg" = AssetPath::CorrectPath(input)
#Fetch details on the resource
5. var header = AssetCache::GetHeader(path)
    a) var assetId = AssetCache::mExportsNameTable[path]
    b) return null_if Invalid(assetId)
    c) return AssetCache::mExports[assetId]
#Can't allocate resource when types don't match
6. exit_if header.Type != GfxTexture
#Create instance
7. var instance = Reflection.Create(header.Type);
8. instance->SetAssetType(header);
#Begin loading
9.  var assetStream = AssetCache::GetStreamSync(header.File);
10. var streamReader = assetStream.GetInstance(header.Object);
#Note: Serialize here does not load dependencies, eg GfxImageTextureData should have an instance but hasn't loaded data.
11. instance->Serialize(streamReader);
#Load all dependencies detected.
12. streamReader.RecursiveResolveDependenciesSync();
#Finally release the stream so the main stream may get GCed
12. streamReader.Close();
13. instance->OnLoadSync();
14. instance->OnPostLoadSync();
15. 

### ResolveSync(streamReaderRoot, asset["/user/characters/markus/textures/face.png"])
if(streamReaderRoot.IsProcessed(asset.header))
{
    return;
}
var assetStream = AssetCache::GetStreamSync(asset.header.File);
var streamReader = assetStream.GetInstance(asset.header.Object);
streamReader.SetRoot(streamReaderRoot);
asset->Serialize(streamReader);
streamReader.RecursiveResolveDependenciesSync();
streamReader.Close();
asset->OnLoadSync();
    a) UploadTextureData 


GfxImageTextureData::Serialize(Stream& s)
{
    // PinnableStreams are held in memory until unpinned.
    if(s.IsPinnable())
    {
        s.Pin(GetAssetType());
        mDataStream = &s;
        mDataAddress = mDataStream->GetPropertyAddress("mBuffer");
    }
    else
    {
        SERIALIZE(s, mBuffer, "");
    }
}

GfxImageTextureData::OnLoadSync()
{
    if(mDataStream)
    {
        mAdapter->Load(mDataAddress.GetData(), mDataAddress.GetSize());
        mDataStream->Unpin(GetAssetType());
        mDataStream = nullptr;
        mDataAddress = StreamPropertyAddress();
    }
    else
    {
        mAdapter->Load(mBuffer.GetData(), mBuffer.GetSize());
    }
}

GfxImageTextureData::OnReload()
{
    mAdapter->Release();
    if(mDataStream)
    {
        mAdapter->Load(mDataAddress.GetData(), mDataAddress.GetSize());
        mDataStream->Unpin(GetAssetType());
        mDataStream = nullptr;
        mDataAddress = StreamPropertyAddress();
    }
    else
    {
        mAdapter->Load(mBuffer.GetData(), mBuffer.GetSize());
    }
}




#Attempt to locate the data in the cache and get a stream
5. var assetStreamInstance = AssetCache::Fetch(path)
    a) var assetId = AssetCache::mExportsNameTable[path]
    b) exit_if Invalid(assetId)
    c) var assetExportHeader = AssetCache::mExports[assetId]
    d) var assetStream = AssetCache::GetStream(assetExportHeader.File)
    e) var assetStreamInstance = assetStream.GetInstance(assetExportHeader.Object)
    f) return (assetStreamInstance, assetExportHeader.Type);
6. 




OperationType:
    Add
    Update
    Delete

Priority:
    Critical
    Normal
    Low


Content can be added/updated/deleted
Adds can come in any priority although they will typically be Normal/Low
Updates can come in any priority although anything that changes gameplay is critical
All deletes are critical


Content Server.......Hosts content meaning it will notify the GameUpdater when there is new content.
API Server...........Hosts web-api-like interface for match making/storing persistent information
Game Server..........Hosts the immediate game-server which may be required for online games
Game Client..........Hosts the client (human player) 
Game Updater.........Registers communication with content server on startup to ensure cache can be updated correctly
                     Maintains the communication in the event there are game updates
Game Cache...........Hosts the client local-copy of the content


Singleplayer:
    In the event of a content update that contains Critical items, the Game Client is prompted to restart and
update the game-cache with new critical content, any Normal/Low priority content can be streamed in


Multiplayer:
    In the event of a content update that contains Critical items, all Game Servers with stale Content Server versions
are locked and will expire connections in 20 minutes. All connected Game Clients are prompted to restart and
update the game-cache with the new critical content, after the Game Client gets the required content they can join
any game server with a matching Content Server version.
    

Game Server caching:
    During content deploy some game servers are 'pre-heated' meaning they get the content early, other expiring game servers
can use the same Game Updater to update their content and restart.

Since Game Server's don't typically require visual/audible assets they can keep a smaller 'data cache'



Asset:
Name..........: (Unique) -- Determined by a content creator
ID............: (Unique) -- Determined by content server
Cache Location: (Unique|ClientOnly) -- Determined by Game Updater

# Content Operations:

Assume we have Assets
[ Asset.ID :   Size(B) :    Name   ]
[   723    |   607252  | Bush1.png ]
[   427    |   592652  | Bush2.png ] 
[   864    |   732137  | Bush3.png ] 
[   824    |   782395  | Bush4.png ] 
[   726    |  1028271  | Bush5.png ] 
[   72     |  1140934  | Bush6.png ] 


Assume we have a CacheBlob
[ Index : Asset.ID ]
[    0  |   723    ] 607252
[    1  |   427    ] 592652
[    2  |   864    ] 732137
[    3  |   824    ] 782395
[    4  |   null   ] 262994
[    5  |   726    ] 1028271
[    6  |   72     ] 1140934

Total Bytes Allocated....:  5 146 635
Total Bytes Reserved.....: 10 000 000
Total Bytes Fragmented...:    262 994
Bytes Used (w/ Fragments):  5 146 635 (51.5%)
Bytes Used...............:  4 883 641 (48.8%)
Fragmented...............:    262 994 ( 5.1%)


### Create
To create an asset we need to reserve space for it in a CacheBlob

Blob::Reserve(607252);

Maybe our blob has space in Blob[5]
If it doesn't then we need to go to the end and allocate a new BlobIndex
If the blob cannot contain a new index with size 607252 bytes we must allocate a new blob
### Update
To update an asset we need to know its updated size

ID=427
Size=612300
Blob::Update(ID, Size)

This will update index 1 IF Size is less than the Blob[1].Capacity
If there is not enough space for the updated object then the Blob[1] is nulled out

Blob[1] = null;

And then we do the Create process

{ BlobID, BlobIndex } = Blob::Create(Size)

And then finally update the CacheLocation of the asset

Asset[427].CacheLocation = ( BlobID | BlobIndex )
### Destroy
To destroy an asset we simply can null out the Blob index and remove the asset from the manifest

### Find
To find asset data we need to use it's CacheLocation to first get the BlobID and then the index into 
that blob for where the data is

### Optimize
Because Create/Update/Destroy can leave holes in our data it would be best to do an optimization step where
reallocate the blobs so there are no more holes and less memory overall is wasted on blob extra blobs.

BlobProperties: {
    Fragmented: Bytes that are reserved by a blob index but not used by an asset
}

In the case above 