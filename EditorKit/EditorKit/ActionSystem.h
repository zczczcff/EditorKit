#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <sstream>
#include <utility>
#include <random>
#include <iomanip>
#include <algorithm>
#include "Type_Check.h"

/*
*   ！！！注意！！！
*   使用值类型作为订阅参数时，会在被调用时触发移动语义（如果有移动语义），导致只有第一个订阅触发得到资源。复杂对象尽量使用引用。
*/

// Action类型
enum class ActionHandlerType
{
    TriggerListener,     // 触发器监听器
    Validator,          // 验证器
    ValidationListener, // 验证监听器
    SequentialProcessor, // 顺序处理器
    FinalProcessor,     // 最终处理器
    CompletionListener   // 完成监听器
};

// Action句柄
template<typename KeyType>
class ActionHandle
{
private:
    uint64_t id_;
    KeyType actionKey_;
    ActionHandlerType type_;

public:
    ActionHandle(uint64_t id = 0, const KeyType& actionKey = KeyType{},
        ActionHandlerType type = ActionHandlerType::TriggerListener)
        : id_(id), actionKey_(actionKey), type_(type)
    {
    }

    uint64_t GetId() const { return id_; }
    const KeyType& GetActionKey() const { return actionKey_; }
    ActionHandlerType GetType() const { return type_; }

    bool operator==(const ActionHandle& other) const
    {
        return id_ == other.id_ && actionKey_ == other.actionKey_ && type_ == other.type_;
    }

    bool isValid() const { return id_ != 0; }

    struct Hash
    {
        size_t operator()(const ActionHandle& handle) const
        {
            return std::hash<uint64_t>{}(handle.id_) ^
                (std::hash<KeyType>{}(handle.actionKey_) << 1) ^
                (std::hash<int>{}(static_cast<int>(handle.type_)) << 2);
        }
    };
};

// 执行结果
struct ActionResult
{
    bool success;                    // 是否成功执行
    bool validationPassed;          // 验证是否通过
    std::string errorMessage;       // 错误信息
    size_t totalValidators;         // 验证器总数
    size_t passedValidators;        // 通过验证器数
    size_t totalProcessors;         // 处理器总数
    size_t executedProcessors;      // 已执行处理器数
    size_t totalListeners;          // 监听器总数
    size_t executedListeners;       // 已执行监听器数

    ActionResult()
        : success(false), validationPassed(false), totalValidators(0),
        passedValidators(0), totalProcessors(0), executedProcessors(0),
        totalListeners(0), executedListeners(0)
    {
    }

    std::string toString() const
    {
        std::stringstream ss;
        ss << "ActionResult{success:" << success
            << ", validationPassed:" << validationPassed
            << ", validators:" << passedValidators << "/" << totalValidators
            << ", processors:" << executedProcessors << "/" << totalProcessors
            << ", listeners:" << executedListeners << "/" << totalListeners;
        if (!errorMessage.empty())
        {
            ss << ", error:" << errorMessage;
        }
        ss << "}";
        return ss.str();
    }
};

// Action处理器基类
template<typename KeyType>
class IActionHandler
{
protected:
    ActionHandle<KeyType> handle_;
    std::string description_;
    int priority_;  // 执行优先级，值越小优先级越高

public:
    IActionHandler(const ActionHandle<KeyType>& handle, const std::string& description, int priority = 0)
        : handle_(handle), description_(description), priority_(priority)
    {
    }

    virtual ~IActionHandler() = default;

    const ActionHandle<KeyType>& GetHandle() const { return handle_; }
    const std::string& GetDescription() const { return description_; }
    int GetPriority() const { return priority_; }
    virtual std::string GetArgTypes() const = 0;
    virtual ActionHandlerType GetHandlerType() const = 0;
    virtual size_t GetArgCount() const = 0;
};

// 验证器处理器
template<typename KeyType, typename... Args>
class ValidatorHandler : public IActionHandler<KeyType>
{
public:
    using ValidatorFunc = std::function<bool(Args...)>;

    ValidatorHandler(const ActionHandle<KeyType>& handle, ValidatorFunc validator,
        const std::string& description, int priority = 0)
        : IActionHandler<KeyType>(handle, description, priority), validator_(std::move(validator))
    {
        
    }

    bool Validate(Args&&... args) const
    {
        return validator_(std::forward<Args>(args)...);
    }

    std::string GetArgTypes() const override
    {
        return type_check::get_template_args_info<Args...>();
    }

    ActionHandlerType GetHandlerType() const override
    {
        return ActionHandlerType::Validator;
    }

    size_t GetArgCount() const override
    {
        return sizeof...(Args);
    }

private:
    ValidatorFunc validator_;
};

// 处理器基类，用于顺序处理器、最终处理器和监听器
template<typename KeyType, typename... Args>
class ProcessorHandler : public IActionHandler<KeyType>
{
public:
    using ProcessorFunc = std::function<void(Args...)>;

