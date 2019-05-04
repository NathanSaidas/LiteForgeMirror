#include "Core/IO/BinaryStream.h"
#include "Core/IO/TextStream.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Reflection/Object.h"
#include "Core/Reflection/Type.h"
#include "Core/String/String.h"
#include "Core/String/StringCommon.h"
#include "Core/String/Token.h"
#include "Core/String/TokenTable.h"
#include "Core/Test/Test.h"
#include "Core/Utility/DateTime.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Utility.h"

#include "Runtime/Asset/AssetDataController.h"
#include "Runtime/Asset/AssetCacheController.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/AssetTypes.h"
#include "Runtime/Asset/CacheBlob.h"
#include "Runtime/Asset/CacheBlock.h"
#include "Runtime/Asset/CacheWriter.h"
#include "Runtime/Asset/CacheStream.h"
#include "Runtime/Reflection/ReflectionMgr.h"
#include "Runtime/Reflection/ReflectionTypes.h"

#include <algorithm>

// todo:
// + Implement CacheBlock::Write(bytes)
// + Implement CacheBlock::Read(bytes)
// + Come up with a async/concurrent model of CacheBlock
// + Commenting and Documentation CacheBlock

namespace lf {

struct CacheAssetRequest
{
    Token           mCacheBlock;
    CacheIndex      mCacheIndex;
    AssetType*      mAssetType;
    MemoryBuffer    mCompiledData;
};

static const UInt32 KB = 1024;
static const UInt32 MB = 1024 * KB;
static const char* BUG_MESSAGE = "";
static const char* NULL_MSG = "";
static BugCallback TestBugReporter = [](const char* msg, ErrorCode, ErrorApi) { BUG_MESSAGE = msg; };

struct MockAssetData
{
    MockAssetData() : mID(INVALID32), mSize(0), mName(), mCacheObjectID(INVALID16) {}
    MockAssetData(UInt32 id, UInt32 size, const char* name) : mID(id), mSize(size), mName(name), mCacheObjectID(INVALID16) {}
    UInt32 mID;
    UInt32 mSize;
    String mName;

