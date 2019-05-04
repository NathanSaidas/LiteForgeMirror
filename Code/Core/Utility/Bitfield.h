#ifndef LF_CORE_BITFIELD_H
#define LF_CORE_BITFIELD_H

#include "Core/Common/Types.h"
#include <initializer_list>

namespace lf {
template<typename EnumT, typename ValueT = UInt32>
class Bitfield
{
public:
    typedef EnumT enum_type;
    typedef ValueT value_type;

    LF_INLINE Bitfield();
    explicit LF_INLINE Bitfield(value_type mask);
    LF_INLINE Bitfield(const std::initializer_list<enum_type>& bits);
    LF_INLINE void Set(enum_type bit);
    LF_INLINE void Unset(enum_type bit);
    LF_INLINE void SetMask(value_type mask);
    LF_INLINE void UnsetMask(value_type mask);
    LF_INLINE bool Has(enum_type bit) const;
    LF_INLINE bool Is(value_type mask) const;
    LF_INLINE bool Any(value_type mask) const;
    LF_INLINE value_type Bit(enum_type bit) const;
    LF_INLINE void Reset();

    value_type value;
};

template<typename EnumT, typename ValueT = UInt32>
Bitfield<EnumT, ValueT>::Bitfield() : value(0) {}

template<typename EnumT, typename ValueT = UInt32>
Bitfield<EnumT, ValueT>::Bitfield(value_type mask) : value(mask) {}

template<typename EnumT, typename ValueT = UInt32>
Bitfield<EnumT, ValueT>::Bitfield(const std::initializer_list<enum_type>& bits) : value(0)
{
    for (const enum_type bit : bits)
    {
        Set(bit);
    }
}

template<typename EnumT, typename ValueT = UInt32>
void Bitfield<EnumT, ValueT>::Set(enum_type bit)
{
    value |= Bit(bit);
}

template<typename EnumT, typename ValueT = UInt32>
void Bitfield<EnumT, ValueT>::Unset(enum_type bit)
{
    value = value & ~(Bit(bit));
}

template<typename EnumT, typename ValueT = UInt32>
void Bitfield<EnumT, ValueT>::SetMask(value_type mask)
{
    value |= mask;
}

template<typename EnumT, typename ValueT = UInt32>
void Bitfield<EnumT, ValueT>::UnsetMask(value_type mask)
{
    value = value & ~(mask);
}

template<typename EnumT, typename ValueT = UInt32>
bool Bitfield<EnumT, ValueT>::Has(enum_type bit) const
{
    return(value & Bit(bit)) > 0;
}

template<typename EnumT, typename ValueT = UInt32>
bool Bitfield<EnumT, ValueT>::Is(value_type mask) const
{
    return(value & mask) == mask;
}

template<typename EnumT, typename ValueT = UInt32>
bool Bitfield<EnumT, ValueT>::Any(value_type mask) const
{
    return(value & mask) > 0;
}

template<typename EnumT, typename ValueT = UInt32>
typename Bitfield<EnumT, ValueT>::value_type Bitfield<EnumT, ValueT>::Bit(enum_type bit) const
{
    return value_type(1) << static_cast<value_type>(bit);
}

template<typename EnumT, typename ValueT = UInt32>
void Bitfield<EnumT, ValueT>::Reset()
{
    value = 0;
}
} // namespace lf

#endif // LF_CORE_BITFIELD_H
