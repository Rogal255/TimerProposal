#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <vector>

template<typename T>
concept DurationConcept = std::same_as<T, std::chrono::nanoseconds>
|| std::same_as<T, std::chrono::microseconds>
|| std::same_as<T, std::chrono::milliseconds>
|| std::same_as<T, std::chrono::seconds>
|| std::same_as<T, std::chrono::minutes>
|| std::same_as<T, std::chrono::hours>;

template<DurationConcept DurationType>
struct Task {
    Task(const std::function<void()>& func, const DurationType& period, const std::size_t& id) : fun(func),
                                                                                                 period(period),
                                                                                                 uniqueId(id) {}

    std::function<void()> fun;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastExecuted{std::chrono::high_resolution_clock::now()};
    DurationType period;
    std::size_t uniqueId;
};

template<DurationConcept DurationType>
class Timer {
public:
    explicit Timer(DurationType&& resolution) : resolution_{std::forward<DurationType>(resolution)} {
        vec_.reserve(5);
    }

    ~Timer() { stop(); }

    Timer(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer& operator=(Timer&&) = delete;

    [[maybe_unused]] std::size_t addTask(const std::function<void()>& fun, const DurationType& period) noexcept {
        std::lock_guard lg(vecMutex_);
        if (vec_.empty()) {
            thread_ = threadStart();
        }
        vec_.emplace_back(fun, period, idCounter_++);
        return vec_.back().uniqueId;
    }

    void removeTask(std::size_t uniqueId) noexcept {
        std::lock_guard lg(vecMutex_);
        vec_.erase(std::remove_if(vec_.begin(), vec_.end(), [&](const auto& task) {
            return task.uniqueId == uniqueId;
        }));
    }

    void stop() noexcept {
        running_ = false;
    }

    void resume() noexcept {
        running_ = true;
        std::lock_guard lg(vecMutex_);
        for (auto& task: vec_) {
            task.lastExecuted = std::chrono::high_resolution_clock::now();
        }
        thread_ = threadStart();
    }

private:
    std::vector<Task<DurationType>> vec_;
    std::mutex vecMutex_;
    DurationType resolution_;
    std::jthread thread_;
    std::atomic<bool> running_{true};
    std::size_t idCounter_{0};

    std::jthread threadStart() noexcept {
        return std::jthread{[this] {
            while (running_) {
                auto start = std::chrono::high_resolution_clock::now();
                this->onUpdate();
                auto stop = std::chrono::high_resolution_clock::now();
                std::this_thread::sleep_for(resolution_ - std::chrono::duration_cast<DurationType>(stop - start));
            }
        }};
    }

    void onUpdate() noexcept {
        std::lock_guard lg(vecMutex_);
        for (auto& task: vec_) {
            if (task.lastExecuted + task.period <= std::chrono::high_resolution_clock::now()) {
                task.fun();
                task.lastExecuted += task.period;   // For time measure correctness
                // task.lastExecuted = std::chrono::high_resolution_clock::now();
            }
        }
    }
};