    CacheObjectId mCacheObjectID;
};

struct MockCacheDefrag
{
    CacheObjectId mSource;
    CacheObjectId mDest;
    UInt32        mAssetID;
};

// Stubs:
struct StubAssetTypeData
{
    AssetTypeData mTypeData;
    UInt32        mSize;
    String        mCacheName;
};
struct StubAssetCacheHeader
{
    String mCacheName;
    UInt32 mSize;
    UInt32 mUID;
    CacheObjectId mBlobObjectID;
};
LF_FORCE_INLINE Stream& operator<<(Stream& s, StubAssetCacheHeader& self)
{
    SERIALIZE(s, self.mCacheName, "");
    SERIALIZE(s, self.mUID, "");
    SERIALIZE(s, self.mSize, "");
    SERIALIZE(s, self.mBlobObjectID, "");
    return s;
}

#define CREATE_ASSET_STUB(Name)                         \
class StubAsset##Name : public AssetObject              \
{                                                       \
    DECLARE_CLASS(StubAsset##Name, AssetObject);        \
};                                                      \
DEFINE_CLASS(lf::StubAsset##Name) { NO_REFLECTION; }        \
class StubAsset##Name##Data : public AssetObject        \
{                                                       \
    DECLARE_CLASS(StubAsset##Name##Data, AssetObject);  \
};                                                      \
DEFINE_CLASS(lf::StubAsset##Name##Data) { NO_REFLECTION; }

CREATE_ASSET_STUB(Texture);
CREATE_ASSET_STUB(Font);
CREATE_ASSET_STUB(Audio);
CREATE_ASSET_STUB(Mesh);
CREATE_ASSET_STUB(Shader);
CREATE_ASSET_STUB(Level);
CREATE_ASSET_STUB(Script);
#undef CREATE_ASSET_STUB

#define CREATE_ASSET_OBJECT_STUB(Name, Base)            \
class StubAsset##Name : public Base                     \
{                                                       \
    DECLARE_CLASS(StubAsset##Name, Base);               \
};                                                      \
DEFINE_CLASS(lf::StubAsset##Name) { NO_REFLECTION; }

CREATE_ASSET_OBJECT_STUB(Material, AssetObject);
CREATE_ASSET_OBJECT_STUB(Character, AssetObject);
CREATE_ASSET_OBJECT_STUB(Hunter, StubAssetCharacter);
CREATE_ASSET_OBJECT_STUB(AdamHunter, StubAssetHunter);
CREATE_ASSET_OBJECT_STUB(KrisHunter, StubAssetHunter);
CREATE_ASSET_OBJECT_STUB(Warlock, StubAssetCharacter);
CREATE_ASSET_OBJECT_STUB(AdamWarlock, StubAssetWarlock);
CREATE_ASSET_OBJECT_STUB(KrisWarlock, StubAssetWarlock);
CREATE_ASSET_OBJECT_STUB(Titan, StubAssetCharacter);
CREATE_ASSET_OBJECT_STUB(AdamTitan, StubAssetTitan);
CREATE_ASSET_OBJECT_STUB(KrisTitan, StubAssetTitan);


#define DEFINE_MAKE_ASSET(AssetType, CacheName, Category)                                                                                                              \
void Make##AssetType(const char* name, UInt32 uid, UInt32 size, const char* dataHash, const char* objectHash, StubAssetTypeData& outData, StubAssetTypeData& outObject)  \
{                                                                                                                                                   \
    outData.mTypeData.mFullName = Token(name);                                                                                                                \
    outData.mTypeData.mConcreteType = typeof(StubAsset##AssetType##Data)->GetFullName();                                                                      \
    outData.mTypeData.mCacheName = CacheName;                                                                                                                 \
    outData.mTypeData.mUID = uid;                                                                                                                             \
    outData.mTypeData.mParentUID = INVALID32;                                                                                                                 \
    outData.mTypeData.mVersion = 0;                                                                                                                           \
    outData.mTypeData.mAttributes = 0;                                                                                                                        \
    outData.mTypeData.mFlags = 1 << AssetFlags::AF_BINARY;                                                                                                    \
    outData.mTypeData.mCategory = Category;                                                                                                  \
    TEST(outData.mTypeData.mHash.Parse(dataHash));                                                                                                            \
    outData.mSize = size;                                                                                                                                   \
                                                                                                                                                    \
    outObject.mTypeData.mFullName = Token(String(name) + ".lfpkg");                                                                                           \
    outObject.mTypeData.mConcreteType = typeof(StubAsset##AssetType)->GetFullName();                                                                          \
    outObject.mTypeData.mCacheName = "gb";                                                                                                                    \
    outObject.mTypeData.mUID = uid + 1;                                                                                                                       \
    outObject.mTypeData.mParentUID = INVALID32;                                                                                                               \
    outObject.mTypeData.mVersion = 0;                                                                                                                         \
    outObject.mTypeData.mAttributes = 0;                                                                                                                      \
    outObject.mTypeData.mFlags = 0;                                                                                                                           \
    outObject.mTypeData.mCategory = AssetCategory::AC_SERIALIZED_OBJECT;                                                                                      \
    TEST(outData.mTypeData.mHash.Parse(objectHash));                                                                                                          \
    outObject.mSize = 2 * 1024;                                                                                                                             \
}                                                                                                                                                   \
void Make##AssetType(const char* name, UInt32 uid, UInt32 size, const char* dataHash, const char* objectHash, TArray<StubAssetTypeData>& outTypes)                   \
{                                                                                                                                                   \
    StubAssetTypeData data;                                                                                                                             \
    StubAssetTypeData object;                                                                                                                           \
    Make##AssetType(name, uid, size, dataHash, objectHash, data, object);                                                                                 \
    outTypes.Add(data);                                                                                                                             \
    outTypes.Add(object);                                                                                                                           \
}                                                                                                                                                   

void MakeObject(const Type* concreteType, const char* name, UInt32 uid, const UInt32 size, const char* hash, TArray<StubAssetTypeData>& types)
{
    StubAssetTypeData object;
    object.mTypeData.mFullName = Token(String(name) + ".lfpkg");
    object.mTypeData.mConcreteType = concreteType->GetFullName();
    object.mTypeData.mCacheName = "gb";
    object.mTypeData.mUID = uid;
    object.mTypeData.mParentUID = INVALID32;
    object.mTypeData.mVersion = 0;
    object.mTypeData.mAttributes = 0;
    object.mTypeData.mFlags = 0;
    object.mTypeData.mCategory = AssetCategory::AC_SERIALIZED_OBJECT;
    TEST(object.mTypeData.mHash.Parse(hash));
    object.mSize = size;

    types.Add(object);
}

void DeriveObject(const char* name, const char* parent, UInt32 uid, UInt32 size, const char* hash, TArray<StubAssetTypeData >& types)
{
    Token parentName(String(parent) + ".lfpkg");
    for (const StubAssetTypeData & data : types)
    {
        if (data.mTypeData.mFullName == parentName)
        {
            StubAssetTypeData object;
            object.mTypeData.mFullName = Token(String(name) + ".lfpkg");
            object.mTypeData.mConcreteType = data.mTypeData.mConcreteType;
            object.mTypeData.mCacheName = "gb";
            object.mTypeData.mUID = uid;
            object.mTypeData.mParentUID = data.mTypeData.mUID;
            object.mTypeData.mVersion = 0;
            object.mTypeData.mAttributes = 0;
            object.mTypeData.mFlags = 0;
            object.mTypeData.mCategory = AssetCategory::AC_SERIALIZED_OBJECT;
            TEST(object.mTypeData.mHash.Parse(hash));
            object.mSize = size;
            types.Add(object);
            return;
        }
    }
    TEST(false); // object does not exist
}

DEFINE_MAKE_ASSET(Texture, "gb_t", AssetCategory::AC_TEXTURE);
DEFINE_MAKE_ASSET(Font, "gb_f", AssetCategory::AC_FONT);
DEFINE_MAKE_ASSET(Audio, "gb_a", AssetCategory::AC_AUDIO);
DEFINE_MAKE_ASSET(Mesh, "gb_m", AssetCategory::AC_MESH);
DEFINE_MAKE_ASSET(Shader, "gb_s", AssetCategory::AC_SHADER);
DEFINE_MAKE_ASSET(Level, "gb_l", AssetCategory::AC_LEVEL);
DEFINE_MAKE_ASSET(Script, "gb_x", AssetCategory::AC_SCRIPT);
#undef DEFINE_CREATE_ASSET

void PopulateAssetCategories(const Type* categoryTypes[AssetCategory::MAX_VALUE])
{
    categoryTypes[AssetCategory::AC_TEXTURE] = typeof(StubAssetTextureData);
    categoryTypes[AssetCategory::AC_FONT] = typeof(StubAssetFontData);
    categoryTypes[AssetCategory::AC_AUDIO] = typeof(StubAssetAudioData);
    categoryTypes[AssetCategory::AC_MESH] = typeof(StubAssetMeshData);
    categoryTypes[AssetCategory::AC_SHADER] = typeof(StubAssetShaderData);
    categoryTypes[AssetCategory::AC_LEVEL] = typeof(StubAssetLevelData);
    categoryTypes[AssetCategory::AC_SCRIPT] = typeof(StubAssetScriptData);
    categoryTypes[AssetCategory::AC_SERIALIZED_OBJECT] = typeof(AssetObject);
}

void PopulateSampleAssets(TArray<StubAssetTypeData>& types)
{
    MakeTexture("/User/Environments/AncientForest/Textures/grass0.png", 0, 370688, "ddfef8c83e5a5f337e8d145d5b0d0fd3", "fe797073bb0ab6bed7632320ab04e3e6", types);
    MakeTexture("/User/Environments/AncientForest/Textures/grass1.png", 2, 278528, "4f2a2564cb75cc00fb10a8d9835a1583", "6119100460826c1b0b8379e0334efb66", types);
    MakeTexture("/User/Environments/AncientForest/Textures/grass2.png", 4, 442368, "5413a64b87202ba90f935f85848dc568", "d205a0a268bc22395aa264e4b688b234", types);
    MakeTexture("/User/Environments/AncientForest/Textures/grass3.png", 6, 122880, "83586fce71a1ad795c54be31ce8c7786", "129a6cb6cc13734767868831033647da", types);
    MakeTexture("/User/Environments/AncientForest/Textures/grass4.png", 8, 339968, "b1823fa3f53089cea0985bbd85a0e7c9", "92e2432840dd8fd1505a6e0d4ba0840e", types);
    MakeTexture("/User/Environments/AncientForest/Textures/grass5.png", 10, 737280, "471de2c44b7fe4eec7a76d9d74c274a1", "eacde3347c05d9b3d89c082acc15a5bf", types);
    MakeTexture("/User/Environments/AncientForest/Textures/grass6.png", 12, 104448, "db8bc2bd3ac2269a7026c4b26af76039", "c5a881d60568506aa52ef00027de3402", types);
    MakeTexture("/User/Environments/AncientForest/Textures/grass7.png", 14, 124928, "64bd1f431582e79dfa30fdb402256988", "092d6e70db359665cbb2b2b08538a9e0", types);
    MakeTexture("/User/Environments/AncientForest/Textures/grass8.png", 16, 238592, "a033851f25f59c18f5f9883a1ae1b7bc", "f45adf0afc899597f2279253973c6e6e", types);
    MakeTexture("/User/Environments/AncientForest/Textures/grass9.png", 18, 557056, "f43d217fe54cffc58803b0ca0b8ee9a8", "c9a00017be5965bbcf682ee9ba3fe3b8", types);
    MakeTexture("/User/Environments/AncientForest/Textures/grass10.png", 20, 22528, "42db8219ce201eea9865a3a4d09f393a", "e79545b4d9d4152a9453fc5d390f8fc0", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass0.fbx", 22, 73728, "ffd0f6ae455d688c8a2a13118af3cd0e", "b6d7038615d9fad1a225aa6194881d86", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass1.fbx", 24, 86016, "a397b906fe41b42fdc867e730cf4da5e", "bd49148b6fb34286cdb40f11cea8c28d", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass2.fbx", 26, 165888, "5fc099d61eacbccf091c96abcb8d64dc", "4ef22d954d1111d5800e720ac69732ef", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass3.fbx", 28, 32768, "96fb8f3890f5d2b53a56bcd676e758d4", "cb357d8f6a344556a21e2a618df70d85", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass4.fbx", 30, 134144, "075e5e04d9cbd65ebbfb907a6bf5fd6e", "7be8724bbc2b87741a8299de824f19da", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass5.fbx", 32, 186368, "63e0784ad84c8d1566d55ae28ddca104", "f86c4ee02dc9de6e009c818986721c76", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass6.fbx", 34, 23552, "3628c829c8a770effb315ca3146fbfff", "e74fe61b992da8c2dcf8ea20b51fc097", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass7.fbx", 36, 45056, "bb58be40dd636f79452e5fd352c9bb32", "442b69999f28a1d12bfa723eddb64e92", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass8.fbx", 38, 49152, "16b346d58655c44176277cee61522be1", "e060800cf35b16d822e5959891465341", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass9.fbx", 40, 2048, "072d3937f7b66173dc2d540c08da426f", "3471ba94524cf7e90f73ddb42c7097b4", types);
    MakeMesh("/User/Environments/AncientForest/Models/grass10.fbx", 42, 12288, "3554802de1ed2dcad5cbf17f0374a7ec", "df8baa8b65ff222e3b881cfb67c9abf4", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient0.wav", 44, 2097152, "16859031f3156bdb93a5b7d59ddacf3d", "65759bc696633348622ceba9e393329f", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient1.wav", 46, 1048576, "84311b6ffe57ed694640b4e26bdfc8e7", "b6254917e8b171c249925e7cf1178fd7", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient2.wav", 48, 2394112, "4253826ec54cfa3ca4a5a5f7ccd5c26c", "4045e8770e12b201edca953c77de7bb4", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient3.wav", 50, 1330176, "cf942b0561109ae91a94f61e8825ed35", "7b33c7f916fe08ce0cf020bc787f24ea", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient4.wav", 52, 3009536, "3895a2f0675e56ba9f7fd7ecf6a5f51b", "282918e5d018211688e1d3d9d65061cd", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient5.wav", 54, 1965056, "9edacd8c9473932c943eca365a307507", "b5e710da5dcb968b16f7b120f0914d33", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient6.wav", 56, 1252352, "78a6918f75252beadc06d45aae3d75ac", "b519f94c775fd2e3b7b553a2b96173db", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient7.wav", 58, 1965056, "e0d58c6ebe71011270e896538a475459", "fd22d804e2e4ae86b77174ebbfc08b70", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient8.wav", 60, 1149952, "db04e34af91badc29906769db500f3f4", "55f20083dc99dd306702324a085099b0", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient9.wav", 62, 1892352, "034276d6f42fddae89e20e5ab8b0927e", "4083752abacdbaa3568491a8e350df50", types);
    MakeAudio("/User/Environments/AncientForest/Sounds/windAmbient10.wav", 64, 1012736, "808c5e9f0a245446c8c9063364b0efb4", "d9669235a36343263392344d38c4fe43", types);
    MakeShader("/User/Environments/AncientForest/Shaders/GrassFill.shader", 66, 184320, "0f7ed1d4ebbd9b39b9e8e8a789c0d0a5", "d20320a72e606aff6bbf4846e0572423", types);
    MakeShader("/User/Environments/AncientForest/Shaders/PbrGrassFill.shader", 68, 112640, "4750cdbaf213ea54e7ab9a019de3caec", "5916ae711ee33bd7f8e65ab52d4854ec", types);
    MakeShader("/User/Environments/AncientForest/Shaders/VertexGrassAnimate.shader", 70, 225280, "bb420fe6d0b1851a2165503d96b8a872", "ca94afd46d49299e7179b7b23a099e5b", types);
    MakeShader("/User/Environments/AncientForest/Shaders/VertexGrassAnimateWind.shader", 72, 138240, "f3416d4e4d7822512c37b234b825e566", "38375de0c3f42264916a2ed31cb8768c", types);
    MakeShader("/User/Environments/AncientForest/Shaders/VertexGrassAnimateForce.shader", 74, 23552, "a8151d0a12155f448a523e4979004150", "de1f853f0b01929ae166fcbfd2520947", types);
    MakeLevel("/User/Environments/AncientForest/Levels/Test.level", 76, 18432, "244db56ec085f07b68cd25b6a21721b0", "f7e6378d94cae10eec570e5150fcf034", types);
    MakeLevel("/User/Environments/AncientForest/Levels/TestShaders.level", 78, 10240, "47e90dd663d7f281d03905678f11dd4f", "9b9f97d333fcf68ac0c75cafdb53956d", types);
    MakeLevel("/User/Environments/AncientForest/Levels/ShowAssets.level", 80, 7340032, "83f32a1b78ec8c18a4baa797677459e5", "cf42dc988f2dfffdcc954f7d69c922b2", types);
    MakeObject(typeof(StubAssetAdamHunter), "/User/Environments/AncientForest/AdamHunter", 82, 1030, "4ec88c1a74713fcf2ec6f5b56503a7c3", types);
    MakeObject(typeof(StubAssetAdamWarlock), "/User/Environments/AncientForest/AdamWarlock", 83, 199, "f76d364cf14b411e32acbb0ea5dfaa6d", types);
    MakeObject(typeof(StubAssetAdamTitan), "/User/Environments/AncientForest/AdamTitan", 84, 1004, "cd7d5b141346e0198d72bb47836453d4", types);
    DeriveObject("/User/Environments/AncientForest/SuperAdamHunter", "/User/Environments/AncientForest/AdamHunter", 85, 2392, "631cd9e2c348137cc966baa44c109fff", types);
    DeriveObject("/User/Environments/AncientForest/SuperAdamWarlock", "/User/Environments/AncientForest/AdamWarlock", 86, 4899, "a8497bbaa5ad60517b31c42559ff80ec", types);
    DeriveObject("/User/Environments/AncientForest/SuperAdamTitan", "/User/Environments/AncientForest/AdamTitan", 87, 1002, "64811a4fa0e1e2f7076e0d020dd94782", types);
    MakeObject(typeof(StubAssetKrisHunter), "/User/Environments/AncientForest/KrisHunter", 88, 9288, "630cd7daa98436cb4bfcce6ee3f65ea6", types);
    MakeObject(typeof(StubAssetKrisWarlock), "/User/Environments/AncientForest/KrisWarlock", 89, 4390, "8d0a253cbb17c539025ba03c8edb839b", types);
    MakeObject(typeof(StubAssetKrisTitan), "/User/Environments/AncientForest/KrisTitan", 90, 200, "14aa1b361f48bad572e253e31aab40bf", types);
    DeriveObject("/User/Environments/AncientForest/SuperKrisHunter", "/User/Environments/AncientForest/KrisHunter", 91, 383, "41dd615fb5a72449490aa07cbb3277d3", types);
    DeriveObject("/User/Environments/AncientForest/SuperKrisWarlock", "/User/Environments/AncientForest/KrisWarlock", 92, 199, "25696b2e0d57a522ea5005305c7e0f6a", types);
    DeriveObject("/User/Environments/AncientForest/SuperKrisTitan", "/User/Environments/AncientForest/KrisTitan", 93, 1024, "35d6f7dd4174c81a01da7b8d4c8213a4", types);

    for (auto& type : types)
    {
        switch (type.mTypeData.mCategory)
        {
            case AssetCategory::AC_TEXTURE: { type.mCacheName = "_t"; } break;
            case AssetCategory::AC_FONT: { type.mCacheName = "_f"; } break;
            case AssetCategory::AC_AUDIO: { type.mCacheName = "_a"; } break;
            case AssetCategory::AC_MESH: { type.mCacheName = "_m"; } break;
            case AssetCategory::AC_SHADER: { type.mCacheName = "_s"; } break;
            case AssetCategory::AC_LEVEL: { type.mCacheName = "_l"; } break;
            case AssetCategory::AC_SCRIPT: { type.mCacheName = "_x"; } break;
            case AssetCategory::AC_SERIALIZED_OBJECT:
            default:
                break;
        }
    }
}

void StubFillCacheData(MemoryBuffer& buffer, String& text)
{
    TArray<StubAssetTypeData> data;
    PopulateSampleAssets(data);

    TArray<StubAssetCacheHeader> headers;
    AssetCacheController cache;
    for (const auto& type : data)
    {
        Token cacheName(type.mTypeData.mCacheName);
        auto blockIndex = cache.FindCacheBlockIndex(cacheName);
        if (Invalid(blockIndex))
        {
            TEST_CRITICAL(cache.CreateBlock(cacheName));
        }
        blockIndex = cache.FindCacheBlockIndex(cacheName);
        CacheIndex index = cache.Create(blockIndex, type.mTypeData.mUID, type.mSize);
        TEST_CRITICAL(index == true);
        TEST_CRITICAL(index.mUID == type.mTypeData.mUID);

        StubAssetCacheHeader header;
        header.mSize = type.mSize;
        header.mUID = index.mUID;
        header.mBlobObjectID = static_cast<CacheObjectId>(index.mObjectID);
        header.mCacheName = cacheName.CStr();
        header.mCacheName += "_" + ToString(index.mBlobID);
        headers.Add(header);
    }

    BinaryStream bs;
    bs.Open(Stream::MEMORY, &buffer, Stream::SM_WRITE);
    bs.BeginObject("AssetCache", "NativeObject");
    SERIALIZE_STRUCT_ARRAY(bs, headers, "");
    bs.EndObject();
    bs.Close();

    TextStream ts;
    ts.Open(Stream::TEXT, &text, Stream::SM_WRITE);
    ts.BeginObject("AssetCache", "NativeObject");
    SERIALIZE_STRUCT_ARRAY(ts, headers, "");
    ts.EndObject();
    ts.Close();
}

void ReportBlobState(CacheBlob& blob, const char* header)
{
    auto GetPercent = [](SizeT num, SizeT denom) { return 100.0 * (static_cast<double>(num) / static_cast<double>(denom)); };

    LoggerMessage log;
    log << header << "\n";
    log << StreamPrecision(1);
    log << "Total Bytes Allocated.....:" << StreamFillRight(8) << blob.GetBytesUsed() << StreamFillRight() << " / " << StreamFillRight(8) << blob.GetBytesReserved() << StreamFillRight() << "\n";
    log << "Total Bytes Reserved......:" << StreamFillRight(8) << blob.GetCapacity() << StreamFillRight() << "\n";
    log << "Total Bytes Fragmented....:" << StreamFillRight(8) << blob.GetFragmentedBytes() << StreamFillRight() << "\n";
    log << "Used Usage................:" << StreamFillRight(8) << GetPercent(blob.GetBytesUsed(), blob.GetCapacity()) << StreamFillRight() << "%\n";
    log << "Reserved Usage............:" << StreamFillRight(8) << GetPercent(blob.GetBytesReserved(), blob.GetCapacity()) << StreamFillRight() << "%\n";
    log << "Fragmented................:" << StreamFillRight(8) << GetPercent(blob.GetFragmentedBytes(), blob.GetBytesReserved()) << StreamFillRight() << "%\n";

    auto state = log.mContent.Push();
    log << StreamPrecision(1);
    log << "[......Visualization.....]\n";

    for (SizeT i = 0, size = blob.Size(); i < size; ++i)
    {
        CacheObject obj;
        if (blob.GetObject(static_cast<CacheObjectId>(i), obj))
        {
            log << StreamFillRight(2) << i << StreamFillRight() << " | ";
            if (Invalid(obj.mUID))
            {
                log << "null ";
            }
            else
            {
                log << StreamFillRight(4) << obj.mUID << StreamFillRight() << " ";
            }
            log << StreamFillRight(8) << obj.mSize << StreamFillRight() << " /" << StreamFillRight(8) << obj.mCapacity << StreamFillRight() 
                << " " << StreamFillRight(2) << GetPercent(obj.mSize, obj.mCapacity) << StreamFillRight() << "%\n";
        }
        else
        {
            log << "---INVALID OBJECT ID---\n";
        }
    }


    log.mContent.Pop(state);
    gTestLog.Debug(log);
}


struct AssetEditorTypeInfo
{
    Token               mSourceFile;
    DateTimeEncoded     mLastModify;
    TArray<ObjectPtr>   mInstances;
};

const SizeT ATI_SIZE = sizeof(AssetType);
const SizeT ATI_EDITOR_SIZE = sizeof(AssetEditorTypeInfo);
const SizeT ATD_SIZE = sizeof(AssetTypeData);


//
// Exporter:
//   + Contains a list of Bundles
//   + Contains a list of PackageExports

//   + When exporting it can iterate through PackageExports and generate a 'Block Title' [ Bundle + Tag ]
// 
// 

String AssetNameToFilePath(const String& assetName)
{
    String workingDir = FileSystem::GetWorkingPath();
    workingDir += "/../Content" + assetName;
    return FileSystem::PathResolve(workingDir);
}



REGISTER_TEST(CacheBlob_FragmentationTest)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    TArray<MockAssetData> assets;
    assets.Add(MockAssetData(723, 607252, "Bush1.png"));
    assets.Add(MockAssetData(427, 592652, "Bush2.png"));
    assets.Add(MockAssetData(172, 262994, "Bush3.png"));
    assets.Add(MockAssetData(864, 732137, "Bush4.png"));
    assets.Add(MockAssetData(824, 782395, "Bush5.png"));
    assets.Add(MockAssetData(726, 1028271, "Bush6.png"));
    assets.Add(MockAssetData(72, 1140934, "Bush7.png"));

    CacheBlob blob;
    blob.Initialize(TArray<CacheObject>(), 10 * MB);
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);

    for (SizeT i = 0; i < assets.Size(); ++i)
    {
        assets[i].mCacheObjectID = blob.Reserve(assets[i].mID, assets[i].mSize);
        TEST_CRITICAL(Valid(assets[i].mCacheObjectID));
        TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);
    }

    
    ReportBlobState(blob, "Before Destroy");

    // Destroy Bush3:
    CacheObjectId objectID = assets[2].mCacheObjectID;
    TEST_CRITICAL(blob.Destroy(objectID));
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);
    assets[2].mCacheObjectID = INVALID16;
    assets[2].mSize -= 56020;
    ReportBlobState(blob, "Destroy Bush3:");
    
    // Destroy Bush5:
    objectID = assets[4].mCacheObjectID;
    TEST_CRITICAL(blob.Destroy(objectID));
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);
    assets[4].mCacheObjectID = INVALID16;
    assets[4].mSize *= 2;
    ReportBlobState(blob, "Destroy Bush5:");


    // Destroy Bush2:
    objectID = assets[1].mCacheObjectID;
    TEST_CRITICAL(blob.Destroy(objectID));
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);
    assets[1].mCacheObjectID = INVALID16;
    assets[1].mSize -= 122720;
    ReportBlobState(blob, "Destroy Bush2:");

    for (SizeT i = 0; i < assets.Size(); ++i)
    {
        auto& asset = assets[i];
        if (Invalid(asset.mCacheObjectID))
        {
            asset.mCacheObjectID = blob.Reserve(asset.mID, asset.mSize);
            TEST_CRITICAL(Valid(asset.mCacheObjectID));
            TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);
        }
    }
    ReportBlobState(blob, "Reimported Assets:");

    // Defrag:
    CacheBlob defrag;
    defrag.Initialize(TArray<CacheObject>(), static_cast<UInt32>(blob.GetCapacity()));

    // Map < BlobID, DefragID >
    TArray<MockCacheDefrag> defragSteps;
    for (SizeT i = 0, size = blob.Size(); i < size; ++i)
    {
        CacheObject obj;
        TEST_CRITICAL(blob.GetObject(static_cast<CacheObjectId>(i), obj));

        if (Invalid(obj.mUID))
        {
            continue;
        }

        MockCacheDefrag step;
        step.mSource = static_cast<CacheObjectId>(i);
        step.mDest = static_cast<CacheObjectId>(defrag.Size());
        step.mAssetID = obj.mUID;
        defragSteps.Add(step);

        CacheObjectId id = defrag.Reserve(obj.mUID, obj.mSize);
        TEST_CRITICAL(Valid(id));
        TEST_CRITICAL(id == step.mDest);
    }

    TEST_CRITICAL(defrag.GetFragmentedBytes() == 0);
    TEST_CRITICAL(defrag.GetBytesUsed() == defrag.GetBytesReserved());
    TEST_CRITICAL(defrag.GetBytesUsed() < defrag.GetCapacity());
    TEST_CRITICAL(defrag.GetBytesReserved() < blob.GetBytesReserved());

    for (MockCacheDefrag& step : defragSteps)
    {
        auto iter = std::find_if(assets.begin(), assets.end(), [&step](const MockAssetData& asset) { return asset.mID == step.mAssetID; });
        TEST_CRITICAL(iter != assets.end());
        gTestLog.Debug(LogMessage("Defraging ") << iter->mName << " from " << step.mSource << " to " << step.mDest);
    }

    ReportBlobState(defrag, "After Defrag");
}

REGISTER_TEST(CacheBlob_FailReserveTest)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    
    CacheBlob blob;
    TEST_CRITICAL(Invalid(blob.Reserve(1, 450)));
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_OPERATION_BLOB_NOT_INITIALIZED);
    BUG_MESSAGE = NULL_MSG;

