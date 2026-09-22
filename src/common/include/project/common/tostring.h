#ifndef TO_STRING_INCLUDED
#define TO_STRING_INCLUDED

#include <string>
#include <string_view>
#include <sstream>
#include <type_traits>
#include <utility>
#include <fmt/format.h>
#include <fmt/chrono.h>


namespace frmt {

template <typename T>
constexpr std::string to_string(T&& value) {
    using type = std::remove_cvref_t<T>;
    if constexpr(std::is_enum_v<type>) {
        return std::to_string(static_cast<std::underlying_type_t<type>>(value));
    } 
    else {
        return fmt::format("{}", value);
    }
}


template <typename T>
constexpr std::string to_string(const T& value) {
    using type = std::remove_cvref_t<T>;
    if constexpr(std::is_enum_v<type>) {
        return std::to_string(static_cast<std::underlying_type_t<type>>(value));
    } 
    else {
        return fmt::format("{}", value);
    }
}


template <typename... Args>
constexpr std::string concat(Args&&... args) {
    std::string res;
    auto concat_t = [](auto&& arg) {
        using type = std::remove_cvref_t<decltype(arg)>;
        if constexpr(std::is_enum_v<type>) {
            return std::to_string(static_cast<std::underlying_type_t<type>>(arg));
        }
        else {
            return fmt::format("{}", arg);
        }
    };
    ((res += concat_t(std::forward<Args>(args)) + " "), ...);
    res.pop_back();
    return res;
}

} // namespace tstr

#endif