    ProcessorHandler(const ActionHandle<KeyType>& handle, ProcessorFunc processor,
        ActionHandlerType type, const std::string& description, int priority = 0)
        : IActionHandler<KeyType>(handle, description, priority),
        processor_(std::move(processor)), type_(type)
    {
        
    }

    void Process(Args&&... args) const
    {
        processor_(std::forward<Args>(args)...);
    }


    std::string GetArgTypes() const override
    {
        return type_check::get_template_args_info<Args...>();
    }

    ActionHandlerType GetHandlerType() const override
    {
        return type_;
    }

    size_t GetArgCount() const override
    {
        return sizeof...(Args);
    }

private:
    ProcessorFunc processor_;
    ActionHandlerType type_;
};

// Action处理器容器
template<typename KeyType, typename... Args>
class ActionProcessorContainer
{
private:
    std::vector<std::unique_ptr<ValidatorHandler<KeyType, Args...>>> validators_;
    std::vector<std::unique_ptr<ProcessorHandler<KeyType, Args...>>> sequentialProcessors_;
    std::unique_ptr<ProcessorHandler<KeyType, Args...>> finalProcessor_;
    std::vector<std::unique_ptr<ProcessorHandler<KeyType, Args...>>> triggerListeners_;
    std::vector<std::unique_ptr<ProcessorHandler<KeyType, Args...>>> validationListeners_;
    std::vector<std::unique_ptr<ProcessorHandler<KeyType, Args...>>> completionListeners_;

public:
    // 添加处理器
    void AddValidator(std::unique_ptr<ValidatorHandler<KeyType, Args...>> validator)
    {
        validators_.push_back(std::move(validator));
        SortByPriority(validators_);
    }

    void AddSequentialProcessor(std::unique_ptr<ProcessorHandler<KeyType, Args...>> processor)
    {
        sequentialProcessors_.push_back(std::move(processor));
        SortByPriority(sequentialProcessors_);
    }

    void SetFinalProcessor(std::unique_ptr<ProcessorHandler<KeyType, Args...>> processor)
    {
        finalProcessor_ = std::move(processor);
    }

    void AddTriggerListener(std::unique_ptr<ProcessorHandler<KeyType, Args...>> listener)
    {
        triggerListeners_.push_back(std::move(listener));
        SortByPriority(triggerListeners_);
    }

    void AddValidationListener(std::unique_ptr<ProcessorHandler<KeyType, Args...>> listener)
    {
        validationListeners_.push_back(std::move(listener));
        SortByPriority(validationListeners_);
    }

    void AddCompletionListener(std::unique_ptr<ProcessorHandler<KeyType, Args...>> listener)
    {
        completionListeners_.push_back(std::move(listener));
        SortByPriority(completionListeners_);
    }

    // 移除处理器
    bool RemoveHandler(const ActionHandle<KeyType>& handle)
    {
        auto removeFromVector = [&handle](auto& vector) -> bool
        {
            for (auto it = vector.begin(); it != vector.end(); ++it)
            {
                if ((*it)->GetHandle() == handle)
                {
                    vector.erase(it);
                    return true;
                }
            }
            return false;
        };

        // 检查并移除最终处理器
        if (finalProcessor_ && finalProcessor_->GetHandle() == handle)
        {
            finalProcessor_.reset();
            return true;
        }

        return removeFromVector(validators_) ||
            removeFromVector(sequentialProcessors_) ||
            removeFromVector(triggerListeners_) ||
            removeFromVector(validationListeners_) ||
            removeFromVector(completionListeners_);
    }

    // 执行流程
    ActionResult Execute(Args&&... args)
    {
        ActionResult result;

        // 阶段1: 触发监听器（使用完美转发）
        result.totalListeners += triggerListeners_.size();
        for (const auto& listener : triggerListeners_)
        {
            try
            {
                listener->Process(std::forward<Args>(args)...);
                result.executedListeners++;
            }
            catch (const std::exception& e)
            {
                result.errorMessage = "Trigger listener error: " + std::string(e.what());
            }
        }

        // 阶段2: 验证器（使用完美转发）
        result.totalValidators = validators_.size();
        for (const auto& validator : validators_)
        {
            try
            {
                if (validator->Validate(std::forward<Args>(args)...))
                {
                    result.passedValidators++;
                }
                else
                {
                    result.validationPassed = false;
                    result.errorMessage = "Validation failed by: " + validator->GetDescription();
                    return result;
                }
            }
            catch (const std::exception& e)
            {
                result.validationPassed = false;
                result.errorMessage = "Validator error: " + std::string(e.what());
                return result;
            }
        }
        result.validationPassed = (result.totalValidators == 0) || (result.passedValidators == result.totalValidators);

        // 验证失败提前返回
        if (!result.validationPassed && result.totalValidators > 0)
        {
            return result;
        }

        // 阶段3: 验证通过监听器
        result.totalListeners += validationListeners_.size();
        for (const auto& listener : validationListeners_)
        {
            try
            {
                listener->Process(std::forward<Args>(args)...);
                result.executedListeners++;
            }
            catch (const std::exception& e)
            {
                result.errorMessage = "Validation listener error: " + std::string(e.what());
            }
        }

        // 阶段4: 顺序处理器
        result.totalProcessors = sequentialProcessors_.size();
        for (const auto& processor : sequentialProcessors_)
        {
            try
            {
                processor->Process(std::forward<Args>(args)...);
                result.executedProcessors++;
            }
            catch (const std::exception& e)
            {
                result.errorMessage = "Sequential processor error: " + std::string(e.what());
                result.success = false;
                return result;
            }
        }

        // 阶段5: 最终处理器
        if (finalProcessor_)
        {
            try
            {
                finalProcessor_->Process(std::forward<Args>(args)...);
                result.executedProcessors++;
                result.totalProcessors++;
            }
            catch (const std::exception& e)
            {
                result.errorMessage = "Final processor error: " + std::string(e.what());
                result.success = false;
                return result;
            }
        }

        // 阶段6: 完成监听器
        result.totalListeners += completionListeners_.size();
        for (const auto& listener : completionListeners_)
        {
            try
            {
                listener->Process(std::forward<Args>(args)...);
                result.executedListeners++;
            }
            catch (const std::exception& e)
            {
                result.errorMessage = "Completion listener error: " + std::string(e.what());
            }
        }

        result.success = true;
        return result;
    }

