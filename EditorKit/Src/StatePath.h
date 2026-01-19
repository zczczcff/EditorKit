#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <variant>
#include <functional>
#include <cassert>
#include "StatePathListener.h"
#include "StateNode.h"




// 路径状态系统
class StatePath
{
private:
    ObjectNode* root;
    EventManager eventManager;
    bool enableEvents = true;
    std::function<void(const char*)> errorCallback; // 错误回调
        // 默认错误处理函数
    static void defaultErrorHandler(const char* errorMsg)
    {
        std::cout << "StatePath Error: " << errorMsg << std::endl;
    }

    // 触发错误回调
    void triggerError(const std::string& errorMsg) const
    {
        if (errorCallback)
        {
            errorCallback(errorMsg.c_str());
        }
        else
        {
            defaultErrorHandler(errorMsg.c_str());
        }
    }

    // 触发事件
    void triggerEvent(EventType type, const std::string& path,
        const std::string& relatedPath = "",
        BaseNode* node = nullptr)
    {
        if (!enableEvents) return;

        PathEvent event;
        event.type = type;
        event.path = path;
        event.relatedPath = relatedPath;
        event.node = node;
        event.nodeType = node ? node->getType() : NodeType::EMPTY;

        auto listeners = eventManager.findListeners(path, type);
        for (const auto& listener : listeners)
        {
            listener.callback(event);
        }
    }
public:
    // 添加事件监听器
    ListenerId addEventListener(const std::string& path, ListenGranularity granularity,
        EventType eventType, EventCallback callback)
    {
        return eventManager.addListener(path, granularity, eventType, callback);
    }

    // 移除事件监听器
    bool removeEventListener(ListenerId id)
    {
        return eventManager.removeListener(id);
    }

    // 启用/禁用事件
    void setEventEnabled(bool enabled)
    {
        enableEvents = enabled;
    }
private:
    // 分割路径
    std::vector<std::string> splitPath(const std::string& path) const
    {
        std::vector<std::string> parts;
        std::stringstream ss(path);
        std::string part;

        while (std::getline(ss, part, '/'))
        {
            if (!part.empty())
            {
                parts.push_back(part);
            }
        }

        return parts;
    }

    // 组合路径
    std::string combinePath(const std::string& base, const std::string& relative) const
    {
        if (base.empty()) return relative;
        if (relative.empty()) return base;
        return base + "/" + relative;
    }

    // 递归更新节点及其子节点的绝对路径
    void updateNodePath(BaseNode* node, const std::string& newPath)
    {
        if (node->getType() == NodeType::OBJECT)
        {
            ObjectNode* objNode = static_cast<ObjectNode*>(node);
            objNode->setAbsolutePath(newPath);

            // 递归更新子节点
            objNode->forEachChild([&](const std::string& name, BaseNode* child)
                {
                    updateNodePath(child, combinePath(newPath, name));
                });
        }
    }

    // 获取父节点和最后一部分名称（自动创建中间节点）
    std::pair<ObjectNode*, std::string> getParentAndName(const std::string& path)
    {
        std::vector<std::string> parts = splitPath(path);

        if (parts.empty())
        {
            return { nullptr, "" };
        }

        ObjectNode* current = root;
        std::string lastName = parts.back();

        for (size_t i = 0; i < parts.size() - 1; ++i)
        {
            BaseNode* node = current->getChild(parts[i]);
            if (!node)
            {
                ObjectNode* newObj = new ObjectNode(this, combinePath(current->getAbsolutePath(), parts[i]));
                current->addChild(parts[i], newObj);
                current = newObj;
            }
            else if (node->getType() != NodeType::OBJECT)
            {
                ObjectNode* newObj = new ObjectNode(this, combinePath(current->getAbsolutePath(), parts[i]));
                current->addChild(parts[i], newObj);
                current = newObj;
            }
            else
            {
                current = static_cast<ObjectNode*>(node);
            }
        }

        return { current, lastName };
    }

