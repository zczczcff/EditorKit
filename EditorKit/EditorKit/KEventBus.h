#pragma once

//----------------------------------------------------------自定义键类型事件总线系统-------------------------------------------------

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <typeinfo>
#include <typeindex>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <utility>
#include <random>
#include <iomanip>
#include "Type_Check.h"

//！！！重要说明！！！
/*
*   1.Subscribe回调只支持值类型参数;publish不限定参数引用类型/值类型
*   2.Subscribe回调和Publish函数参数数量、类型必须精确匹配，隐式类型转换在此处无效
*       如参数为（short i）的回调，使用publish(10)无法匹配；参数为（std::string s）的回调，使用publish("Test")无法匹配
*       正确方式为：（short i）->publish(short(10));（std::string s）->publish(std::string("Test"))或（const char* s）->publish("Test")
*   3.新增单播事件功能：每个事件名称只能有一个订阅者，后订阅的会覆盖先订阅的
*   4.支持自定义键类型和哈希函数
*/

// 订阅模式枚举
enum class SubscriptionMode
{
    Multicast,  // 多播模式（默认）：一个事件可以有多个订阅者
    Unicast     // 单播模式：一个事件只能有一个订阅者，后订阅的覆盖先订阅的
};


// UUID实现
class EventID
{
private:
    uint64_t data_[2]; // 128位UUID

public:
    EventID() : data_{ 0, 0 } {}

    EventID(uint64_t part1, uint64_t part2) : data_{ part1, part2 } {}

    // 生成UUID
    static EventID generate()
    {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis;

        return EventID(dis(gen), dis(gen));
    }

    // 转换为字符串
    std::string toString() const
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0')
            << std::setw(16) << data_[0]
            << std::setw(16) << data_[1];
        return ss.str();
    }

    // 比较操作符
    bool operator==(const EventID& other) const
    {
        return data_[0] == other.data_[0] && data_[1] == other.data_[1];
    }

    bool operator<(const EventID& other) const
    {
        if (data_[0] != other.data_[0]) return data_[0] < other.data_[0];
        return data_[1] < other.data_[1];
    }

    // 哈希函数支持
    struct Hash
    {
        size_t operator()(const EventID& uuid) const
        {
            return std::hash<uint64_t>{}(uuid.data_[0]) ^
                (std::hash<uint64_t>{}(uuid.data_[1]) << 1);
        }
    };
};

// 发布结果结构
struct PublishResult
{
    bool success;  // 是否至少有一个订阅者成功执行
    int totalSubscribers;  // 总订阅者数量
    int successfulExecutions;  // 成功执行的数量
    int failedExecutions;  // 执行失败的数量
    std::string errorMessage;  // 错误信息（如果有的话）
    std::string publishedArgTypes;  // 发布的参数类型
    std::vector<std::string> failedSubscriberTypes;  // 失败的订阅者参数类型
    SubscriptionMode publishMode;  // 发布模式

    // 新增：所有订阅者的预期参数类型
    std::vector<std::string> allExpectedTypes;

    PublishResult(bool s = true, const std::string& msg = "",
        const std::string& published = "", const std::string& expected = "",
        SubscriptionMode mode = SubscriptionMode::Multicast)
        : success(s), totalSubscribers(0), successfulExecutions(0),
        failedExecutions(0), errorMessage(msg),
        publishedArgTypes(published), publishMode(mode)
    {
        if (!expected.empty())
        {
            allExpectedTypes.push_back(expected);
        }
    }

    // 新增：添加统计信息的方法
    void AddSuccess()
    {
        successfulExecutions++;
        totalSubscribers++;
        success = successfulExecutions > 0;  // 更新success标志
    }

    void AddFailure(const std::string& expectedType)
    {
        failedExecutions++;
        totalSubscribers++;
        failedSubscriberTypes.push_back(expectedType);
        allExpectedTypes.push_back(expectedType);
        success = successfulExecutions > 0;  // 至少有一个成功就算总体成功
    }

