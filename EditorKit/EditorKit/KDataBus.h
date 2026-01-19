#pragma once


#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <iostream>
#include <sstream>
#include <cstdio>

// 数据总线结果结构
struct DataBusResult
{
    bool success;  // 操作是否成功
    void* dataPtr; // 数据指针（如果成功）
    std::string errorMessage; // 错误信息
    std::string registeredType; // 注册时的类型名称
    std::string requestedType; // 请求时的类型名称

    DataBusResult(bool s = false, void* ptr = nullptr, const std::string& msg = "")
        : success(s), dataPtr(ptr), errorMessage(msg), registeredType(""), requestedType("")
    {
    }

    // 获取详细的结果信息
    std::string GetDetails() const
    {
        std::stringstream ss;
        if (success)
        {
            ss << "操作成功 - 数据类型: " << registeredType;
        }
        else
        {
            ss << "操作失败 - " << errorMessage;
            if (!registeredType.empty() && !requestedType.empty())
            {
                ss << " (注册类型: " << registeredType << ", 请求类型: " << requestedType << ")";
            }
        }
        return ss.str();
    }

    // 模板方法：安全地转换指针类型
    template<typename T>
    T* GetAs() const
    {
        if (!success || dataPtr == nullptr)
        {
            return nullptr;
        }

        // 运行时类型检查
        if (requestedType != registeredType)
        {
            return nullptr;
        }

        return static_cast<T*>(dataPtr);
    }
};

// 数据项信息结构
struct DataItemInfo
{
    void* dataPtr; // 数据指针
    std::type_index typeIndex; // 类型信息
    std::string typeName; // 类型名称
    std::string description; // 描述信息

    DataItemInfo(void* ptr, const std::type_index& index, const std::string& name, const std::string& desc = "")
        : dataPtr(ptr), typeIndex(index), typeName(name), description(desc)
    {
    }
};

// 数据总线系统 - 支持自定义键类型和哈希函数
template<typename KeyType = std::string, typename Hash = std::hash<KeyType>>
class DataBus
{
    // 添加键到字符串的转换方法
    std::string KeyToString(const KeyType& key) const
    {
        std::ostringstream oss;
        oss << key;
        return oss.str();
    }
private:
    // 存储数据项的映射表
    std::unordered_map<KeyType, DataItemInfo, Hash> dataMap_;

    // 错误处理回调函数
    std::function<void(const char*)> errorHandler_;

    // 默认错误处理函数（输出到printf）
    static void DefaultErrorHandler(const char* message)
    {
        printf("DataBus Error: %s\n", message);
    }

    // 获取类型的可读名称
    template<typename T>
    std::string GetTypeName() const
    {
        return std::string(typeid(T).name());
    }

    // 内部错误报告函数
    void ReportError(const std::string& message) const
    {
        if (errorHandler_)
        {
            errorHandler_(message.c_str());
        }
    }

public:
    DataBus() : errorHandler_(DefaultErrorHandler)
    {
    }

    virtual ~DataBus()
    {
        Clear();
    }

    // 设置错误处理函数
    void SetErrorHandler(std::function<void(const char*)> handler)
    {
        errorHandler_ = std::move(handler);
    }

    // 注册数据指针
    template<typename T>
    DataBusResult RegisterData(const KeyType& key, T* dataPtr, const std::string& description = "")
    {
        if (dataPtr == nullptr)
        {
            std::string error = "注册数据指针为空: " + KeyToString(key); 
            ReportError(error);
            return DataBusResult(false, nullptr, error);
        }

        auto it = dataMap_.find(key);
        if (it != dataMap_.end())
        {
            std::stringstream ss;
            ss << "键 '" << KeyToString(key) << "' 已存在，当前注册类型: " << it->second.typeName; 
            std::string error = ss.str();
            ReportError(error);
            return DataBusResult(false, nullptr, error);
        }

        // 创建数据项信息
        DataItemInfo info(dataPtr, std::type_index(typeid(T)), GetTypeName<T>(), description);

        // 注册到映射表
        dataMap_.emplace(key, std::move(info));

        std::stringstream ss;
        ss << "成功注册数据 - 键: " << key << ", 类型: " << GetTypeName<T>()
            << (description.empty() ? "" : ", 描述: " + description);
        ReportError(ss.str());

        return DataBusResult(true, dataPtr, "注册成功");
    }

