#include "StateNode.h"
#include "StatePath.h"

// ObjectNode构造函数的实现
ObjectNode::ObjectNode(StatePath* system, const std::string& path)
    : stateSystem(system), absolutePath(path)
{
}

// 组合路径的辅助函数
std::string combinePath(const std::string& base, const std::string& relative)
{
    if (base.empty()) return relative;
    if (relative.empty()) return base;
    return base + "/" + relative;
}

// 相对路径操作接口实现
bool ObjectNode::setInt(const std::string& relativePath, int value)
{
    if (stateSystem && !relativePath.empty())
    {
        std::string fullPath = combinePath(absolutePath, relativePath);
        stateSystem->setInt(fullPath, value);
        return true;
    }
    return false;
}

bool ObjectNode::setFloat(const std::string& relativePath, float value)
{
    if (stateSystem && !relativePath.empty())
    {
        std::string fullPath = combinePath(absolutePath, relativePath);
        stateSystem->setFloat(fullPath, value);
        return true;
    }
    return false;
}

bool ObjectNode::setBool(const std::string& relativePath, bool value)
{
    if (stateSystem && !relativePath.empty())
    {
        std::string fullPath = combinePath(absolutePath, relativePath);
        stateSystem->setBool(fullPath, value);
        return true;
    }
    return false;
}

bool ObjectNode::setPointer(const std::string& relativePath, void* value)
{
    if (stateSystem && !relativePath.empty())
    {
        std::string fullPath = combinePath(absolutePath, relativePath);
        stateSystem->setPointer(fullPath, value);
        return true;
    }
    return false;
}

bool ObjectNode::setString(const std::string& relativePath, const std::string& value)
{
    if (stateSystem && !relativePath.empty())
    {
        std::string fullPath = combinePath(absolutePath, relativePath);
        stateSystem->setString(fullPath, value);
        return true;
    }
    return false;
}

bool ObjectNode::setObject(const std::string& relativePath)
{
    if (stateSystem && !relativePath.empty())
    {
        std::string fullPath = combinePath(absolutePath, relativePath);
        stateSystem->setObject(fullPath);
        return true;
    }
    return false;
}

BaseNode* ObjectNode::getNode(const std::string& relativePath)
{
    if (stateSystem)
    {
        std::string fullPath = combinePath(absolutePath, relativePath);
        return stateSystem->getNode(fullPath);
    }
    return nullptr;
}

bool ObjectNode::hasNode(const std::string& relativePath)
{
    if (stateSystem)
    {
        std::string fullPath = combinePath(absolutePath, relativePath);
        return stateSystem->hasNode(fullPath);
    }
    return false;
}

bool ObjectNode::removeNode(const std::string& relativePath)
{
    if (stateSystem && !relativePath.empty())
    {
        std::string fullPath = combinePath(absolutePath, relativePath);
        return stateSystem->removeNode(fullPath);
    }
    return false;
}

IntNode* BaseNode::AsIntNode()
{
    if (getType() == NodeType::INT)
    {
        return static_cast<IntNode*>(this);
    }
    return nullptr;
}

FloatNode* BaseNode::AsFloatNode()
{
    if (getType() == NodeType::FLOAT)
    {
        return static_cast<FloatNode*>(this);
    }
    return nullptr;
}

BoolNode* BaseNode::AsBoolNode()
{
    if (getType() == NodeType::BOOL)
    {
        return static_cast<BoolNode*>(this);
    }
    return nullptr;
}

PointerNode* BaseNode::AsPointerNode()
{
    if (getType() == NodeType::POINTER)
    {
        return static_cast<PointerNode*>(this);
    }
    return nullptr;
}

StringNode* BaseNode::AsStringNode()
{
    if (getType() == NodeType::STRING)
    {
        return static_cast<StringNode*>(this);
    }
    return nullptr;
}

ObjectNode* BaseNode::AsObjectNode()
{
    if (getType() == NodeType::OBJECT)
    {
        return static_cast<ObjectNode*>(this);
    }
    return nullptr;
}