    // 获取详细的统计信息
    std::string GetStatistics() const
    {
        std::stringstream ss;
        ss << "执行统计: " << successfulExecutions << "/" << totalSubscribers
            << " 个订阅者成功执行";
        if (failedExecutions > 0)
        {
            ss << ", " << failedExecutions << " 个失败";
        }
        ss << " (模式: " << (publishMode == SubscriptionMode::Unicast ? "单播" : "多播") << ")";
        return ss.str();
    }
};

// 类型信息获取（支持引用）
template<typename T>
std::string GetTypeNameImpl()
{
    //if constexpr (std::is_reference_v<T>)
    //{
    //    if constexpr (std::is_lvalue_reference_v<T>)
    //    {
    //        return std::string(typeid(std::remove_reference_t<T>).name()) + "&";
    //    }
    //    else
    //    {
    //        return std::string(typeid(std::remove_reference_t<T>).name()) + "&&";
    //    }
    //}
    //else
    //{
    //    return std::string(typeid(T).name());
    //}
    return std::string(typeid(T).name());
}

template<typename T, typename... Args>
std::string BuildTypeNamesString()
{
    if constexpr (sizeof...(Args) == 0)
    {
        return GetTypeNameImpl<T>();
    }
    else
    {
        return GetTypeNameImpl<T>() + ", " + BuildTypeNamesString<Args...>();
    }
}

template<typename... Args>
std::string GetTemplateArgsInfo()
{
    if constexpr (sizeof...(Args) == 0)
    {
        return "void";
    }
    else
    {
        return BuildTypeNamesString<Args...>();
    }
}

// 事件函数接口
class IEventFunction
{
protected:
    EventID token_;
    std::string description_;
    SubscriptionMode mode_;
    std::string argTypes_;  // 存储参数类型信息
public:
    IEventFunction(const EventID& token, const std::string& description,
        SubscriptionMode mode, const std::string& argTypes)
        : token_(token), description_(description), mode_(mode), argTypes_(argTypes)
    {
    }
    virtual ~IEventFunction() = default;

    const EventID& GetToken() const { return token_; }
    const std::string& GetDescription() const { return description_; }
    SubscriptionMode GetMode() const { return mode_; }
    virtual std::string GetArgTypes() { return argTypes_; };
    virtual size_t GetArgCount() const = 0;
    virtual bool ExecuteWithForward(void* args[]) = 0;
};

// 事件函数实现（支持完美转发）
template <typename... Args>
class EventFunctionImpl : public IEventFunction
{
public:
    using HandlerType = std::function<void(Args...)>;

    EventFunctionImpl(HandlerType delegate, const EventID& token,
        const std::string& description, SubscriptionMode mode)
        : IEventFunction(token, description, mode, GetTemplateArgsInfo<Args...>()),
        delegate_(std::move(delegate))
    {
        // 移除值类型限制，支持引用类型
    }

    // 完美转发执行
    void Execute(Args&&... args) const
    {
        delegate_(std::forward<Args>(args)...);
    }

    size_t GetArgCount() const override
    {
        return sizeof...(Args);
    }

    // 实现字节码接口的完美转发
    bool ExecuteWithForward(void* args[]) override
    {
        if constexpr (sizeof...(Args) == 0)
        {
            Execute();
            return true;
        }
        else
        {
            return ExecuteWithArgs(args, std::index_sequence_for<Args...>{});
        }
    }

private:
    template<size_t... Is>
    bool ExecuteWithArgs(void* args[], std::index_sequence<Is...>)
    {
        // 类型匹配检查
        if (!CheckArgTypes<Args...>(args))
        {
            return false;
        }

        // 使用完美转发
        Execute(
            std::forward<Args>(
                *static_cast<std::remove_reference_t<Args>*>(args[Is])
                )...
        );
        return true;
    }

    // 类型安全检查
    template<typename T, typename... Rest>
    bool CheckArgTypes(void* args[])
    {
        if constexpr (sizeof...(Rest) == 0)
        {
            return CheckSingleArgType<T>(args[0]);
        }
        else
        {
            return CheckSingleArgType<T>(args[0]) &&
                CheckArgTypes<Rest...>(args + 1);
        }
    }

    template<typename T>
    bool CheckSingleArgType(void* arg)
    {
        using RawType = std::remove_reference_t<T>;
        return arg != nullptr;
    }