    blob.Initialize(TArray<CacheObject>(), 10 * MB);
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);

    TEST_CRITICAL(Invalid(blob.Reserve(INVALID32, 450)));
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_ARGUMENT_ASSET_ID);
    BUG_MESSAGE = NULL_MSG;

    TEST_CRITICAL(Invalid(blob.Reserve(1, 0)));
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_ARGUMENT_SIZE);
    BUG_MESSAGE = NULL_MSG;
    
    TEST_CRITICAL(Invalid(blob.Reserve(1, 10 * MB + 1)));
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);
}

REGISTER_TEST(CacheBlob_FailUpdateTest)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    CacheBlob blob;
    TEST_CRITICAL(blob.Update(1, 450) == false);
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_OPERATION_BLOB_NOT_INITIALIZED);
    BUG_MESSAGE = NULL_MSG;

    blob.Initialize(TArray<CacheObject>(), 10 * MB);
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);
    
    TEST_CRITICAL(blob.Update(INVALID16, 450) == false);
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_ARGUMENT_OBJECT_ID);
    BUG_MESSAGE = NULL_MSG;

    TEST_CRITICAL(blob.Update(0, 700) == false);
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_OPERATION_ASSOC_OBJECT_ID);
    BUG_MESSAGE = NULL_MSG;

    CacheObjectId id = blob.Reserve(2737, 450);
    TEST_CRITICAL(id == 0);
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);

    TEST_CRITICAL(blob.Destroy(id));
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);

    TEST_CRITICAL(blob.Update(id, 400) == false);
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_OPERATION_OBJECT_NULL);
    BUG_MESSAGE = NULL_MSG;

    id = blob.Reserve(1233, 300);
    TEST_CRITICAL(id == 0);
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);

    TEST_CRITICAL(blob.Update(id, 451) == false);
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);
}

