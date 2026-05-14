#pragma once

#include <type_traits>

#define PARS_FLAGIFY(EnumType)                                                     \
    inline EnumType operator|(EnumType lhs, EnumType rhs) {                          \
        using T = std::underlying_type_t<EnumType>;                                  \
        return static_cast<EnumType>(static_cast<T>(lhs) | static_cast<T>(rhs));     \
    }                                                                                 \
    inline EnumType operator&(EnumType lhs, EnumType rhs) {                          \
        using T = std::underlying_type_t<EnumType>;                                  \
        return static_cast<EnumType>(static_cast<T>(lhs) & static_cast<T>(rhs));     \
    }                                                                                 \
    inline EnumType operator^(EnumType lhs, EnumType rhs) {                          \
        using T = std::underlying_type_t<EnumType>;                                  \
        return static_cast<EnumType>(static_cast<T>(lhs) ^ static_cast<T>(rhs));     \
    }                                                                                 \
    inline EnumType operator~(EnumType val) {                                        \
        using T = std::underlying_type_t<EnumType>;                                  \
        return static_cast<EnumType>(~static_cast<T>(val));                          \
    }                                                                                 \
    inline EnumType& operator|=(EnumType& lhs, EnumType rhs) {                       \
        lhs = lhs | rhs;                                                             \
        return lhs;                                                                  \
    }                                                                                 \
    inline EnumType& operator&=(EnumType& lhs, EnumType rhs) {                       \
        lhs = lhs & rhs;                                                             \
        return lhs;                                                                  \
    }                                                                                 \
    inline EnumType& operator^=(EnumType& lhs, EnumType rhs) {                       \
        lhs = lhs ^ rhs;                                                             \
        return lhs;                                                                  \
    }                                                                                 \
    inline bool has_flag(EnumType value, EnumType flag) {                            \
        using T = std::underlying_type_t<EnumType>;                                  \
        return (static_cast<T>(value) & static_cast<T>(flag)) != 0;                  \
    }