    // 获取节点（不自动创建）
    std::pair<ObjectNode*, std::string> getParentAndNameNoCreate(const std::string& path) const
    {
        std::vector<std::string> parts = splitPath(path);

        if (parts.empty())
        {
            return { nullptr, "" };
        }

        ObjectNode* current = root;
        std::string lastName = parts.back();

        // 遍历到倒数第二部分
        for (size_t i = 0; i < parts.size() - 1; ++i)
        {
            BaseNode* node = current->getChild(parts[i]);
            if (!node || node->getType() != NodeType::OBJECT)
            {
                return { nullptr, "" };
            }
            current = static_cast<ObjectNode*>(node);
        }

        return { current, lastName };
    }

    // 内部辅助方法：安全地获取节点指针
    template<typename NodeType>
    NodeType* getNodeAs(const std::string& path)
    {
        BaseNode* node = getNode(path);
        if (node && node->getType() == NodeType::getStaticType())
        {
            return static_cast<NodeType*>(node);
        }
        return nullptr;
    }

    // 内部辅助方法：获取特定类型的值
    template<typename ValueType, typename NodeType>
    bool getTypedValue(const std::string& path, ValueType& outValue)
    {
        NodeType* node = getNodeAs<NodeType>(path);
        if (node)
        {
            outValue = node->getValue();
            return true;
        }
        return false;
    }

    // 设置节点值（不自动创建路径）
    template<typename NodeType, typename ValueType>
    bool setValueNoCreate(const std::string& path, ValueType value)
    {
        auto [parent, name] = getParentAndNameNoCreate(path);
        if (!parent || name.empty())
        {
            triggerError("Path not found when setting value: " + path);
            return false;
        }

        BaseNode* node = parent->getChild(name);
        if (!node || node->getType() != NodeType::getStaticType())
        {
            triggerError("Node type mismatch or node not found when setting value at path: " + path);
            return false;
        }

        static_cast<NodeType*>(node)->setValue(value);
        triggerEvent(EventType::UPDATE, path, "", node);
        return true;
    }

    // 获取节点的静态类型
    static NodeType getStaticTypeFor(NodeType* node) { return NodeType::OBJECT; }
    static NodeType getStaticTypeFor(IntNode* node) { return NodeType::INT; }
    static NodeType getStaticTypeFor(FloatNode* node) { return NodeType::FLOAT; }
    static NodeType getStaticTypeFor(BoolNode* node) { return NodeType::BOOL; }
    static NodeType getStaticTypeFor(PointerNode* node) { return NodeType::POINTER; }
    static NodeType getStaticTypeFor(StringNode* node) { return NodeType::STRING; }

    // 节点类型访问者
    class TypeCheckVisitor : public NodeVisitor
    {
    private:
        NodeType nodeType;
    public:
        TypeCheckVisitor() : nodeType(NodeType::EMPTY) {}

        void visit(IntNode* node) override { nodeType = NodeType::INT; }
        void visit(FloatNode* node) override { nodeType = NodeType::FLOAT; }
        void visit(BoolNode* node) override { nodeType = NodeType::BOOL; }
        void visit(PointerNode* node) override { nodeType = NodeType::POINTER; }
        void visit(StringNode* node) override { nodeType = NodeType::STRING; }
        void visit(ObjectNode* node) override { nodeType = NodeType::OBJECT; }
        void visit(BaseNode* node) override { nodeType = NodeType::EMPTY; }

        NodeType getType() const { return nodeType; }
    };

    // 值获取访问者
    template<typename T>
    class ValueGetterVisitor : public NodeVisitor
    {
    private:
        T value;
        bool valid;
    public:
        ValueGetterVisitor() : value(), valid(false) {}

        void visit(IntNode* node) override
        {
            if constexpr (std::is_same_v<T, int>)
            {
                value = node->getValue();
                valid = true;
            }
        }
        void visit(FloatNode* node) override
        {
            if constexpr (std::is_same_v<T, float>)
            {
                value = node->getValue();
                valid = true;
            }
        }
        void visit(BoolNode* node) override
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                value = node->getValue();
                valid = true;
            }
        }
        void visit(PointerNode* node) override
        {
            if constexpr (std::is_same_v<T, void*>)
            {
                value = node->getValue();
                valid = true;
            }
        }
        void visit(StringNode* node) override
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                value = node->getValue();
                valid = true;
            }
        }
        void visit(ObjectNode* node) override { valid = false; }
        void visit(BaseNode* node) override { valid = false; }

        T getValue() const { return value; }
        bool isValid() const { return valid; }
    };

