#include "gtest/gtest.h"
#include <chrono>
#include <thread>

#include "src/Timing.hpp"

TEST(ScopeTimer, basic)
{
    {
        ScopeTimer timer;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 1ms variance
    ASSERT_NEAR(TimeGraph::getLastDuration(), 100, 1);
}

TEST(ScopeTimer, last)
{
    {
        ScopeTimer timer;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    {
        ScopeTimer timer2;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    // 1ms variance
    ASSERT_NEAR(TimeGraph::getLastDuration(), 150, 1);
}

TEST(ScopeTimer, root)
{
    {
        ScopeTimer timer;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    {
        ScopeTimer timer2;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        {
            ScopeTimer timer3;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    auto& root = TimeGraph::getGraph();
    auto& testPoint = root.children.back()->children[0];
    auto elapsed = duration(*testPoint);

    // 1ms variance
    ASSERT_NEAR(elapsed, 20, 1);
}
