#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
// 前向声明
class BaseNode;
class IntNode;
class FloatNode;
class BoolNode;
class PointerNode;
class StringNode;
class ObjectNode;
class StatePath;
enum class NodeType
{
    OBJECT,
    INT,
    FLOAT,
    BOOL,
    POINTER,
    STRING,
    EMPTY
};

// 节点基类
class BaseNode
{
public:
    virtual ~BaseNode() = default;
    virtual NodeType getType() const = 0;
    // 新增：简洁的节点内容描述
    virtual std::string getContent() const = 0;

    // 新增：树形打印方法（|-- 样式）
    virtual std::string printTreeStyle(const std::string& prefix = "", bool isLast = true) const = 0;

    IntNode* AsIntNode();
    FloatNode* AsFloatNode();
    BoolNode* AsBoolNode();
    PointerNode* AsPointerNode();
    StringNode* AsStringNode();
    ObjectNode* AsObjectNode();
};

// 空节点
class EmptyNode : public BaseNode
{
public:
    NodeType getType() const override { return NodeType::EMPTY; }
    static NodeType getStaticType() { return NodeType::EMPTY; }

    std::string getContent() const override
    {
        return "[Empty]";
    }

    std::string printTreeStyle(const std::string& prefix, bool isLast) const override
    {
        return prefix + (isLast ? "└── " : "├── ") + getContent() + "\n";
    }

};

// 整数节点
class IntNode : public BaseNode
{
private:
    int value;
public:
    explicit IntNode(int val) : value(val) {}

    NodeType getType() const override { return NodeType::INT; }
    static NodeType getStaticType() { return NodeType::INT; }
    int getValue() const { return value; }
    void setValue(int val) { value = val; }

    std::string getContent() const override
    {
        return "[Int: " + std::to_string(value) + "]";
    }

    std::string printTreeStyle(const std::string& prefix, bool isLast) const override
    {
        return prefix + (isLast ? "└── " : "├── ") + getContent() + "\n";
    }
};

// 浮点数节点
class FloatNode : public BaseNode
{
private:
    float value;
public:
    explicit FloatNode(float val) : value(val) {}

    NodeType getType() const override { return NodeType::FLOAT; }
    static NodeType getStaticType() { return NodeType::FLOAT; }
    float getValue() const { return value; }
    void setValue(float val) { value = val; }

    std::string getContent() const override
    {
        return "[Float: " + std::to_string(value) + "]";
    }

    std::string printTreeStyle(const std::string& prefix, bool isLast) const override
    {
        return prefix + (isLast ? "└── " : "├── ") + getContent() + "\n";
    }
};

// 布尔节点
class BoolNode : public BaseNode
{
private:
    bool value;
public:
    explicit BoolNode(bool val) : value(val) {}

    NodeType getType() const override { return NodeType::BOOL; }
    static NodeType getStaticType() { return NodeType::BOOL; }
    bool getValue() const { return value; }
    void setValue(bool val) { value = val; }

    std::string getContent() const override
    {
        return "[Bool: " + std::string(value ? "true" : "false") + "]";
    }

    std::string printTreeStyle(const std::string& prefix, bool isLast) const override
    {
        return prefix + (isLast ? "└── " : "├── ") + getContent() + "\n";
    }
};

// 指针节点
class PointerNode : public BaseNode
{
private:
    void* value;
public:
    explicit PointerNode(void* val) : value(val) {}

    NodeType getType() const override { return NodeType::POINTER; }
    static NodeType getStaticType() { return NodeType::POINTER; }
    void* getValue() const { return value; }
    void setValue(void* val) { value = val; }

    std::string getContent() const override
    {
        std::ostringstream oss;
        oss << "[Pointer: " << value << "]";
        return oss.str();
    }

    std::string printTreeStyle(const std::string& prefix, bool isLast) const override
    {
        return prefix + (isLast ? "└── " : "├── ") + getContent() + "\n";
    }
};