    // 获取统计信息
    size_t GetTotalHandlers() const
    {
        return validators_.size() + sequentialProcessors_.size() +
            (finalProcessor_ ? 1 : 0) + triggerListeners_.size() +
            validationListeners_.size() + completionListeners_.size();
    }

    // 获取处理器数量
    size_t GetValidatorCount() const { return validators_.size(); }
    size_t GetSequentialProcessorCount() const { return sequentialProcessors_.size(); }
    size_t GetTriggerListenerCount() const { return triggerListeners_.size(); }
    size_t GetValidationListenerCount() const { return validationListeners_.size(); }
    size_t GetCompletionListenerCount() const { return completionListeners_.size(); }
    bool HasFinalProcessor() const { return finalProcessor_ != nullptr; }

private:
    // 按优先级排序
    template<typename T>
    void SortByPriority(std::vector<std::unique_ptr<T>>& handlers)
    {
        std::sort(handlers.begin(), handlers.end(),
            [](const auto& a, const auto& b)
            {
                return a->GetPriority() < b->GetPriority();
            });
    }
};

// Action系统主类
template<typename KeyType, typename Hash = std::hash<KeyType>, typename KeyEqual = std::equal_to<KeyType>>
class ActionSystem
{
private:
    uint64_t nextHandleId_ = 1;

    // 类型擦除的处理器包装器
    class IActionProcessorWrapper
    {
    public:
        virtual ~IActionProcessorWrapper() = default;
        virtual bool RemoveHandler(const ActionHandle<KeyType>& handle) = 0;
        virtual std::string GetArgTypes() const = 0;
        virtual size_t GetArgCount() const = 0;
        virtual size_t GetTotalHandlers() const = 0;
        virtual std::string GetStatistics() const = 0;
        // 新增：字节码执行接口（支持完美转发）
        virtual ActionResult ExecuteWithForward(void* args[]) = 0;
    };

    template<typename... Args>
    class ActionProcessorWrapper : public IActionProcessorWrapper
    {
    private:
        ActionProcessorContainer<KeyType, Args...> container_;
        template<typename T, typename... Rest>
        bool CheckArgTypes(void* args[]) const
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
        bool CheckSingleArgType(void* arg) const
        {
            using RawType = std::remove_reference_t<T>;
            return arg != nullptr;
        }

        // 准备参数指针数组（借鉴EventBus）
        template<typename Tuple, size_t... Is>
        void PrepareArgPointers(void* pointers[], Tuple& tuple, std::index_sequence<Is...>) const
        {
            ((pointers[Is] = static_cast<void*>(&std::get<Is>(tuple))), ...);
        }

        // 带参数执行的实现
        template<size_t... Is>
        ActionResult ExecuteWithArgs(void* args[], std::index_sequence<Is...>)
        {
            // 类型安全检查
            if (!CheckArgTypes<Args...>(args))
            {
                ActionResult result;
                result.errorMessage = "Parameter type mismatch in execution";
                return result;
            }

            // 使用完美转发执行
            return container_.Execute(
                std::forward<Args>(
                    *static_cast<std::remove_reference_t<Args>*>(args[Is])
                    )...
            );
        }
    public:
        ActionResult Execute(Args... args)
        {
            return container_.Execute(std::forward<Args>(args)...);
        }

        // 实现字节码执行接口
        ActionResult ExecuteWithForward(void* args[]) override
        {
            if constexpr (sizeof...(Args) == 0)
            {
                return container_.Execute();
            }
            else
            {
                return ExecuteWithArgs(args, std::index_sequence_for<Args...>{});
            }
        }

