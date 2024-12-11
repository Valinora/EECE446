#include <algorithm>
#include <queue>
#include <optional>

// ;)
class priority_queue_ext : public std::priority_queue<int> {
public:
    std::optional<int> remove(const int& value) {
        auto it = std::find(this->c.begin(), this->c.end(), value);

        if (it == this->c.end()) {
            return std::nullopt;
        } else if (it == this->c.begin()) {
            int ret = this->top();
            this->pop();
            return ret;
        } else {
            int ret = *it;
            this->c.erase(it);
            std::make_heap(this->c.begin(), this->c.end(), this->comp);
            return ret;
        }
    }
};

class ConnectionFactory {

};