// 字符串节点
class StringNode : public BaseNode
{
private:
    std::string value;
public:
    explicit StringNode(const std::string& val) : value(val) {}
    explicit StringNode(const char* val) : value(val) {}

    NodeType getType() const override { return NodeType::STRING; }
    static NodeType getStaticType() { return NodeType::STRING; }
    const std::string& getValue() const { return value; }
    void setValue(const std::string& val) { value = val; }

    std::string getContent() const override
    {
        return "[String: \"" + value + "\"]";
    }

    std::string printTreeStyle(const std::string& prefix, bool isLast) const override
    {
        return prefix + (isLast ? "└── " : "├── ") + getContent() + "\n";
    }
};

// 对象节点
class ObjectNode : public BaseNode
{
private:
    friend class StatePath;
    std::unordered_map<std::string, BaseNode*> children;
    StatePath* stateSystem; // 所属的状态路径系统
    std::string absolutePath; // 节点的绝对路径
        // 构造函数私有化，只能通过StatePath创建
    ObjectNode(StatePath* system, const std::string& path);
    void setAbsolutePath(const std::string& path) { absolutePath = path; }
public:
    // 删除默认构造函数
    ObjectNode() = delete;

    NodeType getType() const override { return NodeType::OBJECT; }
    static NodeType getStaticType() { return NodeType::OBJECT; }

    // 路径信息
    std::string getAbsolutePath() const{ return absolutePath; }
    StatePath* getStateSystem() const{ return stateSystem; }

    // 相对路径操作接口
    bool setInt(const std::string& relativePath, int value);
    bool setFloat(const std::string& relativePath, float value);
    bool setBool(const std::string& relativePath, bool value);
    bool setPointer(const std::string& relativePath, void* value);
    bool setString(const std::string& relativePath, const std::string& value);
    bool setObject(const std::string& relativePath);
    BaseNode* getNode(const std::string& relativePath);
    bool hasNode(const std::string& relativePath);
    bool removeNode(const std::string& relativePath);

    // []操作符支持
    class NodeAccessor
    {
    private:
        ObjectNode* parent;
        std::string relativePath;

        std::string combinePath(const std::string& base, const std::string& rel) const
        {
            if (base.empty()) return rel;
            if (rel.empty()) return base;
            return base + "/" + rel;
        }

    public:
        NodeAccessor(ObjectNode* p, const std::string& path) : parent(p), relativePath(path) {}

        // 赋值操作
        NodeAccessor& operator=(int value)
        {
            parent->setInt(relativePath, value);
            return *this;
        }

        NodeAccessor& operator=(float value)
        {
            parent->setFloat(relativePath, value);
            return *this;
        }

        NodeAccessor& operator=(bool value)
        {
            parent->setBool(relativePath, value);
            return *this;
        }

        NodeAccessor& operator=(void* value)
        {
            parent->setPointer(relativePath, value);
            return *this;
        }

        NodeAccessor& operator=(const std::string& value)
        {
            parent->setString(relativePath, value);
            return *this;
        }

        NodeAccessor& operator=(const char* value)
        {
            parent->setString(relativePath, std::string(value));
            return *this;
        }

        // 链式[]操作符支持
        NodeAccessor operator[](const std::string& subPath)
        {
            return NodeAccessor(parent, combinePath(relativePath, subPath));
        }

        int GetIntValue(int badValue = 0)
        {
			int value = badValue;
			BaseNode* node = parent->getNode(relativePath);
			if (node && node->getType() == NodeType::INT)
			{
				value = static_cast<IntNode*>(node)->getValue();
			}
			return value;
        }

        float GetFloatValue(float badValue = 0.0f)
        {
            float value = badValue;
            BaseNode* node = parent->getNode(relativePath);
            if (node && node->getType() == NodeType::FLOAT)
            {
                value = static_cast<FloatNode*>(node)->getValue();
            }
            return value;
        }

        bool GetBoolValue(bool badValue = false)
        {
            bool value = badValue;
            BaseNode* node = parent->getNode(relativePath);
            if (node && node->getType() == NodeType::BOOL)
            {
                value = static_cast<BoolNode*>(node)->getValue();
            }
            return value;
        }