        void AddValidator(const ActionHandle<KeyType>& handle,
            typename ValidatorHandler<KeyType, Args...>::ValidatorFunc validator,
            const std::string& description, int priority)
        {
            container_.AddValidator(std::make_unique<ValidatorHandler<KeyType, Args...>>(
                handle, std::move(validator), description, priority));
        }

        void AddSequentialProcessor(const ActionHandle<KeyType>& handle,
            typename ProcessorHandler<KeyType, Args...>::ProcessorFunc processor,
            const std::string& description, int priority)
        {
            container_.AddSequentialProcessor(std::make_unique<ProcessorHandler<KeyType, Args...>>(
                handle, std::move(processor), ActionHandlerType::SequentialProcessor,
                description, priority));
        }

        void SetFinalProcessor(const ActionHandle<KeyType>& handle,
            typename ProcessorHandler<KeyType, Args...>::ProcessorFunc processor,
            const std::string& description, int priority)
        {
            container_.SetFinalProcessor(std::make_unique<ProcessorHandler<KeyType, Args...>>(
                handle, std::move(processor), ActionHandlerType::FinalProcessor,
                description, priority));
        }

        void AddListener(const ActionHandle<KeyType>& handle,
            typename ProcessorHandler<KeyType, Args...>::ProcessorFunc listener,
            ActionHandlerType type, const std::string& description, int priority)
        {
            auto handler = std::make_unique<ProcessorHandler<KeyType, Args...>>(
                handle, std::move(listener), type, description, priority);

            switch (type)
            {
            case ActionHandlerType::TriggerListener:
                container_.AddTriggerListener(std::move(handler));
                break;
            case ActionHandlerType::ValidationListener:
                container_.AddValidationListener(std::move(handler));
                break;
            case ActionHandlerType::CompletionListener:
                container_.AddCompletionListener(std::move(handler));
                break;
            default:
                break;
            }
        }

        bool RemoveHandler(const ActionHandle<KeyType>& handle) override
        {
            return container_.RemoveHandler(handle);
        }

        std::string GetArgTypes() const override
        {
            return type_check::get_template_args_info<Args...>();
        }

        size_t GetArgCount() const override
        {
            return sizeof...(Args);
        }

        size_t GetTotalHandlers() const override
        {
            return container_.GetTotalHandlers();
        }

        std::string GetStatistics() const override
        {
            std::stringstream ss;
            ss << "ActionProcessor<" << GetArgTypes() << "> Statistics:\n";
            ss << "  Validators: " << container_.GetValidatorCount() << "\n";
            ss << "  Sequential Processors: " << container_.GetSequentialProcessorCount() << "\n";
            ss << "  Final Processor: " << (container_.HasFinalProcessor() ? "Yes" : "No") << "\n";
            ss << "  Trigger Listeners: " << container_.GetTriggerListenerCount() << "\n";
            ss << "  Validation Listeners: " << container_.GetValidationListenerCount() << "\n";
            ss << "  Completion Listeners: " << container_.GetCompletionListenerCount() << "\n";
            ss << "  Total Handlers: " << GetTotalHandlers();
            return ss.str();
        }
    };

    std::unordered_map<KeyType, std::unique_ptr<IActionProcessorWrapper>, Hash, KeyEqual> actions_;
    std::unordered_map<ActionHandle<KeyType>, KeyType, typename ActionHandle<KeyType>::Hash> handleToActionMap_;

    // 新增：全局动作监听器容器
    using GlobalCompletionListener = std::function<void(const KeyType&, const ActionResult&)>;
    struct GlobalListenerInfo
    {
        GlobalCompletionListener listener;
        std::string description;
        int priority;
        uint64_t id;
    };
    std::vector<GlobalListenerInfo> globalCompletionListeners_;
    uint64_t nextGlobalListenerId_ = 1;

    // 新增：通知全局监听器的辅助方法
    void NotifyGlobalListeners(const KeyType& actionKey, const ActionResult& result)
    {
        for (const auto& listenerInfo : globalCompletionListeners_)
        {
            try
            {
                listenerInfo.listener(actionKey, result);
            }
            catch (const std::exception& e)
            {
                // 全局监听器的异常不应该影响主流程，只记录到错误流
                std::cerr << "Global completion listener error: " << e.what() << std::endl;
            }
        }
    }