REGISTER_TEST(CacheBlob_FailDestroyTest)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    CacheBlob blob;
    TEST_CRITICAL(blob.Destroy(0) == false);
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_OPERATION_BLOB_NOT_INITIALIZED);
    BUG_MESSAGE = NULL_MSG;

    blob.Initialize(TArray<CacheObject>(), 10 * MB);
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);

    TEST_CRITICAL(blob.Destroy(INVALID16) == false);
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_ARGUMENT_OBJECT_ID);
    BUG_MESSAGE = NULL_MSG;

    TEST_CRITICAL(blob.Destroy(0) == false);
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_OPERATION_ASSOC_OBJECT_ID);
    BUG_MESSAGE = NULL_MSG;

    CacheObjectId id = blob.Reserve(2737, 450);
    TEST_CRITICAL(id == 0);
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);

    TEST_CRITICAL(blob.Destroy(id));
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);

    TEST_CRITICAL(blob.Destroy(id) == false);
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_OPERATION_OBJECT_NULL);
    BUG_MESSAGE = NULL_MSG;
}

REGISTER_TEST(CacheBlob_FailGetObjectTest)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    CacheBlob blob;

    CacheObject obj;
    TEST_CRITICAL(blob.GetObject(0, obj));
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_OPERATION_BLOB_NOT_INITIALIZED);
    BUG_MESSAGE = NULL_MSG;

    blob.Initialize(TArray<CacheObject>(), 10 * MB);
    CacheObjectId id = blob.Reserve(1239, 450);
    TEST_CRITICAL(id == 0);
    TEST_CRITICAL(BUG_MESSAGE == NULL_MSG);
    
    TEST_CRITICAL(blob.GetObject(INVALID16, obj) == false);
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_ARGUMENT_OBJECT_ID);

    TEST_CRITICAL(blob.GetObject(1, obj) == false);
    TEST_CRITICAL(BUG_MESSAGE == CacheBlobError::ERROR_MSG_INVALID_OPERATION_ASSOC_OBJECT_ID);
}

REGISTER_TEST(CacheBlock_FailInitialize)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    CacheBlock block;
    TEST(block.GetName().Empty());
    TEST(block.GetDefaultCapacity() == 0);

    block.Initialize(Token(), KB);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_NAME);
    TEST(block.GetName().Empty());
    TEST(block.GetDefaultCapacity() == 0);
    BUG_MESSAGE = NULL_MSG;

    block.Initialize(Token("test"), 0);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_DEFAULT_CAPACITY);
    TEST(block.GetName().Empty());
    TEST(block.GetDefaultCapacity() == 0);
    BUG_MESSAGE = NULL_MSG;

    block.Initialize(Token("test"), KB);
    TEST(BUG_MESSAGE == NULL_MSG);
    TEST(block.GetName().Compare("test"));
    TEST(block.GetDefaultCapacity() == KB);

    block.Initialize(Token("test_fail"), KB);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_OPERATION_INITIALIZED);
    TEST(block.GetName().Compare("test"));
    TEST(block.GetDefaultCapacity() == KB);
}

REGISTER_TEST(CacheBlock_FailCreate)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    CacheBlock block;

    auto index = block.Create(INVALID32, 2 * KB);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_UID);
    BUG_MESSAGE = NULL_MSG;

    index = block.Create(0, 2 * KB);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_SIZE);
    BUG_MESSAGE = NULL_MSG;

    block.Initialize(Token("test"), KB);
    TEST(BUG_MESSAGE == NULL_MSG);

    index = block.Create(INVALID32, 2 * KB);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_UID);
    BUG_MESSAGE = NULL_MSG;

    index = block.Create(0, 2 * KB);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_SIZE);
    BUG_MESSAGE = NULL_MSG;

    index = block.Create(0, 0);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_SIZE);
    BUG_MESSAGE = NULL_MSG;

    // It is theoretically impossible to get ERROR_MSG_INVALID_OPERATION_INITIALIZATION_REQUIRED
    index = block.Create(0, 512);
    TEST(index == true);
    TEST(index.mUID == 0);
    TEST(index.mBlobID == 0);
    TEST(index.mObjectID == 0);
    TEST(BUG_MESSAGE == NULL_MSG);

    index = block.Create(0, 512);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_OPERATION_OBJECT_EXISTS);
    BUG_MESSAGE = NULL_MSG;
}

REGISTER_TEST(CacheBlock_FailUpdate)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    CacheBlock block;

    auto index = block.Update(CacheIndex(), 256);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_INDEX);
    BUG_MESSAGE = NULL_MSG;

    index = block.Update({ 0,0,0 }, 256);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_SIZE);
    BUG_MESSAGE = NULL_MSG;

    block.Initialize(Token("test"), KB);
    TEST(BUG_MESSAGE == NULL_MSG);

    index = block.Update(CacheIndex(), 256);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_INDEX);
    BUG_MESSAGE = NULL_MSG;

    index = block.Update({ 0,0,0 }, 2 * KB);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_SIZE);
    BUG_MESSAGE = NULL_MSG;

    index = block.Update({ 0,0,0 }, 0);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_SIZE);
    BUG_MESSAGE = NULL_MSG;

    index = block.Update({ 0,0,0 }, 256);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_INDEX);
    BUG_MESSAGE = NULL_MSG;

    TEST(block.Create(0, 512) == true);
    TEST(BUG_MESSAGE == NULL_MSG);

    index = block.Update(CacheIndex(5, 0, 0), 256);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_INDEX);
    BUG_MESSAGE = NULL_MSG;

    index = block.Update(CacheIndex(5, 0, 25), 256);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_INDEX);
    BUG_MESSAGE = NULL_MSG;

    index = block.Update(CacheIndex(0, 0, 0), 2 * KB);
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_SIZE);
    BUG_MESSAGE = NULL_MSG;
}

