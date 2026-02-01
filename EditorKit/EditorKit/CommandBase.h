#pragma once
#include <memory>
#include <vector>
#include <string>
#include <chrono>

template<typename DataType>
class CommandBase
{
protected:
    std::chrono::steady_clock::time_point m_Timestamp; // 命令时间戳
    std::string m_Description;
    DataType m_Data; // 自定义数据类型成员变量

public:
    CommandBase(const DataType& data = DataType())
        : m_Timestamp(std::chrono::steady_clock::now()), m_Data(data)
    {
    }

    virtual ~CommandBase() = default;
    virtual bool Execute() = 0;
    virtual bool Undo() = 0;
    virtual std::string GetDescription() const = 0;

    // 合并相关方法，现在可以基于m_Data进行判断
    virtual bool CanMergeWith(const CommandBase<DataType>* other) const { return false; }
    virtual bool MergeWith(std::unique_ptr<CommandBase<DataType>> other) { return false; }

    // 获取和设置数据
    const DataType& GetData() const { return m_Data; }
    void SetData(const DataType& data) { m_Data = data; }

    // 获取时间戳
    std::chrono::steady_clock::time_point GetTimestamp() const
    {
        return m_Timestamp;
    }

    // 检查是否在合并时间窗口内（默认500ms）
    bool IsWithinMergeWindow(const CommandBase<DataType>* other,
        std::chrono::milliseconds window = std::chrono::milliseconds(500)) const
    {
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            other->m_Timestamp - m_Timestamp);
        return diff <= window;
    }
};