    HandlerType delegate_;
};


// 事件总线 - 支持自定义键类型和哈希函数
template<typename EventKeyType = std::string, typename Hash = std::hash<EventKeyType>>
class EventBus
{
public:
    // 订阅接口 - 支持lambda直接传递（默认多播模式）
    template <typename Callable>
    EventID Subscribe(const EventKeyType& eventName, Callable&& handler,
        const std::string& description = std::string(), bool once = false)
    {
        return SubscribeImpl(eventName, std::forward<Callable>(handler), description, once, SubscriptionMode::Multicast);
    }

    // 订阅接口 - 指定订阅模式
    template <typename Callable>
    EventID Subscribe(const EventKeyType& eventName, Callable&& handler,
        SubscriptionMode mode, const std::string& description = std::string(), bool once = false)
    {
        return SubscribeImpl(eventName, std::forward<Callable>(handler), description, once, mode);
    }

    // 订阅单播事件接口 - 简化单播订阅
    template <typename Callable>
    EventID SubscribeUnicast(const EventKeyType& eventName, Callable&& handler,
        const std::string& description = std::string(), bool once = false)
    {
        return SubscribeImpl(eventName, std::forward<Callable>(handler), description, once, SubscriptionMode::Unicast);
    }

    // 订阅接口 - 使用std::function指定类型（多播模式）
    template <typename... Args>
    EventID Subscribe(const EventKeyType& eventName,
        std::function<void(Args...)> handler,
        const std::string& description = std::string(),
        bool once = false)
    {
        // 编译期检查参数类型
        //type_check::assert_value_types<Args...>();
        return SubscribeDetailed<Args...>(eventName, std::move(handler), description, once, SubscriptionMode::Multicast);
    }

    // 订阅接口 - 使用std::function指定类型和订阅模式
    template <typename... Args>
    EventID Subscribe(const EventKeyType& eventName,
        std::function<void(Args...)> handler,
        SubscriptionMode mode,
        const std::string& description = std::string(),
        bool once = false)
    {
        // 编译期检查参数类型
        //type_check::assert_value_types<Args...>();
        return SubscribeDetailed<Args...>(eventName, std::move(handler), description, once, mode);
    }

    // 取消订阅
    bool Unsubscribe(const EventID& token);

    // 发布事件（默认多播模式）- 使用完美转发
    template <typename... Args>
    PublishResult Publish(const EventKeyType& eventName, Args&&... args)
    {
        return PublishImpl<Args...>(eventName, SubscriptionMode::Multicast, std::forward<Args>(args)...);
    }

    // 发布单播事件 - 使用完美转发
    template <typename... Args>
    PublishResult PublishUnicast(const EventKeyType& eventName, Args&&... args)
    {
        return PublishImpl<Args...>(eventName, SubscriptionMode::Unicast, std::forward<Args>(args)...);
    }

    // 发布事件到指定模式 - 使用完美转发
    template <typename... Args>
    PublishResult Publish(const EventKeyType& eventName, SubscriptionMode mode, Args&&... args)
    {
        return PublishImpl<Args...>(eventName, mode, std::forward<Args>(args)...);
    }

    // 是否有订阅者（检查多播）
    bool HasSubscribers(const EventKeyType& eventName) const;

    // 是否有单播订阅者
    bool HasUnicastSubscribers(const EventKeyType& eventName) const;

    // 获取订阅者数量（多播）
    size_t GetSubscriberCount(const EventKeyType& eventName) const;

    // 获取单播订阅者数量
    size_t GetUnicastSubscriberCount(const EventKeyType& eventName) const;

    // 获取所有事件信息字符串
    std::string PrintAllEvents() const;

    // 获取事件统计信息
    std::string GetEventStatistics() const;

    // 检查事件是否存在（多播或单播）
    bool HasEvent(const EventKeyType& eventName) const;

    // 获取事件的订阅模式
    SubscriptionMode GetEventMode(const EventKeyType& eventName) const;