REGISTER_TEST(CacheBlock_FailDestroy)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    CacheBlock block;
    auto index = block.Destroy(CacheIndex());
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_INDEX);
    BUG_MESSAGE = NULL_MSG;

    index = block.Destroy(CacheIndex(0, 0, 0));
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_OPERATION_INITIALIZATION_REQUIRED);
    BUG_MESSAGE = NULL_MSG;

    block.Initialize(Token("test"), KB);
    index = block.Destroy(CacheIndex(0, 0, 0));
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_INDEX);
    BUG_MESSAGE = NULL_MSG;

    TEST(block.Create(0, 512) == true);
    TEST(BUG_MESSAGE == NULL_MSG);

    index = block.Destroy(CacheIndex(0, 0, 1));
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_INDEX);
    BUG_MESSAGE = NULL_MSG;

    index = block.Destroy(CacheIndex(1, 0, 0));
    TEST(index == false);
    TEST(BUG_MESSAGE == CacheBlockError::ERROR_MSG_INVALID_ARGUMENT_INDEX);
    BUG_MESSAGE = NULL_MSG;

    TEST(block.Destroy(CacheIndex(0, 0, 0)) == true);
}

REGISTER_TEST(CacheBlock_Test)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    CacheBlock block;
    block.Initialize(Token("gb"), 8 * KB);
    TEST_CRITICAL(block.GetDefaultCapacity() == 8 * KB);
    TArray<CacheIndex> indices;

    // OP | UID |   BlobID   | ObjectID |  Size    | Blob_0 Memory | Blob_1 Memory | Blob_2 Memory
    //  C |   0 |        0   |        0 |     2 KB |          6 KB | ------------- | -------------
    //  C |   1 |        0   |        1 |     3 KB |          3 KB | ------------- | -------------
    //  C |   2 |        0   |        2 |     2 KB |          1 KB | ------------- | -------------
    //  C |   3 |        1   |        0 |     4 KB |          1 KB |          4 KB | -------------
    //  C |   4 |        0   |        3 |   256  B |        768  B |          4 KB | -------------
    //  C |   5 |        0   |        4 |   767  B |          1  B |          4 KB | -------------
    //  C |   6 |        1   |        1 |     2 KB |          1  B |          2 KB | -------------
    //  C |   7 |        2   |        0 |  2049  B |          1  B |          2 KB |       6143 B
    //  C |   8 |        1   |        2 |  2000  B |          1  B |         48  B |       6143 B
    auto index = block.Create(0, 2 * KB);
    TEST(index == true);
    TEST(index.mUID == 0);
    TEST(index.mBlobID == 0);
    TEST(index.mObjectID == 0);
    indices.Add(index);

    index = block.Create(1, 3 * KB);
    TEST(index == true);
    TEST(index.mUID == 1);
    TEST(index.mBlobID == 0);
    TEST(index.mObjectID == 1);
    indices.Add(index);

    index = block.Create(2, 2 * KB);
    TEST(index == true);
    TEST(index.mUID == 2);
    TEST(index.mBlobID == 0);
    TEST(index.mObjectID == 2);
    indices.Add(index);

    index = block.Create(3, 4 * KB);
    TEST(index == true);
    TEST(index.mUID == 3);
    TEST(index.mBlobID == 1);
    TEST(index.mObjectID == 0);
    indices.Add(index);

    index = block.Create(4, 256);
    TEST(index == true);
    TEST(index.mUID == 4);
    TEST(index.mBlobID == 0);
    TEST(index.mObjectID == 3);
    indices.Add(index);

    index = block.Create(5, 767);
    TEST(index == true);
    TEST(index.mUID == 5);
    TEST(index.mBlobID == 0);
    TEST(index.mObjectID == 4);
    indices.Add(index);

    index = block.Create(6, 2 * KB);
    TEST(index == true);
    TEST(index.mUID == 6);
    TEST(index.mBlobID == 1);
    TEST(index.mObjectID == 1);
    indices.Add(index);

    index = block.Create(7, 2049);
    TEST(index == true);
    TEST(index.mUID == 7);
    TEST(index.mBlobID == 2);
    TEST(index.mObjectID == 0);
    indices.Add(index);

    index = block.Create(8, 2000);
    TEST(index == true);
    TEST(index.mUID == 8);
    TEST(index.mBlobID == 1);
    TEST(index.mObjectID == 2);
    indices.Add(index);
    
    // Blob States: ( UID : ObjectID, Size )
    // [Blob 0:    1 B] -- { 0 : 0, 2KB }, { 1 : 1, 3KB }, { 2 : 2, 2KB }, { 4 : 3, 256B }, { 5 : 4, 767B }
    // [Blob 1:   48 B] -- { 3 : 0, 4KB }, { 6 : 1, 2KB }, { 8 : 2, 2000B }
    // [Blob 2: 6143 B] -- { 7 : 0, 2049B }
    auto stat0 = block.GetBlobStat(0);
    auto stat1 = block.GetBlobStat(1);
    auto stat2 = block.GetBlobStat(2);

    TEST((stat0.mBlobCapacity - stat0.mBytesUsed) == 1);
    TEST((stat1.mBlobCapacity - stat1.mBytesUsed) == 48);
    TEST((stat2.mBlobCapacity - stat2.mBytesUsed) == 6143);

    // Delete WHERE UID = 2
    // [Blob 0: 2049 B] -- { 0 : 0, 2KB }, { 1 : 1, 3KB }, { NULL : 2, 2KB }, { 4 : 3, 256B }, { 5 : 4, 767B }
    // [Blob 1:   48 B] -- { 3 : 0, 4KB }, { 6 : 1, 2KB }, { 8 : 2, 2000B }
    // [Blob 2: 6143 B] -- { 7 : 0, 2049B }
    index = block.Destroy(indices[2]);
    TEST(index == true);
    indices[2].mBlobID = INVALID32; indices[2].mObjectID = INVALID32;
    stat0 = block.GetBlobStat(0);
    stat1 = block.GetBlobStat(1);
    stat2 = block.GetBlobStat(2);
    TEST((stat0.mBlobCapacity - stat0.mBytesUsed) == 2049);
    TEST((stat1.mBlobCapacity - stat1.mBytesUsed) == 48);
    TEST((stat2.mBlobCapacity - stat2.mBytesUsed) == 6143);

    // Delete WHERE UID = 5
    // [Blob 0: 2816 B] -- { 0 : 0, 2KB }, { 1 : 1, 3KB }, { NULL : 2, 2KB }, { 4 : 3, 256B }, { NULL : 4, 767B }
    // [Blob 1:   48 B] -- { 3 : 0, 4KB }, { 6 : 1, 2KB }, { 8 : 2, 2000B }
    // [Blob 2: 6143 B] -- { 7 : 0, 2049B }
    index = block.Destroy(indices[5]);
    TEST(index == true);
    indices[5].mBlobID = INVALID32; indices[5].mObjectID = INVALID32;
    stat0 = block.GetBlobStat(0);
    stat1 = block.GetBlobStat(1);
    stat2 = block.GetBlobStat(2);
    TEST((stat0.mBlobCapacity - stat0.mBytesUsed) == 2816);
    TEST((stat1.mBlobCapacity - stat1.mBytesUsed) == 48);
    TEST((stat2.mBlobCapacity - stat2.mBytesUsed) == 6143);

    // Delete WHERE UID = 6
    // [Blob 0: 2816 B] -- { 0 : 0, 2KB }, { 1 : 1, 3KB }, { NULL : 2, 2KB }, { 4 : 3, 256B }, { NULL : 4, 767B }
    // [Blob 1: 2096 B] -- { 3 : 0, 4KB }, { NULL : 1, 2KB }, { 8 : 2, 2000B }
    // [Blob 2: 6143 B] -- { 7 : 0, 2049B }
    index = block.Destroy(indices[6]);
    TEST(index == true);
    indices[6].mBlobID = INVALID32; indices[6].mObjectID = INVALID32;
    stat0 = block.GetBlobStat(0);
    stat1 = block.GetBlobStat(1);
    stat2 = block.GetBlobStat(2);
    TEST((stat0.mBlobCapacity - stat0.mBytesUsed) == 2816);
    TEST((stat1.mBlobCapacity - stat1.mBytesUsed) == 2096);
    TEST((stat2.mBlobCapacity - stat2.mBytesUsed) == 6143);

    // Update WHERE UID = 1 
    // [Blob 0: 5888 B] -- { 0 : 0, 2KB }, { NULL : 1, 3KB }, { NULL : 2, 2KB }, { 4 : 3, 256B }, { NULL : 4, 767B }
    // [Blob 1: 2096 B] -- { 3 : 0, 4KB }, { NULL : 1, 2KB }, { 8 : 2, 2000B }
    // [Blob 2: 3070 B] -- { 7 : 0, 2049B }, { 1 : 1, 3073 }
    index = block.Update(indices[1], 3073);
    TEST(index == true);
    TEST(index.mUID == 1);
    TEST(index.mBlobID == 2);
    TEST(index.mObjectID == 1);
    indices[1] = index;
    stat0 = block.GetBlobStat(0);
    stat1 = block.GetBlobStat(1);
    stat2 = block.GetBlobStat(2);
    TEST((stat0.mBlobCapacity - stat0.mBytesUsed) == 5888);
    TEST((stat1.mBlobCapacity - stat1.mBytesUsed) == 2096);
    TEST((stat2.mBlobCapacity - stat2.mBytesUsed) == 3070);

    // Update WHERE UID = 8 2049 
    // [Blob 0: 3839 B] -- { 0 : 0, 2KB }, { 8 : 1, 3KB }, { NULL : 2, 2KB }, { 4 : 3, 256B }, { NULL : 4, 767B }
    // [Blob 1: 4096 B] -- { 3 : 0, 4KB }, { NULL : 1, 2KB }, { NULL : 2, 2000B }
    // [Blob 2: 3070 B] -- { 7 : 0, 2049B }, { 1 : 1, 3073 }
    index = block.Update(indices[8], 2050);
    TEST(index == true);
    TEST(index.mUID == 8);
    TEST(index.mBlobID == 0);
    TEST(index.mObjectID == 1);
    indices[8] = index;
    stat0 = block.GetBlobStat(0);
    stat1 = block.GetBlobStat(1);
    stat2 = block.GetBlobStat(2);
    TEST((stat0.mBlobCapacity - stat0.mBytesUsed) == 3838);
    TEST((stat1.mBlobCapacity - stat1.mBytesUsed) == 4096);
    TEST((stat2.mBlobCapacity - stat2.mBytesUsed) == 3070);

    // CREATE UID = 6 1024 
    // [Blob 0: 2815 B] -- { 0 : 0, 2KB }, { 8 : 1, 3KB }, { 6 : 2, 1KB/2KB }, { 4 : 3, 256B }, { NULL : 4, 767B }
    // [Blob 1: 4096 B] -- { 3 : 0, 4KB }, { NULL : 1, 2KB }, { NULL : 2, 2000B }
    // [Blob 2: 3070 B] -- { 7 : 0, 2049B }, { 1 : 1, 3073 }
    index = block.Create(indices[6].mUID, 1024);
    TEST(index == true);
    TEST(index.mUID == 6);
    TEST(index.mBlobID == 0);
    TEST(index.mObjectID == 2);
    indices[6] = index;
    stat0 = block.GetBlobStat(0);
    stat1 = block.GetBlobStat(1);
    stat2 = block.GetBlobStat(2);
    TEST((stat0.mBlobCapacity - stat0.mBytesUsed) == 2814);
    TEST((stat1.mBlobCapacity - stat1.mBytesUsed) == 4096);
    TEST((stat2.mBlobCapacity - stat2.mBytesUsed) == 3070);

    // UPDATE UID = 6 2048
    // [Blob 0: 1791 B] -- { 0 : 0, 2KB }, { 8 : 1, 3KB }, { 6 : 2, 2KB }, { 4 : 3, 256B }, { NULL : 4, 767B }
    // [Blob 1: 4096 B] -- { 3 : 0, 4KB }, { NULL : 1, 2KB }, { NULL : 2, 2000B }
    // [Blob 2: 3070 B] -- { 7 : 0, 2049B }, { 1 : 1, 3073 }
    index = block.Update(indices[6], 2048);
    TEST(index == true);
    TEST(index.mUID == 6);
    TEST(index.mBlobID == 0);
    TEST(index.mObjectID == 2);
    indices[6] = index;
    stat0 = block.GetBlobStat(0);
    stat1 = block.GetBlobStat(1);
    stat2 = block.GetBlobStat(2);
    TEST((stat0.mBlobCapacity - stat0.mBytesUsed) == 1790);
    TEST((stat1.mBlobCapacity - stat1.mBytesUsed) == 4096);
    TEST((stat2.mBlobCapacity - stat2.mBytesUsed) == 3070);

    // UPDATE UID = 6 3000
    // [Blob 0: 1791 B] -- { 0 : 0, 2KB }, { 8 : 1, 3KB }, { NULL : 2, 2KB }, { 4 : 3, 256B }, { NULL : 4, 767B }
    // [Blob 1: 4096 B] -- { 3 : 0, 4KB }, { NULL : 1, 2KB }, { NULL : 2, 2000B }
    // [Blob 2: 3070 B] -- { 7 : 0, 2049B }, { 1 : 1, 3073 }, { 6 : 2, 3073 B }
    index = block.Update(indices[6], 3000);
    TEST(index == true);
    TEST(index.mUID == 6);
    TEST(index.mBlobID == 2);
    TEST(index.mObjectID == 2);
    indices[6] = index;
    stat0 = block.GetBlobStat(0);
    stat1 = block.GetBlobStat(1);
    stat2 = block.GetBlobStat(2);
    TEST((stat0.mBlobCapacity - stat0.mBytesUsed) == 3838);
    TEST((stat1.mBlobCapacity - stat1.mBytesUsed) == 4096);
    TEST((stat2.mBlobCapacity - stat2.mBytesUsed) == 70);

    TEST(stat0.mNumObjectsFragmented == 2);
    TEST(stat1.mNumObjectsFragmented == 2);
    TEST(stat2.mNumObjectsFragmented == 0);

    auto defragSteps = block.GetDefragSteps();
    (defragSteps);

    for (auto& step : defragSteps)
    {
        gTestLog.Debug
        (
            LogMessage("") << step.mUID << ": " << StreamFillRight(4) << step.mSize << StreamFillRight() 
            << " [" << step.mSourceBlobID << "," << step.mSourceObjectID << "] => [" 
            << step.mDestBlobID << "," << step.mDestObjectID << "]"
        );
    }

    TEST(defragSteps.Size() == 7);
    TEST(defragSteps[0].mUID == 3);
    TEST(defragSteps[0].mSize == 4096);
    TEST(defragSteps[0].mSourceBlobID == 1);
    TEST(defragSteps[0].mSourceObjectID == 0);
    TEST(defragSteps[0].mDestBlobID == 0);
    TEST(defragSteps[0].mDestObjectID == 0);

    TEST(defragSteps[1].mUID == 1);
    TEST(defragSteps[1].mSize == 3073);
    TEST(defragSteps[1].mSourceBlobID == 2);
    TEST(defragSteps[1].mSourceObjectID == 1);
    TEST(defragSteps[1].mDestBlobID == 0);
    TEST(defragSteps[1].mDestObjectID == 1);

    TEST(defragSteps[2].mUID == 4);
    TEST(defragSteps[2].mSize == 256);
    TEST(defragSteps[2].mSourceBlobID == 0);
    TEST(defragSteps[2].mSourceObjectID == 3);
    TEST(defragSteps[2].mDestBlobID == 0);
    TEST(defragSteps[2].mDestObjectID == 2);

    TEST(defragSteps[3].mUID == 6);
    TEST(defragSteps[3].mSize == 3000);
    TEST(defragSteps[3].mSourceBlobID == 2);
    TEST(defragSteps[3].mSourceObjectID == 2);
    TEST(defragSteps[3].mDestBlobID == 1);
    TEST(defragSteps[3].mDestObjectID == 0);

    TEST(defragSteps[4].mUID == 8);
    TEST(defragSteps[4].mSize == 2050);
    TEST(defragSteps[4].mSourceBlobID == 0);
    TEST(defragSteps[4].mSourceObjectID == 1);
    TEST(defragSteps[4].mDestBlobID == 1);
    TEST(defragSteps[4].mDestObjectID == 1);

    TEST(defragSteps[5].mUID == 7);
    TEST(defragSteps[5].mSize == 2049);
    TEST(defragSteps[5].mSourceBlobID == 2);
    TEST(defragSteps[5].mSourceObjectID == 0);
    TEST(defragSteps[5].mDestBlobID == 1);
    TEST(defragSteps[5].mDestObjectID == 2);

    TEST(defragSteps[6].mUID == 0);
    TEST(defragSteps[6].mSize == 2048);
    TEST(defragSteps[6].mSourceBlobID == 0);
    TEST(defragSteps[6].mSourceObjectID == 0);
    TEST(defragSteps[6].mDestBlobID == 2);
    TEST(defragSteps[6].mDestObjectID == 0);

    // todo: These defrag steps didnt work right?
    // todo: apply defrag

    LF_DEBUG_BREAK;
}