    // 内部实现方法，支持lambda
    template<typename Callable>
    ActionHandle<KeyType> AddHandlerImpl(const KeyType& actionKey, Callable&& handler,
        ActionHandlerType type, const std::string& description, int priority)
    {
        using traits = function_traits<std::decay_t<Callable>>;

        // 根据参数数量分派到具体实现
        if constexpr (traits::arity == 0)
        {
            return AddHandlerDetailed<>(actionKey,
                std::function<void()>(std::forward<Callable>(handler)),
                type, description, priority);
        }
        else if constexpr (traits::arity == 1)
        {
            return AddHandlerDetailed<typename traits::template arg_type<0>>(
                actionKey,
                std::function<void(typename traits::template arg_type<0>)>(
                    std::forward<Callable>(handler)),
                type, description, priority);
        }
        else if constexpr (traits::arity == 2)
        {
            return AddHandlerDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>>(
                    actionKey,
                    std::function<void(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>)>(
                            std::forward<Callable>(handler)),
                    type, description, priority);
        }
        else if constexpr (traits::arity == 3)
        {
            return AddHandlerDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>>(
                    actionKey,
                    std::function<void(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>)>(
                            std::forward<Callable>(handler)),
                    type, description, priority);
        }
        else if constexpr (traits::arity == 4)
        {
            return AddHandlerDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>>(
                    actionKey,
                    std::function<void(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>,
                        typename traits::template arg_type<3>)>(
                            std::forward<Callable>(handler)),
                    type, description, priority);
        }
        else if constexpr (traits::arity == 5)
        {
            return AddHandlerDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>,
                typename traits::template arg_type<4>>(
                    actionKey,
                    std::function<void(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>,
                        typename traits::template arg_type<3>,
                        typename traits::template arg_type<4>)>(
                            std::forward<Callable>(handler)),
                    type, description, priority);
        }
        else if constexpr (traits::arity == 6)
        {
            return AddHandlerDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>,
                typename traits::template arg_type<4>,
                typename traits::template arg_type<5>>(
                    actionKey,
                    std::function<void(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>,
                        typename traits::template arg_type<3>,
                        typename traits::template arg_type<4>,
                        typename traits::template arg_type<5>)>(
                            std::forward<Callable>(handler)),
                    type, description, priority);
        }
        else if constexpr (traits::arity == 7)
        {
            return AddHandlerDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>,
                typename traits::template arg_type<4>,
                typename traits::template arg_type<5>,
                typename traits::template arg_type<6>>(
                    actionKey,
                    std::function<void(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>,
                        typename traits::template arg_type<3>,
                        typename traits::template arg_type<4>,
                        typename traits::template arg_type<5>,
                        typename traits::template arg_type<6>)>(
                            std::forward<Callable>(handler)),
                    type, description, priority);
        }
        else if constexpr (traits::arity == 8)
        {
            return AddHandlerDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>,
                typename traits::template arg_type<4>,
                typename traits::template arg_type<5>,
                typename traits::template arg_type<6>,
                typename traits::template arg_type<7>>(
                    actionKey,
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
                    type, description, priority);
        }
        else if constexpr (traits::arity == 9)
        {
            return AddHandlerDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>,
                typename traits::template arg_type<4>,
                typename traits::template arg_type<5>,
                typename traits::template arg_type<6>,
                typename traits::template arg_type<7>,
                typename traits::template arg_type<8>>(
                    actionKey,
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
                    type, description, priority);
        }
        else
        {
            static_assert(traits::arity <= 9, "最多支持9个参数");
            return ActionHandle<KeyType>();
        }
    }

    // 详细的添加处理器实现
    template<typename... Args>
    ActionHandle<KeyType> AddHandlerDetailed(const KeyType& actionKey,
        std::function<void(Args...)> handler,
        ActionHandlerType type,
        const std::string& description,
        int priority)
    {
        ActionHandle<KeyType> handle(nextHandleId_++, actionKey, type);

        auto* processor = GetOrCreateProcessor<Args...>(actionKey);

        switch (type)
        {
        case ActionHandlerType::Validator:
            // 验证器需要特殊的处理，因为签名应该是bool(Args...)
            // 这里需要将void(Args...)转换为bool(Args...)
            processor->AddValidator(handle,
                [handler = std::move(handler)](Args... args) -> bool {
                handler(args...);
                return true;
            }, description, priority);
            break;

        case ActionHandlerType::SequentialProcessor:
            processor->AddSequentialProcessor(handle, std::move(handler), description, priority);
            break;

        case ActionHandlerType::FinalProcessor:
            processor->SetFinalProcessor(handle, std::move(handler), description, priority);
            break;

        case ActionHandlerType::TriggerListener:
        case ActionHandlerType::ValidationListener:
        case ActionHandlerType::CompletionListener:
            processor->AddListener(handle, std::move(handler), type, description, priority);
            break;
        }

        handleToActionMap_[handle] = actionKey;
        return handle;
    }

