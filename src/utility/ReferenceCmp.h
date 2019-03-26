#ifndef ZEN_UTILITY_REFERENCE_CMP_H_
#define ZEN_UTILITY_REFERENCE_CMP_H_

#include <functional>

template <typename T>
struct ReferenceWrapperCmp
{
    using type = std::reference_wrapper<T>;

    bool operator()(const type& lhs, const type& rhs) const noexcept
    {
        return std::less<T*>()(&lhs.get(), &rhs.get());
    }
};

#endif
