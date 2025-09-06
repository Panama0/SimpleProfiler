#pragma once

#include <cassert>
#include <chrono>
#include <iostream>
#include <ratio>
#include <vector>

using TimePoint = std::chrono::steady_clock::time_point;

struct TimeNode
{
    TimeNode() = default;
    TimeNode(const char* n, TimeNode* p) : name(n), parent(p) {}

    const char* name{};

    TimePoint start;
    TimePoint end;

    TimeNode* parent{};
    std::vector<TimeNode*> children{};
};

inline double duration(const TimePoint& start, const TimePoint& end)
{
    return std::chrono::duration<double, std::milli>(end - start).count();
}

inline double duration(const TimeNode& node)
{
    return duration(node.start, node.end);
}

class TimeGraph
{
public:
    // add child to the current node
    static void addNode(const char* name)
    {
        if(!m_root)
        {
            init();
        }

        auto& node = m_nodeList.emplace_back(name, m_current);

        m_current->children.emplace_back(&node);
        m_current = &m_nodeList.back();
    }

    // pop current node
    static void popNode()
    {
        m_last = m_current;

        if(m_current->parent)
        {
            m_current = m_current->parent;
        }
        else
        {
            // we are in the root
            assert(false && !"Attempted to pop root\n");
        }
    }

    // access functions
    static TimeNode& current() { return *m_current; }

    static TimeNode& getGraph() { return *m_root; }

    static double getLastDuration()
    {
        if(m_last)
        {
            return duration(*m_last);
        }
        else
        {
            return -1;
        }
    }

    static void printGraph() { printNode(*m_root); }

    static void printNode(TimeNode& node, int depth = 0)
    {
        // indent
        int indentWidth{3};
        for(int i{}; i < depth * indentWidth; i++)
        {
            std::cout << " ";
        }
        if(depth > 0)
        {
            std::cout << "└──";
        }

        std::cout << node.name << ": " << duration(node) << "ms" << std::endl;

        for(const auto& child : node.children)
        {
            printNode(*child, depth + 1);
        }
    }

private:
    static void init()
    {
        // default max
        m_nodeList.reserve(1000);

        m_nodeList.emplace_back("root", nullptr);
        m_root = &m_nodeList.back();
        m_current = m_root;
    }

    static inline std::vector<TimeNode> m_nodeList;

    static inline TimeNode* m_root{};
    static inline TimeNode* m_current{};
    static inline TimeNode* m_last{};
};

class Timer
{
public:
protected:
    void timerStart(const char* label)
    {
        TimeGraph::addNode(label);
        TimeGraph::current().start = std::chrono::steady_clock::now();
    }

    void timerEnd()
    {
        auto end{std::chrono::steady_clock::now()};

        TimeGraph::current().end = end;
        TimeGraph::popNode();
    }

private:
};

class ScopeTimer : public Timer
{
public:
    ScopeTimer(const char* label = "Scope") { timerStart(label); }

    ~ScopeTimer() { timerEnd(); }

private:
};
