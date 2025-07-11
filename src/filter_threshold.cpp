#include "filter_threshold.h"
#include <iostream>     // For std::cout, std::cerr
#include <vector>
#include <numeric>      // For std::inner_product or manual sum
#include <algorithm>    // For std::copy
#include <iomanip>      // For std::fixed, std::setprecision

// Define the static filter window
const std::vector<double> FilterThreshold::FILTER_WINDOW = {0.05, 0.1, 0.15, 0.2, 0.25, 0.2, 0.15, 0.1, 0.05};

FilterThreshold::FilterThreshold(
    BlockingQueue<std::pair<uint8_t, uint8_t>>& input_queue,
    double tv,
    long long t_ns,
    bool& producer_finished_flag)
    : input_queue_(input_queue),
      threshold_value_(tv),
      t_ns_(t_ns),
      running_(true),
      producer_finished_flag_(producer_finished_flag) {
    if (FILTER_WINDOW.size() != WINDOW_SIZE) {
        // This should ideally be a compile-time check or an exception
        std::cerr << "Error: Filter window size mismatch!" << std::endl;
        // Potentially throw an error or set running_ to false
    }
}

void FilterThreshold::stop() {
    running_ = false;
}

void FilterThreshold::process_element() {
    // This function is called when data_buffer_ has enough elements (WINDOW_SIZE)
    // The element to be filtered is at index PAST_ELEMENTS (e.g., 4th index for 0-indexed)
    if (data_buffer_.size() < WINDOW_SIZE) {
        return; // Not enough data to form a full window
    }

    double filtered_value = 0.0;
    for (size_t i = 0; i < WINDOW_SIZE; ++i) {
        filtered_value += static_cast<double>(data_buffer_[i]) * FILTER_WINDOW[i];
    }

    // The original element that was filtered is data_buffer_[PAST_ELEMENTS]
    // We can now remove the oldest element from the buffer as it has been processed
    // (either as part of a window, or it's now past the "past elements" section for the newest center)
    // uint8_t processed_original_element = data_buffer_[PAST_ELEMENTS]; // This is the element that was at the center

    // Apply threshold
    bool defect = (filtered_value >= threshold_value_);

    // Output the result for the element that was at the center of this window
    std::cout << "Filtered Output for element value " << static_cast<int>(data_buffer_[PAST_ELEMENTS])
              << " (window center): "
              << "Filtered val = " << std::fixed << std::setprecision(4) << filtered_value
              << ", Thresholded = " << (defect ? "1 (Defect)" : "0 (No Defect)")
              << std::endl;

    // Remove the oldest element from the buffer, as it's no longer needed for future windows
    // centered on subsequent elements.
    data_buffer_.pop_front();
}


void FilterThreshold::run() {
    std::pair<uint8_t, uint8_t> received_pair;

    while (running_) {
        // Try to get data from the queue.
        // This pop can block. If producer is finished and queue is empty,
        // we need a way to stop.

        // Non-blocking try_pop to check producer_finished_flag_ periodically
        bool got_item = input_queue_.try_pop(received_pair);

        if (got_item) {
            data_buffer_.push_back(received_pair.first);
            if (data_buffer_.size() >= WINDOW_SIZE) {
                process_element(); // Process if window is full, oldest element at front is centered
            }
            data_buffer_.push_back(received_pair.second);
            if (data_buffer_.size() >= WINDOW_SIZE) {
                process_element(); // Process again if second element made window full
            }
        } else {
            // Queue was empty
            if (producer_finished_flag_ && input_queue_.empty()) {
                // Producer is done and queue is empty, process remaining buffer then stop
                std::cout << "FilterThreshold: Producer finished and queue empty." << std::endl;
                break; // Exit main processing loop
            }
            // If producer not finished, or queue not empty (race condition, unlikely with try_pop logic),
            // just yield/sleep briefly to avoid busy-waiting if items are infrequent.
            // The problem states T is a max cycle time, implying work or sleep.
            // If no item, we still "complete a cycle" by sleeping.
            // std::this_thread::yield(); // or a short sleep
        }

        // Regardless of whether an item was processed or not, ensure the cycle time T.
        // If process_element took time, this sleep will be shorter.
        // If no item was popped and processed, this will be a full T sleep.
        std::this_thread::sleep_for(std::chrono::nanoseconds(t_ns_));
    }

    // After the loop, process any remaining elements in the buffer
    // that can form a complete window. This is important when the producer has finished.
    std::cout << "FilterThreshold: Processing remaining elements in buffer after main loop..." << std::endl;
    while (data_buffer_.size() >= WINDOW_SIZE) {
        process_element();
    }

    std::cout << "FilterThreshold: Exiting run loop. " << data_buffer_.size() << " elements remaining in buffer (not enough for a full window)." << std::endl;
    if(!data_buffer_.empty()){
        std::cout << "Remaining elements: ";
        for(uint8_t val : data_buffer_){
            std::cout << static_cast<int>(val) << " ";
        }
        std::cout << std::endl;
    }
}