public:
    StatePath() : errorCallback(defaultErrorHandler)
    {
        root = new ObjectNode(this, ""); // 根节点绝对路径为空
    }

    ~StatePath()
    {
        delete root;
    }

    // 设置错误回调
    void setErrorCallback(std::function<void(const char*)> callback)
    {
        errorCallback = callback;
    }

    // 获取节点
    BaseNode* getNode(const std::string& path)
    {
        auto [parent, name] = getParentAndNameNoCreate(path);
        if (parent && !name.empty())
        {
            return parent->getChild(name);
        }
        return nullptr;
    }

    // 通过[]操作符设置值（自动创建路径）
    void setInt(const std::string& path, int value)
    {
        bool exists = hasNode(path);
        auto [parent, name] = getParentAndName(path);
        if (parent && !name.empty())
        {
            BaseNode* oldNode = parent->getChild(name);
            if (oldNode&&oldNode->getType()==NodeType::INT)
            {
                static_cast<IntNode*>(oldNode)->setValue(value);
                triggerEvent(EventType::UPDATE, path, "", oldNode);
            }
            else
            {
                // 节点不存在或类型不匹配，创建新节点
                if (oldNode && oldNode->getType() != NodeType::INT)
                {
                    triggerError("Node type mismatch when setting float value at path: " + path);
                }
                parent->addChild(name, new IntNode(value));
                triggerEvent(exists ? EventType::UPDATE : EventType::ADD, path, "", parent->getChild(name));
            }
        }
        else
        {
            triggerError("Invalid path when setting float value: " + path);
        }
    }

    void setFloat(const std::string& path, float value)
    {
        bool exists = hasNode(path);
        auto [parent, name] = getParentAndName(path);
        if (parent && !name.empty())
        {
            BaseNode* oldNode = parent->getChild(name);
            if (oldNode && oldNode->getType() == NodeType::FLOAT)
            {
                // 节点已存在且类型匹配，直接赋值
                static_cast<FloatNode*>(oldNode)->setValue(value);
                triggerEvent(EventType::UPDATE, path, "", oldNode);
            }
            else
            {
                // 节点不存在或类型不匹配，创建新节点
                if (oldNode && oldNode->getType() != NodeType::FLOAT)
                {
                    triggerError("Node type mismatch when setting float value at path: " + path);
                }
                parent->addChild(name, new FloatNode(value));
                triggerEvent(exists ? EventType::UPDATE : EventType::ADD, path, "", parent->getChild(name));
            }
        }
        else
        {
            triggerError("Invalid path when setting float value: " + path);
        }
    }

    // 类似的改进其他set函数
    void setBool(const std::string& path, bool value)
    {
        bool exists = hasNode(path);
        auto [parent, name] = getParentAndName(path);
        if (parent && !name.empty())
        {
            BaseNode* oldNode = parent->getChild(name);
            if (oldNode && oldNode->getType() == NodeType::BOOL)
            {
                static_cast<BoolNode*>(oldNode)->setValue(value);
                triggerEvent(EventType::UPDATE, path, "", oldNode);
            }
            else
            {
                if (oldNode && oldNode->getType() != NodeType::BOOL)
                {
                    triggerError("Node type mismatch when setting bool value at path: " + path);
                }
                parent->addChild(name, new BoolNode(value));
                triggerEvent(exists ? EventType::UPDATE : EventType::ADD, path, "", parent->getChild(name));
            }
        }
        else
        {
            triggerError("Invalid path when setting bool value: " + path);
        }
    }

    void setPointer(const std::string& path, void* value)
    {
        bool exists = hasNode(path);
        auto [parent, name] = getParentAndName(path);
        if (parent && !name.empty())
        {
            BaseNode* oldNode = parent->getChild(name);
            if (oldNode && oldNode->getType() == NodeType::POINTER)
            {
                static_cast<PointerNode*>(oldNode)->setValue(value);
                triggerEvent(EventType::UPDATE, path, "", oldNode);
            }
            else
            {
                if (oldNode && oldNode->getType() != NodeType::POINTER)
                {
                    triggerError("Node type mismatch when setting pointer value at path: " + path);
                }
                parent->addChild(name, new PointerNode(value));
                triggerEvent(exists ? EventType::UPDATE : EventType::ADD, path, "", parent->getChild(name));
            }
        }
        else
        {
            triggerError("Invalid path when setting pointer value: " + path);
        }
    }

    void setString(const std::string& path, const std::string& value)
    {
        bool exists = hasNode(path);
        auto [parent, name] = getParentAndName(path);
        if (parent && !name.empty())
        {
            BaseNode* oldNode = parent->getChild(name);
            if (oldNode && oldNode->getType() == NodeType::STRING)
            {
                static_cast<StringNode*>(oldNode)->setValue(value);
                triggerEvent(EventType::UPDATE, path, "", oldNode);
            }
            else
            {
                if (oldNode && oldNode->getType() != NodeType::STRING)
                {
                    triggerError("Node type mismatch when setting string value at path: " + path);
                }
                parent->addChild(name, new StringNode(value));
                triggerEvent(exists ? EventType::UPDATE : EventType::ADD, path, "", parent->getChild(name));
            }
        }
        else
        {
            triggerError("Invalid path when setting string value: " + path);
        }
    }

    void setObject(const std::string& path)
    {
        bool exists = hasNode(path);
        auto [parent, name] = getParentAndName(path);
        if (parent && !name.empty())
        {
            BaseNode* oldNode = parent->getChild(name);
            if (oldNode && oldNode->getType() == NodeType::OBJECT)
            {
                // 对象节点已存在，不需要做额外操作
                triggerEvent(EventType::UPDATE, path, "", oldNode);
            }
            else
            {
                if (oldNode && oldNode->getType() != NodeType::OBJECT)
                {
                    triggerError("Node type mismatch when setting object at path: " + path);
                }
                parent->addChild(name, new ObjectNode(this, path));
                triggerEvent(exists ? EventType::UPDATE : EventType::ADD, path, "", parent->getChild(name));
            }
        }
        else
        {
            triggerError("Invalid path when setting object: " + path);
        }
    }

    void setNode(const std::string& path, BaseNode* node)
    {
        bool exists = hasNode(path);
        auto [parent, name] = getParentAndName(path);
        if (parent && !name.empty())
        {
            BaseNode* oldNode = parent->getChild(name);
            if (oldNode && oldNode->getType() == node->getType())
            {
                // 替换现有节点
                parent->addChild(name, node);
                triggerEvent(EventType::UPDATE, path, "", node);
                delete oldNode; // 删除旧节点
            }
            else
            {
                if (oldNode && oldNode->getType() != node->getType())
                {
                    triggerError("Node type mismatch when setting node at path: " + path);
                }
                parent->addChild(name, node);
                triggerEvent(exists ? EventType::UPDATE : EventType::ADD, path, "", node);
                if (oldNode)
                {
                    delete oldNode; // 删除类型不匹配的旧节点
                }
            }
        }
        else
        {
            delete node; // 路径无效，删除节点
            triggerError("Invalid path when setting node: " + path);
        }
    }

    // 通过SetValue方法设置值（不自动创建路径，返回是否成功）
    bool TrySetIntValue(const std::string& path, int value)
    {
        return setValueNoCreate<IntNode, int>(path, value);
    }

    bool TrySetFloatValue(const std::string& path, float value)
    {
        return setValueNoCreate<FloatNode, float>(path, value);
    }

    bool TrySetBoolValue(const std::string& path, bool value)
    {
        return setValueNoCreate<BoolNode, bool>(path, value);
    }

    bool TrySetPointerValue(const std::string& path, void* value)
    {
        return setValueNoCreate<PointerNode, void*>(path, value);
    }

    bool TrySetStringValue(const std::string& path, const std::string& value)
    {
        return setValueNoCreate<StringNode, std::string>(path, value);
    }


    // 移除节点
    bool removeNode(const std::string& path)
    {
        auto [parent, name] = getParentAndNameNoCreate(path);
        if (parent && !name.empty())
        {
            BaseNode* node = parent->removeChild(name);
            if (node)
            {
                triggerEvent(EventType::REMOVE, path, "", node);
                delete node;
                return true;
            }
        }
        return false;
    }

    // 移动节点
    bool moveNode(const std::string& fromPath, const std::string& toPath)
    {
        // 获取源节点
        auto [fromParent, fromName] = getParentAndNameNoCreate(fromPath);
        if (!fromParent || fromName.empty())
        {
            return false;
        }

        BaseNode* node = fromParent->removeChild(fromName);
        if (!node)
        {
            return false;
        }

        // 获取目标位置
        auto [toParent, toName] = getParentAndName(toPath);
        if (!toParent || toName.empty())
        {
            fromParent->addChild(fromName, node);
            return false;
        }

        // 触发移动事件（只触发移动，不触发增删）
        triggerEvent(EventType::MOVE, fromPath, toPath, node);

        toParent->addChild(toName, node);
        return true;
    }

    // 检查节点是否存在
    bool hasNode(const std::string& path) const
    {
        auto [parent, name] = getParentAndNameNoCreate(path);
        if (parent && !name.empty())
        {
            return parent->hasChild(name);
        }
        return false;
    }

    // 获取节点类型
    NodeType getNodeType(const std::string& path) const
    {
        auto [parent, name] = getParentAndNameNoCreate(path);
        if (parent && !name.empty())
        {
            BaseNode* node = parent->getChild(name);
            if (node)
            {
                return node->getType();
            }
        }
        return NodeType::EMPTY;
    }

    // 遍历对象节点的子节点
    template<typename Func>
    void forEachChild(const std::string& path, Func func) const
    {
        auto [parent, name] = getParentAndNameNoCreate(path);
        if (parent && !name.empty())
        {
            BaseNode* node = parent->getChild(name);
            if (node && node->getType() == NodeType::OBJECT)
            {
                static_cast<ObjectNode*>(node)->forEachChild(func);
            }
        }
        else if (path.empty())
        {
            // 遍历根节点
            root->forEachChild(func);
        }
    }

    // 获取对象节点的子节点名称列表
    std::vector<std::string> getChildNames(const std::string& path) const
    {
        auto [parent, name] = getParentAndNameNoCreate(path);
        if (parent && !name.empty())
        {
            BaseNode* node = parent->getChild(name);
            if (node && node->getType() == NodeType::OBJECT)
            {
                return static_cast<ObjectNode*>(node)->getChildNames();
            }
        }
        else if (path.empty())
        {
            // 获取根节点的子节点
            return root->getChildNames();
        }
        return {};
    }

    // 获取值（通过访问者模式）
    bool getInt(const std::string& path, int& outValue)
    {
        return getTypedValue<int, IntNode>(path, outValue);
    }

    bool getFloat(const std::string& path, float& outValue)
    {
        return getTypedValue<float, FloatNode>(path, outValue);
    }

    bool getBool(const std::string& path, bool& outValue)
    {
        return getTypedValue<bool, BoolNode>(path, outValue);
    }

    bool getPointer(const std::string& path, void*& outValue)
    {
        return getTypedValue<void*, PointerNode>(path, outValue);
    }

    bool getString(const std::string& path, std::string& outValue)
    {
        return getTypedValue<std::string, StringNode>(path, outValue);
    }

    // 通用的值获取（通过访问者模式）
    template<typename T>
    bool getValue(const std::string& path, T& outValue)
    {
        BaseNode* node = getNode(path);
        if (!node) return false;

        ValueGetterVisitor<T> visitor;
        node->accept(visitor);
        if (visitor.isValid())
        {
            outValue = visitor.getValue();
            return true;
        }
        return false;
    }

    int GetIntValue(const std::string& path, int badValue = 0)
    {
        int ret = badValue;
        getTypedValue<int, IntNode>(path, ret);
        return ret;
    }

    // 获取浮点数值
    float GetFloatValue(const std::string& path, float badValue = 0.0f)
    {
        float ret = badValue;
        getTypedValue<float, FloatNode>(path, ret);
        return ret;
    }

    // 获取布尔值
    bool GetBoolValue(const std::string& path, bool badValue = false)
    {
        bool ret = badValue;
        getTypedValue<bool, BoolNode>(path, ret);
        return ret;
    }

    // 获取指针值
    void* GetPointerValue(const std::string& path, void* badValue = nullptr)
    {
        void* ret = badValue;
        getTypedValue<void*, PointerNode>(path, ret);
        return ret;
    }

    // 获取字符串值
    std::string GetStringValue(const std::string& path, const std::string& badValue = "")
    {
        std::string ret = badValue;
        getTypedValue<std::string, StringNode>(path, ret);
        return ret;
    }

    // 打印整个树
    std::string printTree() const
    {
        return "StatePath Tree:\n" + root->printTreeStyle("", true);
    }

    // 重载[]操作符 - 自动创建路径
    class NodeAccessor
    {
    private:
        StatePath* system;
        std::string path;

        std::string combinePath(const std::string& base, const std::string& rel) const
        {
            if (base.empty()) return rel;
            if (rel.empty()) return base;
            return base + "/" + rel;
        }
    public:
        NodeAccessor(StatePath* sys, const std::string& p) : system(sys), path(p) {}

        // 获取节点
        BaseNode* get() const
        {
            return system->getNode(path);
        }

        // 检查节点是否存在
        bool exists() const
        {
            return system->hasNode(path);
        }

        // 获取节点类型
        NodeType type() const
        {
            return system->getNodeType(path);
        }

        // 设置值（自动创建路径）
        void operator=(int value) { system->setInt(path, value); }
        void operator=(float value) { system->setFloat(path, value); }
        void operator=(bool value) { system->setBool(path, value); }
        void operator=(void* value) { system->setPointer(path, value); }
        void operator=(const std::string& value) { system->setString(path, value); }
        void operator=(const char* value) { system->setString(path, std::string(value)); }

        // 获取值
        bool get(int& outValue) const { return system->getInt(path, outValue); }
        bool get(float& outValue) const { return system->getFloat(path, outValue); }
        bool get(bool& outValue) const { return system->getBool(path, outValue); }
        bool get(void*& outValue) const { return system->getPointer(path, outValue); }
        bool get(std::string& outValue) const { return system->getString(path, outValue); }

        // 获取整数值，失败时返回默认值
        int GetIntValue(int badValue = 0)
        {
            int ret = badValue;
            system->getInt(path, ret);
            return ret;
        }

        // 获取浮点数值，失败时返回默认值
        float GetFloatValue(float badValue = 0.0f)
        {
            float ret = badValue;
            system->getFloat(path, ret);
            return ret;
        }

        // 获取布尔值，失败时返回默认值
        bool GetBoolValue(bool badValue = false)
        {
            bool ret = badValue;
            system->getBool(path, ret);
            return ret;
        }

        // 获取指针值，失败时返回默认值
        void* GetPointerValue(void* badValue = nullptr)
        {
            void* ret = badValue;
            system->getPointer(path, ret);
            return ret;
        }

        // 获取字符串值，失败时返回默认值
        std::string GetStringValue(const std::string& badValue = "")
        {
            std::string ret = badValue;
            system->getString(path, ret);
            return ret;
        }

        // 通用获取值
        template<typename T>
        bool getValue(T& outValue) const
        {
            return system->getValue<T>(path, outValue);
        }

        NodeAccessor operator[](const std::string& subPath)
        {
            return NodeAccessor(system, combinePath(path, subPath));
        }
    };

    NodeAccessor operator[](const std::string& path)
    {
        return NodeAccessor(this, path);
    }
};