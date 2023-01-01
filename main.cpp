#include <iostream>
#include <chrono>
#include "Timer.hpp"

int seconds{1};
int millis{1};

void printSeconds() {
    std::cout << "Second number: " << seconds++ << '\n';
}

void printMillis() {
    std::cout << "Millisecond number: " << millis++ * 100 << '\n';
}

int main() {
    Timer timer(std::chrono::microseconds(1000));
    timer.addTask([] { printSeconds(); }, std::chrono::milliseconds(1000));
    auto id = timer.addTask([] { printMillis(); }, std::chrono::milliseconds(100));
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    timer.removeTask(id);
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    timer.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    timer.resume();
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    timer.stop();
    return 0;
}