    // 获取数据指针（带类型检查）
    template<typename T>
    DataBusResult GetData(const KeyType& key)
    {
        auto it = dataMap_.find(key);
        if (it == dataMap_.end())
        {
            std::string error = "未找到对应键: " + KeyToString(key);  // 修复这里
            ReportError(error);
            return DataBusResult(false, nullptr, error);
        }

        DataItemInfo& info = it->second;
        std::string requestedType = GetTypeName<T>();

        // 运行时类型检查
        if (info.typeIndex != std::type_index(typeid(T)))
        {
            std::stringstream ss;
            ss << "类型不匹配 - 键: " << key
                << ", 注册类型: " << info.typeName
                << ", 请求类型: " << requestedType;
            std::string error = ss.str();
            ReportError(error);

            DataBusResult result(false, nullptr, error);
            result.registeredType = info.typeName;
            result.requestedType = requestedType;
            return result;
        }

        DataBusResult result(true, info.dataPtr, "获取成功");
        result.registeredType = info.typeName;
        result.requestedType = requestedType;
        return result;
    }

    // 安全获取数据（如果类型不匹配返回nullptr）
    template<typename T>
    T* GetDataSafe(const KeyType& key)
    {
        DataBusResult result = GetData<T>(key);
        if (result.success)
        {
            return static_cast<T*>(result.dataPtr);
        }
        return nullptr;
    }

    // 检查键是否存在
    bool HasData(const KeyType& key) const
    {
        return dataMap_.find(key) != dataMap_.end();
    }

    std::string GetDataType(const KeyType& key) const
    {
        auto it = dataMap_.find(key);
        if (it != dataMap_.end())
        {
            return it->second.typeName;
        }
        return "未找到键: " + KeyToString(key);  // 修复这里
    }

    std::string GetDataDescription(const KeyType& key) const
    {
        auto it = dataMap_.find(key);
        if (it != dataMap_.end())
        {
            return it->second.description;
        }
        return "未找到键: " + KeyToString(key);  // 修复这里
    }

    // 注销数据
    bool UnregisterData(const KeyType& key)
    {
        auto it = dataMap_.find(key);
        if (it != dataMap_.end())
        {
            std::stringstream ss;
            ss << "注销数据 - 键: " << key << ", 类型: " << it->second.typeName;
            ReportError(ss.str());

            dataMap_.erase(it);
            return true;
        }

        std::string error = "注销失败，未找到键: " + KeyToString(key);  
        ReportError(error);
        return false;
    }

    // 清空所有数据
    void Clear()
    {
        if (!dataMap_.empty())
        {
            std::stringstream ss;
            ss << "清空数据总线，共 " << dataMap_.size() << " 个数据项";
            ReportError(ss.str());

            dataMap_.clear();
        }
    }

    // 获取数据项数量
    size_t GetDataCount() const
    {
        return dataMap_.size();
    }

    // 获取所有数据的统计信息
    std::string GetStatistics() const
    {
        std::stringstream ss;
        ss << "=== DataBus 统计信息 ===" << std::endl;
        ss << "数据项总数: " << dataMap_.size() << std::endl;

        for (const auto& [key, info] : dataMap_)
        {
            ss << "键: " << key << " | 类型: " << info.typeName;
            if (!info.description.empty())
            {
                ss << " | 描述: " << info.description;
            }
            ss << std::endl;
        }
        ss << "=======================";

        return ss.str();
    }

    // 获取所有键的列表
    std::vector<KeyType> GetAllKeys() const
    {
        std::vector<KeyType> keys;
        keys.reserve(dataMap_.size());

        for (const auto& pair : dataMap_)
        {
            keys.push_back(pair.first);
        }

        return keys;
    }

    // 检查数据类型是否匹配
    template<typename T>
    bool CheckDataType(const KeyType& key) const
    {
        auto it = dataMap_.find(key);
        if (it == dataMap_.end())
        {
            return false;
        }

        return it->second.typeIndex == std::type_index(typeid(T));
    }
};
