#pragma once

#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/View.hpp"
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

namespace SimpleProf
{

#define MAX_DEPTH 15

using TimePoint = std::chrono::steady_clock::time_point;

// base singleton
class ProfilerBase
{
public:
    static ProfilerBase& get()
    {
        static ProfilerBase instance;
        return instance;
    }

    struct TimeNode
    {
        TimeNode() = default;
        TimeNode(const char* _name, uint32_t _depth)
            : name{_name}, depth{_depth}
        {
        }

        const char* name{};

        TimePoint start{};
        TimePoint end{};
        uint32_t depth{};
    };

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

    void startNode(const char* name)
    {
        if(m_nodeList.empty())
        {
            m_nodeList.reserve(1000);
        }

        auto& node = m_nodeList.emplace_back(name, depth());
        m_runningTimers.emplace(&node);

        current().start = now();
    }

    void endNode()
    {
        current().end = now();

        assert(depth() != 0 && "No timer running");

        m_lastEnded = m_runningTimers.top();
        m_runningTimers.pop();
    }

    void clear()
    {
        m_nodeList.clear();
        m_runningTimers = {};
        m_lastEnded = {};
    }

    // access functions
    TimeNode& current()
    {
        assert(depth() != 0 && "No timer running");
        return *m_runningTimers.top();
    }

    const std::vector<TimeNode>& getNodes() { return m_nodeList; }

    void swap(std::vector<TimeNode>& other) { m_nodeList.swap(other); }

    double getLastDuration() { return duration(*m_lastEnded); }

private:
    ProfilerBase() = default;
    ProfilerBase(const ProfilerBase&) = delete;
    ProfilerBase& operator=(const ProfilerBase&) = delete;

    uint32_t depth() { return m_runningTimers.size(); }

    std::vector<TimeNode> m_nodeList;
    std::stack<TimeNode*> m_runningTimers;
    TimeNode* m_lastEnded{};
};

class Timer
{
public:
    static void start(const char* label = "Timer")
    {
        ProfilerBase::get().startNode(label);
    }

    static void stop() { ProfilerBase::get().endNode(); }
};

class ScopeTimer
{
public:
    ScopeTimer(const char* label = "Scope")
    {
        ProfilerBase::get().startNode(label);
    }

    ~ScopeTimer() { ProfilerBase::get().endNode(); }
};

class Session
{
public:
    Session() : m_prof{ProfilerBase::get()} {}

    void start()
    {
        m_running = true;

        m_thread = std::thread([this]() { run(); });
    }

    // get new data
    void newFrame()
    {
        // calculate how long the frame took
        if(!m_firstFrame)
        {
            m_prof.endNode();
        }

        // copy
        m_nodes = m_prof.getNodes();
        m_prof.clear();

        // start new frame
        m_prof.startNode("Frame");
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
        m_window.create(sf::VideoMode{{1280, 720}}, "Profiler");
        updateView({1280, 720});
        m_window.setFramerateLimit(60);

        if(!m_font.openFromFile("../../res/DroidSans.ttf"))
        {
            std::cerr << "Profiler: Font not found" << std::endl;
        }

        std::vector<ProfilerBase::TimeNode> localNodes;

        while(m_running)
        {
            localNodes = m_nodes;

            m_window.clear();

            // process events
            while(auto event = m_window.pollEvent())
            {
                if(event->is<sf::Event::Closed>())
                {
                    m_running = false;
                }

                if(auto resized = event->getIf<sf::Event::Resized>())
                {
                    m_window.setSize(resized->size);
                    updateView(resized->size);
                }
            }

            if(!localNodes.empty())
            {
                drawTree(localNodes);
            }

            m_window.display();
        }
        m_window.close();
    }

    void drawTree(const std::vector<ProfilerBase::TimeNode>& nodes)
    {
        auto worldSize
            = static_cast<sf::Vector2f>(m_window.getView().getSize());

        // TODO: should be calculated seperately
        auto start = nodes.front();
        auto frameDuration = static_cast<float>(ProfilerBase::duration(start));

        float borderWidth{1.f};
        float barHeight{worldSize.y / MAX_DEPTH};

        for(auto& point : nodes)
        {
            // bar
            auto pointDuration
                = static_cast<float>(ProfilerBase::duration(point));
            float width{worldSize.x * (pointDuration / frameDuration)
                        - borderWidth};

            sf::RectangleShape bar{{width, barHeight}};
            bar.setFillColor(colours(point.depth));
            bar.setOutlineColor(sf::Color::White);
            bar.setOutlineThickness(borderWidth);

            auto yOffset = worldSize.y - barHeight
                           - ((barHeight + borderWidth) * point.depth);

            auto startOffset = static_cast<float>(
                ProfilerBase::duration(start.start, point.start));
            auto xOffset = worldSize.x * (startOffset / frameDuration);

            bar.setPosition({xOffset, yOffset});
            m_window.draw(bar);

            // text
            uint32_t textSize{20};

            std::ostringstream label;
            label << std::fixed << std::setprecision(2);
            label << point.name << " - " << ProfilerBase::duration(point)
                  << "ms.";
            sf::Text tag{m_font, label.str(), textSize};
            auto barCentre = bar.getGlobalBounds().getCenter();
            auto offset = tag.getGlobalBounds().getCenter();
            tag.setPosition(barCentre - offset);
            m_window.draw(tag);
        }
    }

    void updateView(const sf::Vector2u& windowSize)
    {
        float windowRatio = windowSize.x / static_cast<float>(windowSize.y);
        float viewRatio = m_view.getSize().x / m_view.getSize().y;

        float sizeX = 1.f, sizeY = 1.f;
        float posX = 0.f, posY = 0.f;

        if(windowRatio > viewRatio)
        {
            sizeX = viewRatio / windowRatio;
            posX = (1.f - sizeX) / 2.f;
        }
        else
        {
            sizeY = windowRatio / viewRatio;
            posY = (1.f - sizeY) / 2.f;
        }

        m_view.setViewport(sf::FloatRect{{posX, posY}, {sizeX, sizeY}});
        m_window.setView(m_view);
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
    sf::View m_view{sf::FloatRect{{}, {1280, 720}}};
    sf::Font m_font{};

    ProfilerBase& m_prof;
    std::vector<ProfilerBase::TimeNode> m_nodes;

    std::thread m_thread;
    bool m_running{false};

    bool m_firstFrame{true};
};
}
