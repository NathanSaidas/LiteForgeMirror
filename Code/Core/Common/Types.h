// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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
#ifndef LF_CORE_TYPES_H
#define LF_CORE_TYPES_H

#pragma warning(disable : 4251) //  template needs to have a dll interface

#if defined(_WIN64)
#define LF_OS_WINDOWS
#define LF_PLATFORM_64
#elif defined(_WIN32)
#define LF_OS_WINDOWS
#define LF_PLATFORM_32
#else
#error "Unknown platform"
#endif

#if defined(_DEBUG)
#define LF_DEBUG_BREAK { volatile ::lf::Int32 BP = 0; if(!BP) { BP = 1;} }
#else
#define LF_DEBUG_BREAK
#endif

#if defined(LF_INTERNAL_DEBUG)
#define LF_DEBUG
#elif defined(LF_INTERNAL_TEST)
#define LF_TEST
#elif defined(LF_INTERNAL_RELEASE)
#define LF_RELEASE
#elif defined(LF_INTERNAL_FINAL)
#define LF_FINAL
#else
#error "Unknown build configuration, please define one of the preprocessor directives LF_INTERNAL_DEBUG/LF_INTERNAL_TEST/LF_INTERNAL_RELEASE/LF_INTERNAL_FINAL."
#endif

#if defined(LF_DEBUG) || defined(LF_TEST)
#define LF_USE_EXCEPTIONS
#endif

#define LF_CONCAT_WRAPPER(a,b) a##b
#define LF_CONCAT(a,b) LF_CONCAT_WRAPPER(a,b)
#define LF_ANONYMOUS_NAME(name) LF_CONCAT(name, __LINE__)
#define LF_STATIC_ASSERT(expression) static_assert(expression, #expression)
#define LF_STATIC_CRASH(message) static_assert(false, message)
#define LF_STATIC_IS_A(SourceT, DestT) { SourceT* compile_is_a_src = 0; DestT* compile_is_a_dest = compile_is_a_src; (compile_is_a_dest); }
#define LF_EXPORT_DLL _declspec(dllexport)
#define LF_IMPORT_DLL _declspec(dllimport)
#define LF_ALIGN(size) _declspec(align(size))
#define LF_ARRAY_SIZE(o) (sizeof(o)/sizeof(o[0]))
#define LF_INLINE inline
#define LF_FORCE_INLINE __forceinline
#define LF_NO_INLINE _declspec(noinline)
#define LF_THREAD_LOCAL _declspec(thread)
#define LF_SIMD_ALIGN 16

namespace lf {
using UInt8 = unsigned char;
using UInt16 = unsigned short;
using UInt32 = unsigned int;
using UInt64 = unsigned long long;

using Int8 = signed char;
using Int16 = signed short;
using Int32 = signed int;
using Int64 = signed long long;

using Float32 = float;
using Float64 = double;

using Char8 = char;
using Char16 = wchar_t;

using ByteT = unsigned char;
using SByteT = char;

#if defined(LF_PLATFORM_32)
using SizeT = unsigned int;
using UIntPtrT = unsigned int;
#elif defined(LF_PLATFORM_64)
using SizeT = unsigned long long;
using UIntPtrT = unsigned long long;
#endif

using Atomic16 = short;
using Atomic32 = long;
using Atomic64 = long long;

enum CopyOnWriteTag { COPY_ON_WRITE };
enum LazyTag { LAZY };
enum AcquireTag { ACQUIRE };
enum ASyncTag { ASYNC };

const UInt8 INVALID8 = static_cast<UInt8>(-1);
const UInt16 INVALID16 = static_cast<UInt16>(-1);
const UInt32 INVALID32 = static_cast<UInt32>(-1);
const UInt64 INVALID64 = static_cast<UInt64>(-1);
const SizeT INVALID = static_cast<SizeT>(-1);

LF_FORCE_INLINE bool Valid(UInt8 v) { return v != INVALID8; }
LF_FORCE_INLINE bool Invalid(UInt8 v) { return v == INVALID8; }
LF_FORCE_INLINE bool Valid(Int8 v) { return v != static_cast<Int8>(INVALID8); }
LF_FORCE_INLINE bool Invalid(Int8 v) { return v == static_cast<Int8>(INVALID8); }

LF_FORCE_INLINE bool Valid(UInt16 v) { return v != INVALID16; }
LF_FORCE_INLINE bool Invalid(UInt16 v) { return v == INVALID16; }
LF_FORCE_INLINE bool Valid(Int16 v) { return v != static_cast<Int16>(INVALID16); }
LF_FORCE_INLINE bool Invalid(Int16 v) { return v == static_cast<Int16>(INVALID16); }

LF_FORCE_INLINE bool Valid(UInt32 v) { return v != INVALID32; }
LF_FORCE_INLINE bool Invalid(UInt32 v) { return v == INVALID32; }
LF_FORCE_INLINE bool Valid(Int32 v) { return v != static_cast<Int32>(INVALID32); }
LF_FORCE_INLINE bool Invalid(Int32 v) { return v == static_cast<Int32>(INVALID32); }

LF_FORCE_INLINE bool Valid(UInt64 v) { return v != INVALID64; }
LF_FORCE_INLINE bool Invalid(UInt64 v) { return v == INVALID64; }
LF_FORCE_INLINE bool Valid(Int64 v) { return v != static_cast<Int64>(INVALID64); }
LF_FORCE_INLINE bool Invalid(Int64 v) { return v == static_cast<Int64>(INVALID64); }

template<typename TENUM>
LF_FORCE_INLINE bool ValidEnum(TENUM value)
{
    return value != TENUM::INVALID_ENUM;
}

template<typename TENUM>
LF_FORCE_INLINE bool InvalidEnum(TENUM value)
{
    return value == TENUM::INVALID_ENUM;
}

} // namespace lf

// Type Traits

template<typename T>
class InternalTypeName
{
public:
    operator const char*() const
    {
        LF_STATIC_CRASH("Type name is not declared!");
        return "INVALID_TYPE_NAME";
    }
};

template<typename T>
class InternalTypeDetails
{
public:
    operator const char*() const
    {
        LF_STATIC_CRASH("Type details is not declared!");
        return "INVALID_TYPE_DETAILS";
    }
};

namespace lf {

// A type trait used for initializing memory using the placement new operator
// eg; 
//      void Foo(T* memory) { new(memory)T(); }
struct ConstructPlacementNew { };
// A type trait used for initializing memory using default assignment operator
// eg;
//      void Foo(T* memory) { *memory = T(); }
struct ConstructDefaultAssign { };

} // namespace lf

// The type trait for type construction, by default everything uses the new placement operator
template<typename T>
struct TypeConstructionTraits { using TypeT = lf::ConstructPlacementNew; };

// Default overrides for primitive data types
template<> struct TypeConstructionTraits<lf::Int8> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<lf::Int16> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<lf::Int32> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<lf::Int64> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<lf::UInt8> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<lf::UInt16> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<lf::UInt32> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<lf::UInt64> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<lf::Float64> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<lf::Float32> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<bool> { using TypeT = lf::ConstructDefaultAssign; };
template<> struct TypeConstructionTraits<lf::Char16> { using TypeT = lf::ConstructDefaultAssign; };




#endif // LF_CORE_TYPES_H