    // 验证器的特殊处理（返回bool）
    template<typename Callable>
    ActionHandle<KeyType> AddValidatorImpl(const KeyType& actionKey, Callable&& validator,
        const std::string& description, int priority)
    {
        using traits = function_traits<std::decay_t<Callable>>;
        static_assert(std::is_same_v<typename traits::return_type, bool>,
            "Validator must return bool type  验证器必须返回bool类型");

        // 根据参数数量分派
        if constexpr (traits::arity == 0)
        {
            return AddValidatorDetailed<>(actionKey,
                std::function<bool()>(std::forward<Callable>(validator)),
                description, priority);
        }
        else if constexpr (traits::arity == 1)
        {
            return AddValidatorDetailed<typename traits::template arg_type<0>>(
                actionKey,
                std::function<bool(typename traits::template arg_type<0>)>(
                    std::forward<Callable>(validator)),
                description, priority);
        }
        else if constexpr (traits::arity == 2)
        {
            return AddValidatorDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>>(
                    actionKey,
                    std::function<bool(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>)>(
                            std::forward<Callable>(validator)),
                    description, priority);
        }
        else if constexpr (traits::arity == 3)
        {
            return AddValidatorDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>>(
                    actionKey,
                    std::function<bool(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>)>(
                            std::forward<Callable>(validator)),
                    description, priority);
        }
        else if constexpr (traits::arity == 4)
        {
            return AddValidatorDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>>(
                    actionKey,
                    std::function<bool(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>,
                        typename traits::template arg_type<3>)>(
                            std::forward<Callable>(validator)),
                    description, priority);
        }
        else if constexpr (traits::arity == 5)
        {
            return AddValidatorDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>,
                typename traits::template arg_type<4>>(
                    actionKey,
                    std::function<bool(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>,
                        typename traits::template arg_type<3>,
                        typename traits::template arg_type<4>)>(
                            std::forward<Callable>(validator)),
                    description, priority);
        }
        else if constexpr (traits::arity == 6)
        {
            return AddValidatorDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>,
                typename traits::template arg_type<4>,
                typename traits::template arg_type<5>>(
                    actionKey,
                    std::function<bool(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>,
                        typename traits::template arg_type<3>,
                        typename traits::template arg_type<4>,
                        typename traits::template arg_type<5>)>(
                            std::forward<Callable>(validator)),
                    description, priority);
        }
        else if constexpr (traits::arity == 7)
        {
            return AddValidatorDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>,
                typename traits::template arg_type<4>,
                typename traits::template arg_type<5>,
                typename traits::template arg_type<6>>(
                    actionKey,
                    std::function<bool(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>,
                        typename traits::template arg_type<3>,
                        typename traits::template arg_type<4>,
                        typename traits::template arg_type<5>,
                        typename traits::template arg_type<6>)>(
                            std::forward<Callable>(validator)),
                    description, priority);
        }
        else if constexpr (traits::arity == 8)
        {
            return AddValidatorDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>,
                typename traits::template arg_type<4>,
                typename traits::template arg_type<5>,
                typename traits::template arg_type<6>,
                typename traits::template arg_type<7>>(
                    actionKey,
                    std::function<bool(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>,
                        typename traits::template arg_type<3>,
                        typename traits::template arg_type<4>,
                        typename traits::template arg_type<5>,
                        typename traits::template arg_type<6>,
                        typename traits::template arg_type<7>)>(
                            std::forward<Callable>(validator)),
                    description, priority);
        }
        else if constexpr (traits::arity == 9)
        {
            return AddValidatorDetailed<
                typename traits::template arg_type<0>,
                typename traits::template arg_type<1>,
                typename traits::template arg_type<2>,
                typename traits::template arg_type<3>,
                typename traits::template arg_type<4>,
                typename traits::template arg_type<5>,
                typename traits::template arg_type<6>,
                typename traits::template arg_type<7>,
                typename traits::template arg_type<8>>(
                    actionKey,
                    std::function<bool(
                        typename traits::template arg_type<0>,
                        typename traits::template arg_type<1>,
                        typename traits::template arg_type<2>,
                        typename traits::template arg_type<3>,
                        typename traits::template arg_type<4>,
                        typename traits::template arg_type<5>,
                        typename traits::template arg_type<6>,
                        typename traits::template arg_type<7>,
                        typename traits::template arg_type<8>)>(
                            std::forward<Callable>(validator)),
                    description, priority);
        }
        else
        {
        static_assert(traits::arity <= 9, "只支持最多9个参数");
        return ActionHandle<KeyType>();
        }
    }

    template<typename... Args>
    ActionHandle<KeyType> AddValidatorDetailed(const KeyType& actionKey,
        std::function<bool(Args...)> validator,
        const std::string& description,
        int priority)
    {
        ActionHandle<KeyType> handle(nextHandleId_++, actionKey, ActionHandlerType::Validator);

        auto* processor = GetOrCreateProcessor<Args...>(actionKey);
        processor->AddValidator(handle, std::move(validator), description, priority);

        handleToActionMap_[handle] = actionKey;
        return handle;
    }

    template<typename... Args>
    ActionProcessorWrapper<Args...>* GetOrCreateProcessor(const KeyType& actionKey)
    {
        auto it = actions_.find(actionKey);
        if (it == actions_.end())
        {
            auto wrapper = std::make_unique<ActionProcessorWrapper<Args...>>();
            auto* ptr = wrapper.get();
            actions_[actionKey] = std::move(wrapper);
            return ptr;
        }

        auto* existing = dynamic_cast<ActionProcessorWrapper<Args...>*>(it->second.get());
        if (!existing)
        {
            throw std::runtime_error("Action parameter type mismatch for key");
        }

        return existing;
    }

public:
    // ========== 支持lambda表达式的简化接口 ==========