String CacheWriterSetup()
{
    // Configure our test output directory
    String testPath = FileSystem::PathResolve(FileSystem::PathJoin(FileSystem::GetWorkingPath(), "../Temp/TestOutput"));
    if (FileSystem::PathCreate(testPath))
    {
        gTestLog.Debug(LogMessage("Creating test path'") << testPath << "'");
    }
    return testPath + "test_cache";
}

void CacheWriterCleanup()
{
    
}

REGISTER_TEST(CacheWriter_WriteTest)
{
    const String testBlock = CacheWriterSetup();
    const String message = "Test content as a string.";


    CacheBlock block;
    block.Initialize(Token("test_cache"), 8 * KB);
    TEST_CRITICAL(block.GetDefaultCapacity() == 8 * KB);
    block.SetFilename(Token(testBlock));
    CacheIndex index = block.Create(0, 1 * KB);
    TEST_CRITICAL(index);

    {
        CacheWriter cw(block, index, message.CStr(), message.Size());
        String fullPath(cw.GetOutputFilename().CStr(), COPY_ON_WRITE);
        TEST_CRITICAL(Valid(fullPath.Find(testBlock)));
        TEST_CRITICAL(FileSystem::FileReserve(fullPath, static_cast<FileSize>(block.GetDefaultCapacity())));
        File f;
        f.Open(fullPath, FF_READ | FF_SHARE_READ | FF_SHARE_WRITE, FILE_OPEN_EXISTING);
        TEST_CRITICAL(f.IsOpen() && f.GetSize() == static_cast<FileSize>(block.GetDefaultCapacity()));
        f.Close();
        TEST(cw.Write());
        f.Open(fullPath, FF_READ | FF_SHARE_READ | FF_SHARE_WRITE, FILE_OPEN_EXISTING);
        TEST_CRITICAL(f.IsOpen() && f.GetSize() == static_cast<FileSize>(block.GetDefaultCapacity()));
        f.Close();
    }
}

