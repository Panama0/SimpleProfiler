#pragma once

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/System/String.hpp"
#include "SFML/System/Vector2.hpp"
#include "SFML/Window/Event.hpp"
#include "SFML/Window/VideoMode.hpp"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <ratio>
#include <sstream>
#include <stack>
#include <thread>
#include <vector>

#define MAX_DEPTH 15

using TimePoint = std::chrono::steady_clock::time_point;

struct TimeNode
{
    TimeNode() = default;
    TimeNode(const char* _name, uint32_t _depth) : name{_name}, depth{_depth}
    {
    }

    const char* name{};

    TimePoint start{};
    TimePoint end{};
    uint32_t depth{};
};

class TimeGraph
{
public:
    static TimePoint now() { return std::chrono::steady_clock::now(); }

    static double duration(const TimePoint& start, const TimePoint& end)
    {
        return std::chrono::duration_cast<
                   std::chrono::duration<double, std::milli>>(end - start)
            .count();
    }

    static double duration(const TimeNode& node)
    {
        return duration(node.start, node.end);
    }

    static void addNode(const char* name)
    {
        if(m_nodeList.empty())
        {
            m_nodeList.reserve(1000);
        }

        auto& node = m_nodeList.emplace_back(name, depth());
        m_runningTimers.emplace(&node);
    }

    static void popNode()
    {
        assert(depth() != 0 && "No timer running");

        m_lastEnded = m_runningTimers.top();
        m_runningTimers.pop();
    }

    static void clear()
    {
        m_nodeList.clear();
        m_runningTimers = {};
        m_lastEnded = {};
    }

    // access functions
    static TimeNode& current()
    {
        assert(depth() != 0 && "No timer running");
        return *m_runningTimers.top();
    }

    static const std::vector<TimeNode>& getNodes() { return m_nodeList; }

    static double getLastDuration() { return duration(*m_lastEnded); }

private:
    static uint32_t depth() { return m_runningTimers.size(); }

    static inline std::vector<TimeNode> m_nodeList;
    static inline std::stack<TimeNode*> m_runningTimers;
    static inline TimeNode* m_lastEnded{};
};

static void timerStart(const char* label)
{
    TimeGraph::addNode(label);
    TimeGraph::current().start = std::chrono::steady_clock::now();
}

static void timerEnd()
{
    TimeGraph::current().end = std::chrono::steady_clock::now();
    TimeGraph::popNode();
}

class ScopeTimer
{
public:
    ScopeTimer(const char* label = "Scope") { timerStart(label); }

    ~ScopeTimer() { timerEnd(); }

private:
};

class Session
{
public:
    void start()
    {
        m_running = true;

        m_thread = std::thread(
            [this]()
            {
                m_window.create(sf::VideoMode{{1280, 720}}, "Profiler");
                m_window.setFramerateLimit(60);

                run();
            });
    }

    // get new data
    void newFrame()
    {
        // calculate how long the frame took
        if(!m_firstFrame)
        {
            timerEnd();
        }

        // copy
        m_nodeList = TimeGraph::getNodes();
        TimeGraph::clear();

        // start new frame
        timerStart("Frame");
        m_firstFrame = false;
    }

    // end and rejoin main thread
    void end()
    {
        m_running = false;
        m_thread.join();
    }

private:
    void run()
    {
        while(m_running)
        {
            m_window.clear();

            // process events
            while(auto event = m_window.pollEvent())
            {
                if(event->is<sf::Event::Closed>())
                {
                    m_running = false;
                }
            }

            if(!m_nodeList.empty())
            {
                drawTree();
            }

            m_window.display();
        }
        m_window.close();
    }

    void drawTree()
    {
        auto worldSize
            = static_cast<sf::Vector2f>(m_window.getView().getSize());

        // TODO: should be calculated seperately
        auto start = m_nodeList.front();
        auto frameDuration = static_cast<float>(TimeGraph::duration(start));


        float borderWidth {1.f};
        float barHeight{worldSize.y / MAX_DEPTH};

        for(auto& point : m_nodeList)
        {
            // bar
            auto pointDuration
                = static_cast<float>(TimeGraph::duration(point));
            float width{worldSize.x * (pointDuration / frameDuration) - borderWidth};

            sf::RectangleShape bar{{width, barHeight}};
            bar.setFillColor(colours(point.depth));
            bar.setOutlineColor(sf::Color::White);
            bar.setOutlineThickness(borderWidth);

            auto yOffset = worldSize.y - barHeight - ((barHeight + borderWidth) * point.depth);

            auto startOffset = static_cast<float>(
                TimeGraph::duration(start.start, point.start));
            auto xOffset = worldSize.x * (startOffset / frameDuration);

            bar.setPosition({xOffset, yOffset});
            m_window.draw(bar);

            // text
            std::ostringstream label;
            label << point.name << " - " << TimeGraph::duration(point)
                  << "ms.";
            sf::Text tag{m_font, label.str()};
            auto barCentre = bar.getGlobalBounds().getCenter();
            auto offset = tag.getGlobalBounds().getCenter();
            tag.setPosition(barCentre - offset);
            m_window.draw(tag);
        }
    }

    constexpr sf::Color colours(uint32_t index)
    {
        switch(index)
        {
        case 0:
            return sf::Color::Red;
        case 1:
            return sf::Color::Blue;
        case 2:
            return sf::Color::Green;
        case 3:
            return sf::Color::Magenta;
        case 4:
            return sf::Color::Cyan;
        default:
            return sf::Color::White;
        }
    }

    sf::RenderWindow m_window;
    sf::Font m_font{"../../res/DroidSans.ttf"};

    std::vector<TimeNode> m_nodeList;

    std::thread m_thread;
    bool m_running{false};

    bool m_firstFrame{true};
};