    ~EventBus();

private:
    template <typename Callable>
    EventID SubscribeImpl(const EventKeyType& eventName, Callable&& handler,
        const std::string& description, bool once, SubscriptionMode mode)
    {
        using traits = function_traits<std::decay_t<Callable>>;
        using arg_types = typename traits::argument_types;

        // 使用编译时if constexpr处理不同参数数量
        if constexpr (traits::arity == 0)
        {
            return SubscribeDetailed<>(eventName,
                std::function<void()>(std::forward<Callable>(handler)), description, once, mode);
        }
        else
        {
            // 编译期检查Callable的参数类型
            using arg_types = typename traits::argument_types;
            //type_check::tuple_value_type_checker<arg_types>::check();

            if constexpr (traits::arity == 1)
            {
                return SubscribeDetailed<typename traits::template arg_type<0>>(
                    eventName,
                    std::function<void(typename traits::template arg_type<0>)>(
                        std::forward<Callable>(handler)),
                    description, once, mode);
            }
            else if constexpr (traits::arity == 2)
            {
                return SubscribeDetailed<
                    typename traits::template arg_type<0>,
                    typename traits::template arg_type<1>>(
                        eventName,
                        std::function<void(
                            typename traits::template arg_type<0>,
                            typename traits::template arg_type<1>)>(
                                std::forward<Callable>(handler)),
                        description, once, mode);
            }
            else if constexpr (traits::arity == 3)
            {
                return SubscribeDetailed<
                    typename traits::template arg_type<0>,
                    typename traits::template arg_type<1>,
                    typename traits::template arg_type<2>>(
                        eventName,
                        std::function<void(
                            typename traits::template arg_type<0>,
                            typename traits::template arg_type<1>,
                            typename traits::template arg_type<2>)>(
                                std::forward<Callable>(handler)),
                        description, once, mode);
            }
            else if constexpr (traits::arity == 4)
            {
                return SubscribeDetailed<
                    typename traits::template arg_type<0>,
                    typename traits::template arg_type<1>,
                    typename traits::template arg_type<2>,
                    typename traits::template arg_type<3>>(
                        eventName,
                        std::function<void(
                            typename traits::template arg_type<0>,
                            typename traits::template arg_type<1>,
                            typename traits::template arg_type<2>,
                            typename traits::template arg_type<3>)>(
                                std::forward<Callable>(handler)),
                        description, once, mode);
            }
            else if constexpr (traits::arity == 5)
            {
                return SubscribeDetailed<
                    typename traits::template arg_type<0>,
                    typename traits::template arg_type<1>,
                    typename traits::template arg_type<2>,
                    typename traits::template arg_type<3>,
                    typename traits::template arg_type<4>>(
                        eventName,
                        std::function<void(
                            typename traits::template arg_type<0>,
                            typename traits::template arg_type<1>,
                            typename traits::template arg_type<2>,
                            typename traits::template arg_type<3>,
                            typename traits::template arg_type<4>)>(
                                std::forward<Callable>(handler)),
                        description, once, mode);
            }
            else if constexpr (traits::arity == 6)
            {
                return SubscribeDetailed<
                    typename traits::template arg_type<0>,
                    typename traits::template arg_type<1>,
                    typename traits::template arg_type<2>,
                    typename traits::template arg_type<3>,
                    typename traits::template arg_type<4>,
                    typename traits::template arg_type<5>>(
                        eventName,
                        std::function<void(
                            typename traits::template arg_type<0>,
                            typename traits::template arg_type<1>,
                            typename traits::template arg_type<2>,
                            typename traits::template arg_type<3>,
                            typename traits::template arg_type<4>,
                            typename traits::template arg_type<5>)>(
                                std::forward<Callable>(handler)),
                        description, once, mode);
            }
            else if constexpr (traits::arity == 7)
            {
                return SubscribeDetailed<
                    typename traits::template arg_type<0>,
                    typename traits::template arg_type<1>,
                    typename traits::template arg_type<2>,
                    typename traits::template arg_type<3>,
                    typename traits::template arg_type<4>,
                    typename traits::template arg_type<5>,
                    typename traits::template arg_type<6>>(
                        eventName,
                        std::function<void(
                            typename traits::template arg_type<0>,
                            typename traits::template arg_type<1>,
                            typename traits::template arg_type<2>,
                            typename traits::template arg_type<3>,
                            typename traits::template arg_type<4>,
                            typename traits::template arg_type<5>,
                            typename traits::template arg_type<6>)>(
                                std::forward<Callable>(handler)),
                        description, once, mode);
            }
            else if constexpr (traits::arity == 8)
            {
                return SubscribeDetailed<
                    typename traits::template arg_type<0>,
                    typename traits::template arg_type<1>,
                    typename traits::template arg_type<2>,
                    typename traits::template arg_type<3>,
                    typename traits::template arg_type<4>,
                    typename traits::template arg_type<5>,
                    typename traits::template arg_type<6>,
                    typename traits::template arg_type<7>>(
                        eventName,
                        std::function<void(
                            typename traits::template arg_type<0>,
                            typename traits::template arg_type<1>,
                            typename traits::template arg_type<2>,
                            typename traits::template arg_type<3>,
                            typename traits::template arg_type<4>,
                            typename traits::template arg_type<5>,
                            typename traits::template arg_type<6>,
                            typename traits::template arg_type<7>)>(
                                std::forward<Callable>(handler)),
                        description, once, mode);
            }
            else if constexpr (traits::arity == 9)
            {
                return SubscribeDetailed<
                    typename traits::template arg_type<0>,
                    typename traits::template arg_type<1>,
                    typename traits::template arg_type<2>,
                    typename traits::template arg_type<3>,
                    typename traits::template arg_type<4>,
                    typename traits::template arg_type<5>,
                    typename traits::template arg_type<6>,
                    typename traits::template arg_type<7>,
                    typename traits::template arg_type<8>>(
                        eventName,
                        std::function<void(
                            typename traits::template arg_type<0>,
                            typename traits::template arg_type<1>,
                            typename traits::template arg_type<2>,
                            typename traits::template arg_type<3>,
                            typename traits::template arg_type<4>,
                            typename traits::template arg_type<5>,
                            typename traits::template arg_type<6>,
                            typename traits::template arg_type<7>,
                            typename traits::template arg_type<8>)>(
                                std::forward<Callable>(handler)),
                        description, once, mode);
            }
            else
            {
                static_assert(traits::arity <= 9, "最多支持9个参数");
                return EventID();
            }
        }
    }

