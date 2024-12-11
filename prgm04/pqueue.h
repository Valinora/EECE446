#pragma once

#include <queue>
#include <optional>
#include <algorithm>

class PriorityQueueExt : public std::priority_queue<int> {
 public:
  /**
   * Removes a given value from the queue, regardless of position, and returns the value.
   * @param value The value to be removed.
   * @return A std::optional containing the value removed or nullopt if no match was found.
   */
  inline std::optional<int> remove(const int& value) {
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