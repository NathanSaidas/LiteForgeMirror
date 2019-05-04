#ifndef LF_CORE_STD_MAP_H
#define LF_CORE_STD_MAP_H

#include <map>

namespace lf
{
    template<typename KeyT, typename ValueT, typename PredT = std::less<KeyT>>
    using TMap = std::map<KeyT, ValueT, PredT>;

    template<typename KeyT, typename ValueT, typename PredT = std::less<KeyT>>
    using TMMap = std::multimap<KeyT, ValueT, PredT>;
}

#endif // LF_CORE_STD_MAP_H