    // 详细的订阅实现
    template <typename... Args>
    EventID SubscribeDetailed(const EventKeyType& eventName,
        std::function<void(Args...)> handler,
        const std::string& description,
        bool once,
        SubscriptionMode mode)
    {
        EventID token = EventID::generate();

        // 创建委托
        auto delegate = std::make_unique<EventFunctionImpl<Args...>>(
            std::move(handler), token, description, mode);

        if (mode == SubscriptionMode::Unicast)
        {
            // 单播模式：检查是否已存在订阅者，存在则先取消
            auto it = unicastEventHandlers_.find(eventName);
            if (it != unicastEventHandlers_.end())
            {
                // 移除旧订阅者的一次性事件记录
                auto onceIt = unicastOnceEventHandlers_.find(eventName);
                if (onceIt != unicastOnceEventHandlers_.end())
                {
                    unicastOnceEventHandlers_.erase(onceIt);
                }

                // 移除Token映射
                tokenToEventNameMap_.erase(it->second->GetToken());
            }

            // 注册单播事件处理器
            unicastEventHandlers_[eventName] = std::move(delegate);

            // 如果是一次性订阅，添加到一次性事件列表
            if (once)
            {
                unicastOnceEventHandlers_[eventName] = token;
            }
        }
        else
        {
            // 多播模式：添加到处理器列表
            multicastEventHandlers_[eventName].push_back(std::move(delegate));

            // 如果是一次性订阅，添加到一次性事件列表
            if (once)
            {
                multicastOnceEventHandlers_[eventName].push_back(token);
            }
        }

        // 注册Token到事件名称的映射
        tokenToEventNameMap_[token] = eventName;

        return token;
    }

