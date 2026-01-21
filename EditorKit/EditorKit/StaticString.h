#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <vector>
#include <iostream>

//静态字符串，用于作为关联容器键值，哈希效率接近整数/枚举
//！！！注意：无法跨库使用！！！
class StaticString
{
public:
    // 构造函数
    StaticString() : StaticString("") {}
    StaticString(const char* str) : id_(getStringId(str)) {}
    StaticString(const std::string& str) : id_(getStringId(str.c_str())) {}

    // 拷贝构造函数
    StaticString(const StaticString& other) = default;

    // 赋值运算符
    StaticString& operator=(const StaticString& other) = default;

    // 获取字符串值
    const std::string& str() const
    {
        return getStringById(id_);
    }

    // 获取ID
    int id() const { return id_; }

    // 转换为C风格字符串
    const char* c_str() const
    {
        return getStringById(id_).c_str();
    }

    // 转换为std::string
    std::string toString() const
    {
        return getStringById(id_);
    }

    // 比较运算符
    bool operator==(const StaticString& other) const { return id_ == other.id_; }
    bool operator!=(const StaticString& other) const { return id_ != other.id_; }
    bool operator<(const StaticString& other) const { return id_ < other.id_; }
    bool operator<=(const StaticString& other) const { return id_ <= other.id_; }
    bool operator>(const StaticString& other) const { return id_ > other.id_; }
    bool operator>=(const StaticString& other) const { return id_ >= other.id_; }

    // 哈希函数
    std::size_t hash() const
    {
        return id_;
        //return std::hash<int>()(id_);
    }

private:
    int id_;

    // 全局字符串池
    struct StringPool
    {
        std::unordered_map<std::string, int> stringToId;  // 字符串到ID的映射
        std::vector<std::string> idToString;              // ID到字符串的映射
        std::mutex mutex;                                 // 线程安全
        int nextId = 0;                                   // 下一个可用的ID

        int getEmptyStringId()
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stringToId.find("") == stringToId.end())
            {
                stringToId[""] = 0;
                idToString.push_back("");
            }
            return 0;
        }

        int getIdForString(const char* str)
        {
            std::lock_guard<std::mutex> lock(mutex);

            std::string s = str ? str : "";

            auto it = stringToId.find(s);
            if (it != stringToId.end())
            {
                return it->second;
            }

            // 新字符串，分配新ID
            int newId = nextId++;
            stringToId[s] = newId;
            idToString.push_back(s);
            return newId;
        }

        const std::string& getStringById(int id)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (id >= 0 && id < static_cast<int>(idToString.size()))
            {
                return idToString[id];
            }
            // 返回空字符串作为默认值
            static const std::string empty = "";
            return empty;
        }
    };

    // 获取全局字符串池实例
    static StringPool& getStringPool()
    {
        static StringPool pool;
        return pool;
    }

    // 辅助方法
    static int getStringId(const char* str)
    {
        return getStringPool().getIdForString(str);
    }

    static int emptyStringId()
    {
        return getStringPool().getEmptyStringId();
    }

    static const std::string& getStringById(int id)
    {
        return getStringPool().getStringById(id);
    }

    friend std::ostream& operator<<(std::ostream& os, const StaticString& ss)
    {
        os << ss.str();
        return os;
    }
};

// 为StaticString提供std::hash特化
namespace std
{
    template<>
    struct hash<StaticString>
    {
        std::size_t operator()(const StaticString& s) const noexcept
        {
            return s.hash();
        }
    };
}

// 为StaticString提供比较函数对象
struct StaticStringCompare
{
    bool operator()(const StaticString& lhs, const StaticString& rhs) const
    {
        return lhs < rhs;
    }
};