    // 验证器lambda版本
    template<typename Callable>
    ActionHandle<KeyType> AddValidator(const KeyType& actionKey, Callable&& validator,
        const std::string& description = "", int priority = 0)
    {
        return AddValidatorImpl(actionKey, std::forward<Callable>(validator), description, priority);
    }

    // 顺序处理器lambda版本
    template<typename Callable>
    ActionHandle<KeyType> AddSequentialProcessor(const KeyType& actionKey, Callable&& processor,
        const std::string& description = "", int priority = 0)
    {
        return AddHandlerImpl(actionKey, std::forward<Callable>(processor),
            ActionHandlerType::SequentialProcessor, description, priority);
    }

    // 最终处理器lambda版本
    template<typename Callable>
    ActionHandle<KeyType> SetFinalProcessor(const KeyType& actionKey, Callable&& processor,
        const std::string& description = "", int priority = 0)
    {
        return AddHandlerImpl(actionKey, std::forward<Callable>(processor),
            ActionHandlerType::FinalProcessor, description, priority);
    }

    // 触发器监听器lambda版本
    template<typename Callable>
    ActionHandle<KeyType> AddTriggerListener(const KeyType& actionKey, Callable&& listener,
        const std::string& description = "", int priority = 0)
    {
        return AddHandlerImpl(actionKey, std::forward<Callable>(listener),
            ActionHandlerType::TriggerListener, description, priority);
    }

    // 验证通过监听器lambda版本
    template<typename Callable>
    ActionHandle<KeyType> AddValidationListener(const KeyType& actionKey, Callable&& listener,
        const std::string& description = "", int priority = 0)
    {
        return AddHandlerImpl(actionKey, std::forward<Callable>(listener),
            ActionHandlerType::ValidationListener, description, priority);
    }

    // 完成监听器lambda版本
    template<typename Callable>
    ActionHandle<KeyType> AddCompletionListener(const KeyType& actionKey, Callable&& listener,
        const std::string& description = "", int priority = 0)
    {
        return AddHandlerImpl(actionKey, std::forward<Callable>(listener),
            ActionHandlerType::CompletionListener, description, priority);
    }

    // ========== 传统的std::function接口 ==========

    // 验证器std::function版本
    template<typename... Args>
    ActionHandle<KeyType> AddValidator(const KeyType& actionKey,
        std::function<bool(Args...)> validator,
        const std::string& description = "",
        int priority = 0)
    {
        ActionHandle<KeyType> handle(nextHandleId_++, actionKey, ActionHandlerType::Validator);

        auto* processor = GetOrCreateProcessor<Args...>(actionKey);
        processor->AddValidator(handle, std::move(validator), description, priority);

        handleToActionMap_[handle] = actionKey;
        return handle;
    }

    // 顺序处理器std::function版本
    template<typename... Args>
    ActionHandle<KeyType> AddSequentialProcessor(const KeyType& actionKey,
        std::function<void(Args...)> processor,
        const std::string& description = "",
        int priority = 0)
    {
        ActionHandle<KeyType> handle(nextHandleId_++, actionKey, ActionHandlerType::SequentialProcessor);

        auto* processorWrapper = GetOrCreateProcessor<Args...>(actionKey);
        processorWrapper->AddSequentialProcessor(handle, std::move(processor), description, priority);

        handleToActionMap_[handle] = actionKey;
        return handle;
    }

    // 最终处理器std::function版本
    template<typename... Args>
    ActionHandle<KeyType> SetFinalProcessor(const KeyType& actionKey,
        std::function<void(Args...)> processor,
        const std::string& description = "",
        int priority = 0)
    {
        ActionHandle<KeyType> handle(nextHandleId_++, actionKey, ActionHandlerType::FinalProcessor);

        auto* processorWrapper = GetOrCreateProcessor<Args...>(actionKey);
        processorWrapper->SetFinalProcessor(handle, std::move(processor), description, priority);

        handleToActionMap_[handle] = actionKey;
        return handle;
    }

    // 监听器std::function版本
    template<typename... Args>
    ActionHandle<KeyType> AddListener(const KeyType& actionKey,
        std::function<void(Args...)> listener,
        ActionHandlerType type,
        const std::string& description = "",
        int priority = 0)
    {
        ActionHandle<KeyType> handle(nextHandleId_++, actionKey, type);

        auto* processorWrapper = GetOrCreateProcessor<Args...>(actionKey);
        processorWrapper->AddListener(handle, std::move(listener), type, description, priority);

        handleToActionMap_[handle] = actionKey;
        return handle;
    }