    // 发布事件实现（使用完美转发）
    template <typename... Args>
    PublishResult PublishImpl(const EventKeyType& eventName, SubscriptionMode mode, Args&&... args)
    {
        PublishResult result;
        result.publishedArgTypes = GetTemplateArgsInfo<std::decay_t<Args>...>();
        result.publishMode = mode;

        // 创建参数的副本或引用，确保生命周期
        auto argStorage = std::make_tuple(std::forward<Args>(args)...);

        // 准备参数指针数组，指向tuple中的元素
        void* argPointers[sizeof...(Args)+1] = {};
        PrepareArgPointers(argPointers, argStorage, std::index_sequence_for<Args...>{});

        if (mode == SubscriptionMode::Unicast)
        {
            // 单播发布
            auto it = unicastEventHandlers_.find(eventName);
            if (it == unicastEventHandlers_.end())
            {
                result.success = false;
                result.errorMessage = "单播事件未找到";
                return result;
            }

            //auto typedHandler = dynamic_cast<EventFunctionImpl<Args...>*>(it->second.get());
            //if (!typedHandler)
            // 检查参数数量和类型名称是否匹配
            if (it->second->GetArgCount() != sizeof...(Args) ||
                it->second->GetArgTypes() != GetTemplateArgsInfo<std::decay_t<Args>...>())
            {
                result.AddFailure(it->second->GetArgTypes());
                result.errorMessage = "单播事件参数类型不匹配";
                return result;
            }

            // 使用完美转发执行
            if (it->second->ExecuteWithForward(argPointers))
            {
                result.AddSuccess();

                // 检查是否是一次性事件
                auto onceIt = unicastOnceEventHandlers_.find(eventName);
                if (onceIt != unicastOnceEventHandlers_.end())
                {
                    Unsubscribe(onceIt->second);
                }
            }
            else
            {
                result.AddFailure(it->second->GetArgTypes());
                result.errorMessage = "单播事件执行失败";
            }
        }
        else
        {
            // 多播发布
            auto handlersIt = multicastEventHandlers_.find(eventName);
            if (handlersIt == multicastEventHandlers_.end())
            {
                result.success = false;
                result.errorMessage = "多播事件未找到";
                return result;
            }

            // 临时存储需要移除的一次性事件Token
            std::vector<EventID> tokensToRemove;
            bool hasAnySuccessfulExecution = false;

            // 遍历所有订阅者
            for (const auto& handler : handlersIt->second)
            {
                // 检查参数数量和类型名称是否匹配
                //auto typedHandler = dynamic_cast<EventFunctionImpl<Args...>*>(handler.get());//使用dynamic_cast进行参数验证性能只有使用typeid进行参数验证的1/30（release）
                //if (!typedHandler)
				if (handler->GetArgCount() != sizeof...(Args) ||
                    handler->GetArgTypes() != GetTemplateArgsInfo<std::decay_t<Args>...>())
                {
                    result.AddFailure(handler->GetArgTypes());
                    continue;
                }

                // 使用完美转发执行
                if (handler->ExecuteWithForward(argPointers))
                {
                    result.AddSuccess();
                    hasAnySuccessfulExecution = true;

                    // 检查是否是一次性事件
                    auto onceIt = multicastOnceEventHandlers_.find(eventName);
                    if (onceIt != multicastOnceEventHandlers_.end())
                    {
                        const auto& onceTokens = onceIt->second;
                        if (std::find(onceTokens.begin(), onceTokens.end(),
                            handler->GetToken()) != onceTokens.end())
                        {
                            tokensToRemove.push_back(handler->GetToken());
                        }
                    }
                }
                else
                {
                    result.AddFailure(handler->GetArgTypes());
                }
            }

            // 设置总体成功标志
            result.success = hasAnySuccessfulExecution;

            // 如果没有成功执行任何订阅者，且有失败的情况，设置错误信息
            if (!hasAnySuccessfulExecution && result.failedExecutions > 0)
            {
                std::stringstream ss;
                ss << "所有 " << result.totalSubscribers << " 个订阅者都执行失败。";
                ss << "发布的参数类型: " << result.publishedArgTypes << "。";
                ss << "期望的参数类型: ";
                for (size_t i = 0; i < result.allExpectedTypes.size(); ++i)
                {
                    if (i > 0) ss << ", ";
                    ss << result.allExpectedTypes[i];
                }
                result.errorMessage = ss.str();
            }

            // 移除一次性事件
            for (const auto& token : tokensToRemove)
            {
                Unsubscribe(token);
            }
        }

        return result;
    }

