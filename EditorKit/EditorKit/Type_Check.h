#pragma once
#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <string>
// 类型检查和工具函数
namespace type_check
{
    template<typename T>
    struct is_value_type
    {
        static constexpr bool value = !std::is_reference_v<T>;
    };

    template<typename... Args>
    struct all_value_types
    {
        static constexpr bool value = (is_value_type<Args>::value && ...);
    };

    template<typename... Args>
    constexpr void assert_value_types()
    {
        static_assert(all_value_types<Args...>::value,
            "All parameters must be value types (non-reference).");
    }

    // 获取类型名称
    template<typename T>
    std::string get_type_name()
    {
        if constexpr (std::is_reference_v<T>)
        {
            if constexpr (std::is_lvalue_reference_v<T>)
            {
                return std::string(typeid(std::remove_reference_t<T>).name()) + "&";
            }
            else
            {
                return std::string(typeid(std::remove_reference_t<T>).name()) + "&&";
            }
        }
        else
        {
            return std::string(typeid(T).name());
        }
    }

    // 构建类型名称字符串
    template<typename T, typename... Args>
    std::string build_type_names_string()
    {
        if constexpr (sizeof...(Args) == 0)
        {
            return get_type_name<T>();
        }
        else
        {
            return get_type_name<T>() + ", " + build_type_names_string<Args...>();
        }
    }

    template<typename... Args>
    std::string get_template_args_info()
    {
        if constexpr (sizeof...(Args) == 0)
        {
            return "void";
        }
        else
        {
            return build_type_names_string<Args...>();
        }
    }

    // C++17兼容的元组类型检查辅助类
    template<typename Tuple>
    struct tuple_value_type_checker;

    template<typename... Ts>
    struct tuple_value_type_checker<std::tuple<Ts...>>
    {
        static void check()
        {
            assert_value_types<Ts...>();
        }
    };
}