#pragma once
#include "CommandBase.h"
#include <vector>
#include <memory>

template<typename DataType>
class EditCommandManager
{
private:
    std::vector<std::unique_ptr<CommandBase<DataType>>> m_UndoStack;
    std::vector<std::unique_ptr<CommandBase<DataType>>> m_RedoStack;
    size_t m_MaxStackSize = 100;
    bool bIsUndoRedo = false;

public:
    bool Execute(std::unique_ptr<CommandBase<DataType>> command)
    {
        if (bIsUndoRedo) // 撤销重做中，直接执行并退出
        {
            return command->Execute();
        }

        // 检查是否可以与上一个命令合并
        if (!m_UndoStack.empty() && m_UndoStack.back()->CanMergeWith(command.get()))
        {
            // 尝试合并命令
            if (m_UndoStack.back()->MergeWith(std::move(command)))
            {
                // 合并成功，重新执行合并后的命令以确保状态正确
                m_UndoStack.back()->Execute();
                return true;
            }
        }

        // 无法合并，正常执行命令
        if (command->Execute())
        {
            m_UndoStack.push_back(std::move(command));

            // 限制撤销栈大小
            if (m_UndoStack.size() > m_MaxStackSize)
            {
                m_UndoStack.erase(m_UndoStack.begin());
            }

            m_RedoStack.clear();
            return true;
        }
        return false;
    }

    bool Undo()
    {
        if (m_UndoStack.empty()) return false;
        bIsUndoRedo = true;
        auto& command = m_UndoStack.back();

        bool success = command->Undo();

        // 移动到重做栈
        m_RedoStack.push_back(std::move(command));
        m_UndoStack.pop_back();
        bIsUndoRedo = false;
        return success;
    }

    bool Redo()
    {
        if (m_RedoStack.empty()) return false;
        bIsUndoRedo = true;
        auto& command = m_RedoStack.back();
        bool success = command->Execute();

        // 移回撤销栈
        m_UndoStack.push_back(std::move(command));
        m_RedoStack.pop_back();
        bIsUndoRedo = false;
        return success;
    }

    void Clear()
    {
        m_UndoStack.clear();
        m_RedoStack.clear();
    }

    bool CanUndo() const { return !m_UndoStack.empty(); }
    bool CanRedo() const { return !m_RedoStack.empty(); }

    // 获取栈大小
    size_t GetUndoStackSize() const { return m_UndoStack.size(); }
    size_t GetRedoStackSize() const { return m_RedoStack.size(); }

    // 设置最大栈大小
    void SetMaxStackSize(size_t maxSize) { m_MaxStackSize = maxSize; }
};