    // 准备参数指针数组（从KEventBus_Ref借鉴）
    template<typename Tuple, size_t... Is>
    void PrepareArgPointers(void* pointers[], Tuple& tuple, std::index_sequence<Is...>)
    {
        ((pointers[Is] = static_cast<void*>(&std::get<Is>(tuple))), ...);
    }


    // 存储多播事件处理器 - 使用自定义哈希函数
    std::unordered_map<EventKeyType, std::vector<std::unique_ptr<IEventFunction>>, Hash> multicastEventHandlers_;

    // 存储单播事件处理器 - 使用自定义哈希函数
    std::unordered_map<EventKeyType, std::unique_ptr<IEventFunction>, Hash> unicastEventHandlers_;

    // 存储多播一次性事件 - 使用自定义哈希函数
    std::unordered_map<EventKeyType, std::vector<EventID>, Hash> multicastOnceEventHandlers_;

    // 存储单播一次性事件 - 使用自定义哈希函数
    std::unordered_map<EventKeyType, EventID, Hash> unicastOnceEventHandlers_;

    // Token到事件名称映射（使用UUID的哈希函数）
    std::unordered_map<EventID, EventKeyType, EventID::Hash> tokenToEventNameMap_;
};

// 取消订阅实现
template<typename EventKeyType, typename Hash>
bool EventBus<EventKeyType, Hash>::Unsubscribe(const EventID& token)
{
    auto it = tokenToEventNameMap_.find(token);
    if (it == tokenToEventNameMap_.end())
    {
        return false;
    }

    const EventKeyType& eventName = it->second;

    // 从多播事件处理器中移除
    auto multicastIt = multicastEventHandlers_.find(eventName);
    if (multicastIt != multicastEventHandlers_.end())
    {
        auto& handlers = multicastIt->second;
        for (auto handlerIt = handlers.begin(); handlerIt != handlers.end(); ++handlerIt)
        {
            if ((*handlerIt)->GetToken() == token)
            {
                handlers.erase(handlerIt);

                // 如果列表为空，移除整个事件
                if (handlers.empty())
                {
                    multicastEventHandlers_.erase(multicastIt);
                }
                break;
            }
        }
    }

    // 从单播事件处理器中移除
    auto unicastIt = unicastEventHandlers_.find(eventName);
    if (unicastIt != unicastEventHandlers_.end() && unicastIt->second->GetToken() == token)
    {
        unicastEventHandlers_.erase(unicastIt);
    }

    // 从多播一次性事件列表移除
    auto multicastOnceIt = multicastOnceEventHandlers_.find(eventName);
    if (multicastOnceIt != multicastOnceEventHandlers_.end())
    {
        auto& onceTokens = multicastOnceIt->second;
        for (auto tokenIt = onceTokens.begin(); tokenIt != onceTokens.end(); ++tokenIt)
        {
            if (*tokenIt == token)
            {
                onceTokens.erase(tokenIt);
                if (onceTokens.empty())
                {
                    multicastOnceEventHandlers_.erase(multicastOnceIt);
                }
                break;
            }
        }
    }

    // 从单播一次性事件列表移除
    auto unicastOnceIt = unicastOnceEventHandlers_.find(eventName);
    if (unicastOnceIt != unicastOnceEventHandlers_.end() && unicastOnceIt->second == token)
    {
        unicastOnceEventHandlers_.erase(unicastOnceIt);
    }

    // 移除Token映射
    tokenToEventNameMap_.erase(it);

    return true;
}

// 订阅者查询实现
template<typename EventKeyType, typename Hash>
bool EventBus<EventKeyType, Hash>::HasSubscribers(const EventKeyType& eventName) const
{
    auto it = multicastEventHandlers_.find(eventName);
    return it != multicastEventHandlers_.end() && !it->second.empty();
}

template<typename EventKeyType, typename Hash>
bool EventBus<EventKeyType, Hash>::HasUnicastSubscribers(const EventKeyType& eventName) const
{
    auto it = unicastEventHandlers_.find(eventName);
    return it != unicastEventHandlers_.end();
}

