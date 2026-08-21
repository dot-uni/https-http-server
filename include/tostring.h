#ifndef TO_STRING_INCLUDED
#define TO_STRING_INCLUDED

#include <string>
#include <string_view>
#include <sstream>
#include <type_traits>
#include <utility>


namespace {
    
template <typename, typename=void>
struct is_streamable : std::false_type {};

template <typename T> 
struct is_streamable<
    T,
    std::void_t<decltype(std::declval<std::ostringstream&>() << std::declval<T>())>
> : std::true_type {};

template <typename T>
constexpr bool is_streamable_v = is_streamable<T>::value;

} // namespace


namespace frmt {


template <typename T>
std::string toString(T&& value) {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr(std::is_enum_v<type>) {
        return std::to_string(static_cast<std::underlying_type_t<type>>(value));
    } 
    else {
        std::ostringstream oss; 
        oss << std::forward<T>(value);
        return oss.str();
    }
}


template <typename... Args>
std::string concat(Args&&... args) {
    std::string res;
    auto concat_t = [](auto&& arg) {
        using type = std::remove_cv_t<std::remove_reference_t<decltype(arg)>>;
        if constexpr(std::is_enum_v<type>) {
            return std::to_string(static_cast<std::underlying_type_t<type>>(arg));
        }
        else {
            std::ostringstream oss;
            oss << std::forward<decltype(arg)>(arg);
            return oss.str();
        }
    };
    ((res += concat_t(std::forward<Args>(args)) + " "), ...);
    res.pop_back();
    return res;
}

} // namespace tstr

#endif