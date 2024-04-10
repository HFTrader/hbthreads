#pragma once

#include "DateTime.h"
#include <vector>
#include <iostream>

namespace hbthreads {

struct EventRateCounter {
    EventRateCounter(DateTime expiration_interval, DateTime time_precision)
        : precision(time_precision), last_index(0), total(0) {
        int num_intervals = expiration_interval.nsecs() / time_precision.nsecs();
        slots.resize(num_intervals);
    }
    void add(std::size_t numberOfEvents) {
        slots[slot].count += numberOfEvents;
        total += numberOfEvents;
    }

    void advance(DateTime dt) {
        std::size_t index = dt.nsecs() / precision.nsecs();
        if (index == last_index) return;
        // std::cout << "Advancing index " << index << " last:" << last_index <<
        // std::endl;
        if (index > last_index + slots.size()) {
            // std::cout << "Erasing entire vector" << std::endl;
            total = 0;
            for (std::size_t j = 0; j < slots.size(); ++j) {
                slots[j].count = 0;
            }
        } else {
            // std::cout << "Erasing slots: ";
            for (std::size_t j = last_index + 1; j <= index; ++j) {
                std::size_t idx = j % slots.size();
                // std::cout << idx << "(" << slots[idx].count << ") ";
                total -= slots[idx].count;
                slots[idx].count = 0;
            }
            // std::cout << std::endl;
        }
        last_index = index;
        slot = index % slots.size();
    }
    std::size_t count() const {
        return total;
    }
    operator std::size_t() const {
        return total;
    }

private:
    struct Slot {
        std::size_t count = 0;
    };
    std::vector<Slot> slots;
    DateTime precision;
    std::size_t last_index;
    std::size_t slot;
    std::size_t total;
};

}  // namespace hbthreads
