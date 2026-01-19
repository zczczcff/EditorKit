#pragma once
#include "StateNode.h"
#include <unordered_set>
// 节点类型枚举



// 事件类型
enum class EventType
{
    ADD,      // 增加节点
    REMOVE,   // 删除节点  
    MOVE,     // 移动节点
    UPDATE    // 修改节点值
};

// 监听粒度
enum class ListenGranularity
{
    NODE,          // 监听指定节点
    DIRECT_CHILD,  // 监听指定节点的直接子节点
    ALL_CHILDREN   // 监听指定节点所有子节点
};

class BaseNode;

// 事件信息结构
struct PathEvent
{
    EventType type;
    std::string path;           // 事件发生的路径
    std::string relatedPath;    // 相关路径（如移动操作的目标路径）
    BaseNode* node;             // 涉及的节点
    NodeType nodeType;          // 节点类型
};

// 事件回调函数类型
using EventCallback = std::function<void(const PathEvent&)>;

// 监听器标识
using ListenerId = size_t;

// 监听器信息
struct ListenerInfo
{
    ListenerId id;
    std::string path;
    ListenGranularity granularity;
    EventCallback callback;
    EventType eventType; // 监听的事件类型
};

// 前缀树节点
class EventTrieNode
{
private:
    std::unordered_map<std::string, EventTrieNode*> children;
    std::vector<ListenerInfo> listeners;

public:
    ~EventTrieNode()
    {
        for (auto& pair : children)
        {
            delete pair.second;
        }
    }

    EventTrieNode* getOrCreateChild(const std::string& part)
    {
        auto it = children.find(part);
        if (it == children.end())
        {
            it = children.emplace(part, new EventTrieNode()).first;
        }
        return it->second;
    }

    EventTrieNode* getChild(const std::string& part) const
    {
        auto it = children.find(part);
        return it != children.end() ? it->second : nullptr;
    }

    void addListener(const ListenerInfo& listener)
    {
        listeners.push_back(listener);
    }

    bool removeListener(ListenerId id)
    {
        auto it = std::find_if(listeners.begin(), listeners.end(),
            [id](const ListenerInfo& info) { return info.id == id; });
        if (it != listeners.end())
        {
            listeners.erase(it);
            return true;
        }
        return false;
    }

    // 新增：只获取当前节点的监听器（不递归子节点）
    std::vector<ListenerInfo> getCurrentListeners() const
    {
        return listeners;
    }

    // 新增：获取直接子节点监听器
    std::vector<ListenerInfo> getDirectChildListeners() const
    {
        std::vector<ListenerInfo> result;
        for (const auto& listener : listeners)
        {
            if (listener.granularity == ListenGranularity::DIRECT_CHILD)
            {
                result.push_back(listener);
            }
        }
        return result;
    }

    // 删除有问题的递归方法
    // void getMatchingListeners(...)  // 删除这个方法
};

class EventManager
{
private:
    EventTrieNode root;
    ListenerId nextId = 1;
    std::unordered_map<ListenerId, std::string> listenerPaths;

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

public:
    ListenerId addListener(const std::string& path, ListenGranularity granularity,
        EventType eventType, EventCallback callback)
    {
        ListenerId id = nextId++;
        ListenerInfo info{ id, path, granularity, callback, eventType };

        auto parts = splitPath(path);
        EventTrieNode* node = &root;
        for (const auto& part : parts)
        {
            node = node->getOrCreateChild(part);
        }
        node->addListener(info);

        listenerPaths[id] = path;
        return id;
    }

    bool removeListener(ListenerId id)
    {
        auto it = listenerPaths.find(id);
        if (it == listenerPaths.end())
        {
            return false;
        }

        std::string path = it->second;
        listenerPaths.erase(it);

        auto parts = splitPath(path);
        EventTrieNode* node = &root;
        for (const auto& part : parts)
        {
            node = node->getChild(part);
            if (!node) return false;
        }

        return node->removeListener(id);
    }

    std::vector<ListenerInfo> findListeners(const std::string& path, EventType eventType)
    {
        std::vector<ListenerInfo> result;
        auto parts = splitPath(path);

        // 使用集合来避免重复的监听器
        std::unordered_set<ListenerId> addedIds;

        // 1. 首先处理精确匹配的节点监听器
        EventTrieNode* exactNode = &root;
        for (const auto& part : parts)
        {
            exactNode = exactNode->getChild(part);
            if (!exactNode) break;
        }

        if (exactNode)
        {
            // 获取精确节点的监听器（不递归子节点）
            auto exactListeners = exactNode->getCurrentListeners();
            for (const auto& listener : exactListeners)
            {
                if (listener.eventType == eventType &&
                    addedIds.find(listener.id) == addedIds.end())
                {
                    // 检查粒度：NODE 需要精确匹配路径
                    if (listener.granularity == ListenGranularity::NODE)
                    {
                        if (listener.path == path)
                        {
                            result.push_back(listener);
                            addedIds.insert(listener.id);
                        }
                    }
                    // ALL_CHILDREN 只要前缀匹配
                    else if (listener.granularity == ListenGranularity::ALL_CHILDREN)
                    {
                        if (path.find(listener.path) == 0) // 前缀匹配
                        {
                            result.push_back(listener);
                            addedIds.insert(listener.id);
                        }
                    }
                }
            }
        }

        // 2. 处理直接子节点监听器
        if (parts.size() > 0)
        {
            // 构建父路径
            std::string parentPath;
            for (size_t i = 0; i < parts.size() - 1; ++i)
            {
                parentPath += (i > 0 ? "/" : "") + parts[i];
            }

            // 找到父节点
            auto parentParts = splitPath(parentPath);
            EventTrieNode* parentNode = &root;
            for (const auto& part : parentParts)
            {
                parentNode = parentNode->getChild(part);
                if (!parentNode) break;
            }

            if (parentNode)
            {
                auto directChildListeners = parentNode->getDirectChildListeners();
                for (const auto& listener : directChildListeners)
                {
                    if (listener.eventType == eventType &&
                        listener.path == parentPath && // 监听器注册在父路径上
                        addedIds.find(listener.id) == addedIds.end())
                    {
                        result.push_back(listener);
                        addedIds.insert(listener.id);
                    }
                }
            }
        }

        // 3. 处理所有祖先节点的 ALL_CHILDREN 监听器
        EventTrieNode* currentNode = &root;
        std::string currentPath;

        for (size_t i = 0; i < parts.size(); ++i)
        {
            currentNode = currentNode->getChild(parts[i]);
            if (!currentNode) break;

            // 构建当前路径
            currentPath += (i > 0 ? "/" : "") + parts[i];

            // 获取当前节点的 ALL_CHILDREN 监听器
            auto currentListeners = currentNode->getCurrentListeners();
            for (const auto& listener : currentListeners)
            {
                if (listener.eventType == eventType &&
                    listener.granularity == ListenGranularity::ALL_CHILDREN &&
                    addedIds.find(listener.id) == addedIds.end())
                {
                    // 检查路径前缀匹配
                    if (path.find(listener.path) == 0)
                    {
                        result.push_back(listener);
                        addedIds.insert(listener.id);
                    }
                }
            }
        }

        return result;
    }
};