REGISTER_TEST(CacheWriter_WriteAsyncTest)
{
    const String testBlock = CacheWriterSetup();
    const String message = "Test content as a string.";


    CacheBlock block;
    block.Initialize(Token("test_cache"), 8 * KB);
    TEST_CRITICAL(block.GetDefaultCapacity() == 8 * KB);
    block.SetFilename(Token(testBlock));
    CacheIndex index = block.Create(0, 1 * KB);
    TEST_CRITICAL(index);

    {
        CacheWriter cw(block, index, message.CStr(), message.Size());
        String fullPath(cw.GetOutputFilename().CStr(), COPY_ON_WRITE);
        TEST_CRITICAL(Valid(fullPath.Find(testBlock)));
        TEST_CRITICAL(FileSystem::FileReserve(fullPath, static_cast<FileSize>(block.GetDefaultCapacity())));
        File f;
        f.Open(fullPath, FF_READ | FF_SHARE_READ | FF_SHARE_WRITE, FILE_OPEN_EXISTING);
        TEST_CRITICAL(f.IsOpen() && f.GetSize() == static_cast<FileSize>(block.GetDefaultCapacity()));
        f.Close();

        // Promise = using CacheWritePromise = PromiseImpl<TCallback<void>, TCallback<void, const String&>>;
        bool writeDone = false;
        auto promise = cw.WriteAsync()
            .Then([&writeDone]()
                {
                    gTestLog.Info(LogMessage("Success!"));
                    writeDone = true;
                }
            )
            .Catch([](const String&)
                {
                    TEST(false);
                }
            )
            .Execute(); // I refuse to do function argument binding, so to ensure all 
                        // 'Then' and 'Catch' callbacks are invoked the user must call 
                        // Execute to actually run the promise.

        SleepCallingThread(2000); // Pretend like were doing something else...
        promise->Wait(); // Ensure the promise is completed, it should be.. we gave it 2 seconds
        TEST_CRITICAL(writeDone);
        f.Open(fullPath, FF_READ | FF_SHARE_READ | FF_SHARE_WRITE, FILE_OPEN_EXISTING);
        TEST_CRITICAL(f.IsOpen() && f.GetSize() == static_cast<FileSize>(block.GetDefaultCapacity()));
        f.Close();
    }
}

REGISTER_TEST(CacheBlock_TestEx)
{
    

    CacheBlock block;
    block.Initialize(Token("gb"), 1 * MB);

    TArray<CacheIndex> indices;

    TArray<StubAssetTypeData> types;
    PopulateSampleAssets(types);
    
    const Type* categoryTypes[AssetCategory::MAX_VALUE];
    PopulateAssetCategories(categoryTypes);

    for(SizeT i = 0; i < types.Size(); ++i)
    {
        const AssetTypeData& type = types[i].mTypeData;
        CacheIndex index = block.Create(type.mUID, 204923);
        TEST(index);
        indices.Add(index);
    }

    TArray<AssetTypeData> assetTypeDatas;
    for (const StubAssetTypeData& type : types)
    {
        assetTypeDatas.Add(type.mTypeData);
    }
    AssetDataController dataController;
    dataController.Initialize(assetTypeDatas, categoryTypes);

    for (const CacheIndex& index : indices)
    {
        const AssetType* assetType = dataController.FindByUID(index.mUID);
        if (assetType)
        {
            String blockName(block.GetName().CStr());
            blockName.Append('_');
            blockName.Append(ByteToHex((index.mBlobID >> 4) & 0x0F));
            blockName.Append(ByteToHex(index.mBlobID & 0x0F));
            blockName.Append(".lfcache");
            blockName.Append(':');
            blockName.Append(ByteToHex((index.mObjectID >> 4) & 0x0F));
            blockName.Append(ByteToHex(index.mObjectID & 0x0F));
            gTestLog.Debug(LogMessage("Cache asset ") << assetType->mFullName << "..." << blockName);
        }
        else
        {
            gTestLog.Warning(LogMessage("Failed to create asset in cache. UID=") << index.mUID);
        }
    }
}

REGISTER_TEST(CacheController_Test)
{
    gReportBugCallback = TestBugReporter;
    BUG_MESSAGE = NULL_MSG;

    TArray<StubAssetTypeData> types;
    PopulateSampleAssets(types);
    const Type* categoryTypes[AssetCategory::MAX_VALUE];
    PopulateAssetCategories(categoryTypes);

    SizeT nameFill = 0;

    AssetDataController data;
    {
        TArray<AssetTypeData> assetTypes;
        for (const auto& type : types)
        {
            assetTypes.Add(type.mTypeData);
            nameFill = Max(type.mTypeData.mFullName.Size(), nameFill);
        }
        data.Initialize(assetTypes, categoryTypes);
    }


    AssetCacheController cache;
    for (const auto& type : types)
    {
        Token cacheName("gb" + type.mCacheName);
        if (Invalid(cache.FindCacheBlockIndex(cacheName)))
        {
            gTestLog.Debug(LogMessage("Creating cache ") << cacheName << "...");
            TEST_CRITICAL(cache.CreateBlock(cacheName));
        }

        CacheBlockIndex blockIndex = cache.FindCacheBlockIndex(cacheName);
        // gTestLog.Debug(LogMessage("Caching asset ") << type.mTypeData.mFullName << " in " << )
        CacheIndex index = cache.Create(blockIndex, type.mTypeData.mUID, type.mSize);
        if (index)
        {
            TEST(BUG_MESSAGE == NULL_MSG);
            String fullCacheName(cacheName.CStr());
            fullCacheName.Append('_');
            fullCacheName.Append(ToHexString(index.mBlobID));
            fullCacheName.Append(".lfcache:");
            fullCacheName.Append(ToString(index.mObjectID));

            // auto assetType = data.FindByUID(type.mTypeData.mUID);
            // assetType->mCacheFileID = index.mBlobID;
            // assetType->mCacheObjectIndex = index.mIndex;

            gTestLog.Debug(LogMessage("Cached asset ")
                << "[" << StreamFillRight(4) << type.mTypeData.mUID << StreamFillRight() << "]" << StreamFillRight(nameFill) << type.mTypeData.mFullName
                << StreamFillRight() << " " << fullCacheName
            );

        }
        else
        {
            gTestLog.Error(LogMessage("Failed to cache asset ")
                << "[" << StreamFillRight(4) << type.mTypeData.mUID << StreamFillRight() << "]" << StreamFillRight(nameFill) << type.mTypeData.mFullName
                << StreamFillRight() << " Block=" << cacheName << ", BlockIndex=" << blockIndex << ", Asset Size=" << type.mSize);
            TEST(false);
        }
        BUG_MESSAGE = NULL_MSG;
    }

    auto GetPercent = [](SizeT num, SizeT denom) { return 100.0 * (static_cast<double>(num) / static_cast<double>(denom)); };
    auto stats = cache.GetBlobStats();
    for (const auto& stat : stats)
    {
        gTestLog.Debug(LogMessage("") << stat.mCacheBlock << "[" << stat.mBlobID << "] " 
            << stat.mBytesReserved << "/" << stat.mBlobCapacity 
            << " (" << StreamPrecision(2) << GetPercent(stat.mBytesReserved, stat.mBlobCapacity) << "%)");
    }


    MemoryBuffer buffer;
    String text;
    StubFillCacheData(buffer, text);

    // todo:
    // cache.Load(buffer);
    // Test for types:


    gTestLog.Debug(LogMessage("AssetCacheHeaders Binary=") << buffer.GetSize() << ", Text=" << text.Size());
    gTestLog.Debug(LogMessage("\n") << text);

    // auto assetCacheBlockIndex = cache.FindCacheBlockIndex(Token("gb_t"));
    // auto assetCacheIndex = cache.Find(assetCacheBlockIndex, uid);
}

REGISTER_TEST(CacheStreamTest)
{
    auto config = TestFramework::GetConfig();
    TestFramework::ExecuteTest("CacheBlob_FailReserveTest", config);
    TestFramework::ExecuteTest("CacheBlob_FailUpdateTest", config);
    TestFramework::ExecuteTest("CacheBlob_FailDestroyTest", config);
    TestFramework::ExecuteTest("CacheBlob_FailGetObjectTest", config);
    TestFramework::ExecuteTest("CacheBlob_FragmentationTest", config);
    TestFramework::ExecuteTest("CacheBlock_FailInitialize", config);
    TestFramework::ExecuteTest("CacheBlock_FailCreate", config);
    TestFramework::ExecuteTest("CacheBlock_FailUpdate", config);
    TestFramework::ExecuteTest("CacheBlock_FailDestroy", config);
    TestFramework::ExecuteTest("CacheBlock_Test", config);
    TestFramework::ExecuteTest("CacheWriter_WriteTest", config);
    TestFramework::ExecuteTest("CacheWriter_WriteAsyncTest", config);
    // TestFramework::ExecuteTest("CacheController_Test", config);
    TestFramework::TestReset();

    // String workingDir = FileSystem::GetWorkingPath();
    // String assetName = "/User/Textures/Bush 1.png";
    // String assetFile = AssetNameToFilePath(assetName);
    // gTestLog.Debug(LogMessage("Working Dir=") << workingDir);
    // gTestLog.Debug(LogMessage("AssetName: ") << assetName);
    // gTestLog.Debug(LogMessage("AssetFile: ") << assetFile);
    // 
    // AssetExporter exporter;
    // exporter.Export(assetName);

}