        void* GetPointerValue(void* badValue = nullptr)
        {
            void* value = badValue;
            BaseNode* node = parent->getNode(relativePath);
            if (node && node->getType() == NodeType::POINTER)
            {
                value = static_cast<PointerNode*>(node)->getValue();
            }
            return value;
        }

        std::string GetStringValue(const std::string& badValue = "")
        {
            std::string value = badValue;
            BaseNode* node = parent->getNode(relativePath);
            if (node && node->getType() == NodeType::STRING)
            {
                value = static_cast<StringNode*>(node)->getValue();
            }
            return value;
        }

        // 检查节点是否存在
        bool exists() const
        {
            return parent->hasNode(relativePath);
        }

        // 获取节点类型
        NodeType type() const
        {
            BaseNode* node = parent->getNode(relativePath);
            return node ? node->getType() : NodeType::EMPTY;
        }
    };

    // []操作符重载
    NodeAccessor operator[](const std::string& relativePath)
    {
        return NodeAccessor(this, relativePath);
    }


    // 添加子节点
    void addChild(const std::string& name, BaseNode* node)
    {
        auto it = children.find(name);
        if (it != children.end())
        {
            delete it->second; // 删除旧的节点
        }
        children[name] = node;
    }

    // 获取子节点
    BaseNode* getChild(const std::string& name) const
    {
        auto it = children.find(name);
        if (it != children.end())
        {
            return it->second;
        }
        return nullptr;
    }

    // 移除子节点
    BaseNode* removeChild(const std::string& name)
    {
        auto it = children.find(name);
        if (it != children.end())
        {
            BaseNode* node = it->second;
            children.erase(it);
            return node; // 调用者负责删除
        }
        return nullptr;
    }

    // 检查子节点是否存在
    bool hasChild(const std::string& name) const
    {
        return children.find(name) != children.end();
    }

    // 获取所有子节点名称
    std::vector<std::string> getChildNames() const
    {
        std::vector<std::string> names;
        names.reserve(children.size());
        for (const auto& pair : children)
        {
            names.push_back(pair.first);
        }
        return names;
    }

    // 遍历子节点
    template<typename Func>
    void forEachChild(Func func) const
    {
        for (const auto& pair : children)
        {
            func(pair.first, pair.second);
        }
    }

    // 清空所有子节点
    void clearChildren()
    {
        for (auto& pair : children)
        {
            delete pair.second;
        }
        children.clear();
    }

    // 获取子节点数量
    size_t getChildCount() const
    {
        return children.size();
    }

    std::string getContent() const override
    {
        return "[Object: " + std::to_string(children.size()) + " children]";
    }

    std::string printTreeStyle(const std::string& prefix, bool isLast) const override
    {
        std::string result;

        // 当前节点的显示
        //result += prefix + (isLast ? "└── " : "├── ") + "Object\n";

        if (children.empty())
        {
            return result;
        }

        // 准备子节点的前缀
        std::string childPrefix = prefix + (isLast ? "    " : "│   ");

        // 递归打印所有子节点
        size_t index = 0;
        const size_t childCount = children.size();

        for (const auto& pair : children)
        {
            bool childIsLast = (++index == childCount);

            // 先打印字段名称和节点内容在同一行
            std::string fieldLine = childPrefix + (childIsLast ? "└── " : "├── ");
            fieldLine += "\"" + pair.first + "\": ";

            // 对于Object节点，只显示类型，不显示详细内容
            if (pair.second->getType() == NodeType::OBJECT)
            {
                fieldLine += "[Object]";
                result += fieldLine + "\n";
                // 递归打印Object子节点
                result += pair.second->printTreeStyle(childPrefix, childIsLast);
            }
            else
            {
                // 对于叶子节点，直接在同一行显示内容
                fieldLine += pair.second->getContent();
                result += fieldLine + "\n";
            }
        }

        return result;
    }

    ~ObjectNode() override
    {
        clearChildren();
    }
};