template<typename EventKeyType, typename Hash>
size_t EventBus<EventKeyType, Hash>::GetSubscriberCount(const EventKeyType& eventName) const
{
    auto it = multicastEventHandlers_.find(eventName);
    return it != multicastEventHandlers_.end() ? it->second.size() : 0;
}

template<typename EventKeyType, typename Hash>
size_t EventBus<EventKeyType, Hash>::GetUnicastSubscriberCount(const EventKeyType& eventName) const
{
    auto it = unicastEventHandlers_.find(eventName);
    return it != unicastEventHandlers_.end() ? 1 : 0;
}

template<typename EventKeyType, typename Hash>
bool EventBus<EventKeyType, Hash>::HasEvent(const EventKeyType& eventName) const
{
    return HasSubscribers(eventName) || HasUnicastSubscribers(eventName);
}

template<typename EventKeyType, typename Hash>
SubscriptionMode EventBus<EventKeyType, Hash>::GetEventMode(const EventKeyType& eventName) const
{
    if (HasUnicastSubscribers(eventName))
    {
        return SubscriptionMode::Unicast;
    }
    else if (HasSubscribers(eventName))
    {
        return SubscriptionMode::Multicast;
    }
    return SubscriptionMode::Multicast; // 默认返回多播
}

template<typename EventKeyType, typename Hash>
std::string EventBus<EventKeyType, Hash>::PrintAllEvents() const
{
    std::stringstream ss;
    ss << "=== EventBus 事件统计 ===" << std::endl;
    ss << "多播事件类型数量: " << multicastEventHandlers_.size() << std::endl;
    ss << "单播事件类型数量: " << unicastEventHandlers_.size() << std::endl;

    // 多播事件
    for (const auto& [eventName, handlers] : multicastEventHandlers_)
    {
        ss << "\n[多播] 事件名称: " << eventName << std::endl;
        ss << "订阅者数量: " << handlers.size() << std::endl;

        for (const auto& handler : handlers)
        {
            ss << "  Token: " << handler->GetToken().toString() << std::endl;
            ss << "  参数类型: " << handler->GetArgTypes() << std::endl;
            ss << "  参数数量: " << handler->GetArgCount() << std::endl;
            ss << "  描述: " << handler->GetDescription() << std::endl;
        }
    }

    // 单播事件
    for (const auto& [eventName, handler] : unicastEventHandlers_)
    {
        ss << "\n[单播] 事件名称: " << eventName << std::endl;
        ss << "  Token: " << handler->GetToken().toString() << std::endl;
        ss << "  参数类型: " << handler->GetArgTypes() << std::endl;
        ss << "  参数数量: " << handler->GetArgCount() << std::endl;
        ss << "  描述: " << handler->GetDescription() << std::endl;
    }
    ss << "=========================" << std::endl;
    return ss.str();
}

template<typename EventKeyType, typename Hash>
std::string EventBus<EventKeyType, Hash>::GetEventStatistics() const
{
    std::stringstream ss;
    ss << "EventBus Statistics:\n";
    ss << "Multicast Event Types: " << multicastEventHandlers_.size() << "\n";
    ss << "Unicast Event Types: " << unicastEventHandlers_.size() << "\n";

    size_t totalMulticastSubscribers = 0;
    for (const auto& [eventName, handlers] : multicastEventHandlers_)
    {
        totalMulticastSubscribers += handlers.size();
        ss << "Multicast Event: " << eventName
            << " - Subscribers: " << handlers.size() << "\n";
    }

    for (const auto& [eventName, handler] : unicastEventHandlers_)
    {
        ss << "Unicast Event: " << eventName << " - Subscribers: 1\n";
    }

    ss << "Total Multicast Subscribers: " << totalMulticastSubscribers << "\n";
    ss << "Total Unicast Subscribers: " << unicastEventHandlers_.size();
    return ss.str();
}

template<typename EventKeyType, typename Hash>
EventBus<EventKeyType, Hash>::~EventBus()
{
    multicastEventHandlers_.clear();
    unicastEventHandlers_.clear();
    multicastOnceEventHandlers_.clear();
    unicastOnceEventHandlers_.clear();
    tokenToEventNameMap_.clear();
}