struct MyAssetIndexTraits
{
    using Base = TAssetIndexTraits<const char*, UInt32>;
    using Compare = typename Base::Compare;

    static Base::KeyType DefaultKey() { return ""; }
    static Base::IndexType DefaultIndex() { return INVALID32; }
};

REGISTER_TEST(AssetIndexTest)
{
    using MyAssetPairIndex = TAssetPairIndex<const char*, UInt32>;
    using MyAssetIndex = TAssetIndex<const char*, UInt32, MyAssetIndexTraits>;

    MyAssetPairIndex builder;
    builder.Add(std::make_pair("/user/characters/markus/textures/face.png", 0));
    builder.Add(std::make_pair("/user/characters/markus/textures/face.png.lfpkg", 1));
    builder.Add(std::make_pair("/user/characters/markus/models/head.fbx", 2));
    builder.Add(std::make_pair("/user/characters/markus/models/head.fbx.lfpkg", 3));
    builder.Add(std::make_pair("/user/characters/markus/models/body.fbx", 4));
    builder.Add(std::make_pair("/user/characters/markus/models/body.fbx.lfpkg", 5));
    builder.Add(std::make_pair("/user/characters/markus/voice/dialog00.wav", 6));
    builder.Add(std::make_pair("/user/characters/markus/voice/dialog00.wav.lfpkg", 7));
    builder.Add(std::make_pair("/user/characters/markus/scripts/markus.lua", 8));
    builder.Add(std::make_pair("/user/characters/markus/scripts/markus.lua.lfpkg", 9));
    builder.Add(std::make_pair("/user/characters/markus/markus.lfpkg", 10));

    std::sort(builder.begin(), builder.end());
    
    MyAssetIndex index;
    index.Build(builder);
    TEST(index.Find("/user/characters/markus/textures/face.png") == 0);
    TEST(index.Find("/user/characters/markus/textures/face.png.lfpkg") == 1);
    TEST(index.Find("/user/characters/markus/models/head.fbx") == 2);
    TEST(index.Find("/user/characters/markus/models/head.fbx.lfpkg") == 3);
    TEST(index.Find("/user/characters/markus/models/body.fbx") == 4);
    TEST(index.Find("/user/characters/markus/models/body.fbx.lfpkg") == 5);
    TEST(index.Find("/user/characters/markus/voice/dialog00.wav") == 6);
    TEST(index.Find("/user/characters/markus/voice/dialog00.wav.lfpkg") == 7);
    TEST(index.Find("/user/characters/markus/scripts/markus.lua") == 8);
    TEST(index.Find("/user/characters/markus/scripts/markus.lua.lfpkg") == 9);
    TEST(index.Find("/user/characters/markus/markus.lfpkg") == 10);

    SizeT footprint = index.QueryFootprint(
            [](const char* const& key) { return strlen(key); }, 
            [](const UInt32&) { return SizeT(0); }
        );

    gTestLog.Debug(LogMessage("Asset Index Footprint=") << footprint);
}

REGISTER_TEST(AssetHashTest)
{
    AssetHash hash;
    TEST_CRITICAL(hash.IsZero());

    TEST_CRITICAL(hash.Parse("") == false);
    TEST_CRITICAL(hash.IsZero());
    TEST_CRITICAL(hash.Parse("1020301010DA") == false);
    TEST_CRITICAL(hash.IsZero());
    TEST_CRITICAL(hash.Parse("A2A28228232828283282282328328Z88") == false);
    TEST_CRITICAL(hash.IsZero());
    TEST_CRITICAL(hash.Parse("A2A28228232828283282282328328C88") == true);
    TEST_CRITICAL(hash.IsZero() == false);

    TEST_CRITICAL(hash.mData[0] == 0xA2);
    TEST_CRITICAL(hash.mData[1] == 0xA2);
    TEST_CRITICAL(hash.mData[2] == 0x82);
    TEST_CRITICAL(hash.mData[3] == 0x28);
    TEST_CRITICAL(hash.mData[4] == 0x23);
    TEST_CRITICAL(hash.mData[5] == 0x28);
    TEST_CRITICAL(hash.mData[6] == 0x28);
    TEST_CRITICAL(hash.mData[7] == 0x28);
    TEST_CRITICAL(hash.mData[8] == 0x32);
    TEST_CRITICAL(hash.mData[9] == 0x82);
    TEST_CRITICAL(hash.mData[10] == 0x28);
    TEST_CRITICAL(hash.mData[11] == 0x23);
    TEST_CRITICAL(hash.mData[12] == 0x28);
    TEST_CRITICAL(hash.mData[13] == 0x32);
    TEST_CRITICAL(hash.mData[14] == 0x8C);
    TEST_CRITICAL(hash.mData[15] == 0x88);

    hash.SetZero();
    TEST_CRITICAL(hash.IsZero());
}





REGISTER_TEST(AssetDataController_InitializeTest)
{
    // todo: Make 'Stubs' for asset types.
    // todo: Make 'Stub' asset data

    const Type* categoryTypes[AssetCategory::MAX_VALUE];
    PopulateAssetCategories(categoryTypes);
    
    TArray<StubAssetTypeData> types;
    PopulateSampleAssets(types);
    TArray<AssetTypeData> assetTypeDatas;
    for (const StubAssetTypeData& type : types)
    {
        assetTypeDatas.Add(type.mTypeData);
    }
    
    AssetDataController dataController;
    dataController.Initialize(assetTypeDatas, categoryTypes);
    
    gTestLog.Debug(LogMessage("DataController__StaticTypes=") << dataController.StaticSize());
    gTestLog.Debug(LogMessage("DataController__Footprint=") << dataController.GetStaticFootprint());
    LF_DEBUG_BREAK;

    
}

REGISTER_TEST(AssetTest)
{
    auto config = TestFramework::GetConfig();
    TestFramework::ExecuteTest("AssetIndexTest", config);
    TestFramework::ExecuteTest("AssetHashTest", config);

    TestFramework::ExecuteTest("CacheBlob_FailReserveTest", config);
    TestFramework::ExecuteTest("CacheBlob_FailUpdateTest", config);
    TestFramework::ExecuteTest("CacheBlob_FailDestroyTest", config);
    TestFramework::ExecuteTest("CacheBlob_FailGetObjectTest", config);
    TestFramework::ExecuteTest("CacheBlob_FragmentationTest", config);



    // TestFramework::ExecuteTest("AssetDataController_InitializeTest", config);
    TestFramework::TestReset();
}

REGISTER_TEST(AssetExporterTest)
{
    AssetTypeData data;
    data.mFullName = Token("/user/characters/markus/textures/face.png");
    data.mConcreteType = typeof(StubAssetTexture)->GetFullName();
    data.mCacheName = Token("gb_t");
    data.mUID = 0;
    data.mParentUID = INVALID32;
    data.mVersion = 0;
    data.mAttributes = 0;
    data.mFlags = (1 << AssetFlags::AF_BINARY);
    data.mCategory = AssetCategory::AC_TEXTURE;
    UInt8 hash[] = { 0xFF, 0xDB, 0xA1, 0x23, 0x44, 0x7F, 0x05, 0x0C, 0xD4, 0x74, 0xCC, 0xAD, 0xFF, 0xDB, 0xA1, 0x23 };
    memcpy(data.mHash.mData, hash, sizeof(hash));

    String text;
    TextStream ts;
    ts.Open(Stream::TEXT, &text, Stream::SM_WRITE);
    ts.BeginObject(data.mFullName.CStr(), data.mConcreteType.CStr());
    data.Serialize(ts);
    ts.EndObject();
    ts.Close();

    gTestLog.Debug(LogMessage("\n") << text);

    AssetTypeData other;
    ts.Open(Stream::TEXT, &text, Stream::SM_READ);
    ts.BeginObject(ts.GetObjectName(0), ts.GetObjectSuper(0));
    other.Serialize(ts);
    ts.EndObject();

    other.mFullName = Token(ts.GetObjectName(0));
    other.mConcreteType = Token(ts.GetObjectSuper(0));
    ts.Close();
    LF_DEBUG_BREAK;

    // AssetExporter exporter;
    // exporter.AddBundle(AssetBundleExportName("GameBase", "gb"));
    // 
    // AssetExportPackage markus;
    // markus.mTag = "";
    // markus.mBundle = "GameBase";
    // markus.mAssets.Add("/user/characters/markus/textures/face.png");
    // markus.mAssets.Add("/user/characters/markus/textures/face.png.lfpkg");
    // markus.mAssets.Add("/user/characters/markus/models/head.fbx");
    // markus.mAssets.Add("/user/characters/markus/models/head.fbx.lfpkg");
    // markus.mAssets.Add("/user/characters/markus/models/body.fbx");
    // markus.mAssets.Add("/user/characters/markus/models/body.fbx.lfpkg");
    // markus.mAssets.Add("/user/characters/markus/voice/dialog00.wav");
    // markus.mAssets.Add("/user/characters/markus/voice/dialog00.wav.lfpkg");
    // markus.mAssets.Add("/user/characters/markus/scripts/markus.lua");
    // markus.mAssets.Add("/user/characters/markus/scripts/markus.lua.lfpkg");
    // markus.mAssets.Add("/user/characters/markus/markus.lfpkg");
    // markus.mBlacklist.Add("/user/characters/markus/scripts/markus.lua");
    // markus.mBlacklist.Add("/user/characters/markus/markus.lfpkg");
    // 
    // exporter.AddPackage(markus);
    // 
    // AssetExportManifest manifest = exporter.CreateManifest();
    // 
    // for (auto& exportInfo : manifest.mExports)
    // {
    //     gTestLog.Debug(LogMessage("Exporting ") << exportInfo.mAssetName << "... " << exportInfo.mCacheFile);
    // }


}



}


