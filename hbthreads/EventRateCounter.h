#pragma once

#include <DateTime.h>
#include <vector>

namespace hbthreads {

struct EventRateCounter {
    EventRateCounter(DateTime expiration_interval, DateTime time_precision)
        : precision(time_precision), last_index(0), total(0) {
        int num_intervals = expiration_interval.nsecs() / time_precision.nsecs() - 1;
        slots.resize(num_intervals);
    }
    void addEvent(std::size_t numberOfEvents) {
        int slot = last_index % slots.size();
        slots[slot].count += numberOfEvents;
        total += numberOfEvents;
    }

    void advance(DateTime dt) {
        std::size_t index = dt.nsecs() / precision.nsecs();
        if (index > last_index + slots.size()) {
            total = 0;
            for (std::size_t j = 0; j < slots.size(); ++j) {
                slots[j].count = 0;
            }
        } else {
            for (std::size_t j = last_index + 1; j <= index; ++j) {
                std::size_t idx = j % slots.size();
                total -= slots[idx].count;
                slots[idx].count = 0;
            }
        }
        last_index = index;
    }

private:
    struct Slot {
        std::size_t count = 0;
    };
    std::vector<Slot> slots;
    DateTime precision;
    std::size_t last_index;
    std::size_t total;
};

}  // namespace hbthreads
