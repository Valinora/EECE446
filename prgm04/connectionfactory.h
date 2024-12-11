#pragma once
#include <queue>
#include <optional>

class priority_queue_ext : public std::priority_queue<int> {
public:
    std::optional<int> remove(const int& value);
};

class ConnectionFactory {

};