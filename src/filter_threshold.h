#ifndef FILTER_THRESHOLD_H
#define FILTER_THRESHOLD_H

#include "blocking_queue.h"
#include <vector>
#include <deque>
#include <cstdint> // For uint8_t
#include <utility> // For std::pair
#include <string>  // For std::string, if needed for output formatting
#include <thread>  // For std::this_thread
#include <chrono>  // For std::chrono
#include <numeric> // For std::inner_product (potentially) or manual loop
#include <iomanip> // For std::fixed, std::setprecision if printing floats

class FilterThreshold {
public:
    FilterThreshold(BlockingQueue<std::pair<uint8_t, uint8_t>>& input_queue,
                    double tv, // Threshold Value
                    long long t_ns, // Process time T in nanoseconds
                    bool& producer_finished_flag); // Reference to a flag indicating producer is done

    // The main loop for the filter and threshold block, to be run in a thread.
    void run();

    // Signal to stop the processor.
    void stop();

    // Static constant for the filter window
    static const std::vector<double> FILTER_WINDOW;
    static const size_t WINDOW_SIZE = 9;
    static const size_t PAST_ELEMENTS = 4;
    static const size_t FUTURE_ELEMENTS = 4;


private:
    void process_element();

    BlockingQueue<std::pair<uint8_t, uint8_t>>& input_queue_;
    double threshold_value_;
    long long t_ns_; // Process time T in nanoseconds
    bool running_;
    bool& producer_finished_flag_; // Shared flag to know when producer is done

    std::deque<uint8_t> data_buffer_; // Buffer to hold elements for the 9-element window
};

#endif // FILTER_THRESHOLD_H