    // 添加全局动作监听器
    uint64_t AddGlobalCompletionListener(GlobalCompletionListener listener,
        const std::string& description = "",
        int priority = 0)
    {
        uint64_t id = nextGlobalListenerId_++;
        globalCompletionListeners_.push_back({ std::move(listener), description, priority, id });

        // 按优先级排序
        std::sort(globalCompletionListeners_.begin(), globalCompletionListeners_.end(),
            [](const GlobalListenerInfo& a, const GlobalListenerInfo& b)
            {
                return a.priority < b.priority;
            });

        return id;
    }

    // 移除全局动作监听器
    bool RemoveGlobalCompletionListener(uint64_t listenerId)
    {
        auto it = std::find_if(globalCompletionListeners_.begin(), globalCompletionListeners_.end(),
            [listenerId](const GlobalListenerInfo& info)
            {
                return info.id == listenerId;
            });

        if (it != globalCompletionListeners_.end())
        {
            globalCompletionListeners_.erase(it);
            return true;
        }
        return false;
    }


    // 执行动作
    template<typename... Args>
    ActionResult Execute(const KeyType& actionKey, Args&&... args)
    {
        auto it = actions_.find(actionKey);
        if (it == actions_.end())
        {
            ActionResult result;
            result.errorMessage = "Action not found";

            // 即使找不到action，也通知全局监听器
            NotifyGlobalListeners(actionKey, result);
            return result;
        }

        // 使用类型名称验证替代dynamic_cast
        std::string expectedArgTypes = type_check::get_template_args_info<std::decay_t<Args>...>();
        if (it->second->GetArgTypes() != expectedArgTypes)
        {
            ActionResult result;
            result.errorMessage = "Action parameter type mismatch. Expected: " +
                it->second->GetArgTypes() + ", but got: " + expectedArgTypes;

            // 类型不匹配时也通知全局监听器
            NotifyGlobalListeners(actionKey, result);
            return result;
        }

        // 检查参数数量
        if (it->second->GetArgCount() != sizeof...(Args))
        {
            ActionResult result;
            result.errorMessage = "Action parameter count mismatch. Expected: " +
                std::to_string(it->second->GetArgCount()) +
                ", but got: " + std::to_string(sizeof...(Args));

            NotifyGlobalListeners(actionKey, result);
            return result;
        }

        // 准备参数存储和指针数组（支持完美转发）
        //auto argStorage = std::make_tuple(std::forward<Args>(args)...);//不再创建副本
        void* argPointers[sizeof...(Args) + 1] = {};

        // 准备参数指针
        PrepareArgPointers(argPointers, std::tie(args...), std::index_sequence_for<Args...>{});

        // 执行action
        ActionResult result = it->second->ExecuteWithForward(argPointers);

        // 通知全局监听器
        NotifyGlobalListeners(actionKey, result);

        return result;
    }

    // 添加参数指针准备辅助函数
    template<typename Tuple, size_t... Is>
    void PrepareArgPointers(void* pointers[], Tuple&& tuple, std::index_sequence<Is...>)
    {
        ((pointers[Is] = (void*)(&std::get<Is>(tuple))), ...);
    }

    // 移除处理器
    bool RemoveHandler(const ActionHandle<KeyType>& handle)
    {
        auto it = handleToActionMap_.find(handle);
        if (it == handleToActionMap_.end())
        {
            return false;
        }

        const auto& actionKey = it->second;
        auto actionIt = actions_.find(actionKey);
        if (actionIt == actions_.end())
        {
            return false;
        }

        bool removed = actionIt->second->RemoveHandler(handle);
        if (removed)
        {
            handleToActionMap_.erase(it);
        }

        return removed;
    }

    // 检查是否存在
    bool HasAction(const KeyType& actionKey) const
    {
        return actions_.find(actionKey) != actions_.end();
    }

    // 获取全局监听器数量
    size_t GetGlobalCompletionListenerCount() const
    {
        return globalCompletionListeners_.size();
    }

    // 清空全局监听器
    void ClearGlobalCompletionListeners()
    {
        globalCompletionListeners_.clear();
    }

    // 清空监听器
    void Clear()
    {
        actions_.clear();
        handleToActionMap_.clear();
        globalCompletionListeners_.clear();  // 新增
        nextHandleId_ = 1;
        nextGlobalListenerId_ = 1;  // 新增
    }

    // 统计信息方法
    std::string GetStatistics() const
    {
        std::stringstream ss;
        ss << "=== ActionSystem Statistics ===\n";
        ss << "Total Actions: " << actions_.size() << "\n";
        ss << "Total Handlers: " << handleToActionMap_.size() << "\n";
        ss << "Global Completion Listeners: " << globalCompletionListeners_.size() << "\n\n";  // 新增

        for (const auto& [actionKey, wrapper] : actions_)
        {
            ss << "Action Key: " << " [type: " << typeid(KeyType).name() << "]\n";
            ss << wrapper->GetStatistics() << "\n\n";
        }

        ss << "===============================";
        return ss.str();
    }
};

// 为常用键类型提供别名
using StringActionSystem = ActionSystem<std::string>;
using IntActionSystem = ActionSystem<int>;
using EnumActionSystem = ActionSystem<int>; // 用于枚举类型

