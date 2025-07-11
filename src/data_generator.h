#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "blocking_queue.h"
#include <string>
#include <vector>
#include <cstdint> // For uint8_t
#include <utility> // For std::pair
#include <thread>    // For std::this_thread
#include <chrono>    // For std::chrono
#include <random>    // For random number generation
#include <fstream>   // For file operations
#include <sstream>   // For string stream operations
#include <iostream>  // For std::cerr

// Forward declaration if DataGenerator needs to be a friend of another class, not needed here.

class DataGenerator {
public:
    DataGenerator(BlockingQueue<std::pair<uint8_t, uint8_t>>& output_queue,
                  int m,
                  long long t_ns, // Process time T in nanoseconds
                  const std::string& csv_filepath = "");

    // The main loop for the data generator, to be run in a thread.
    void run();

    // Signal to stop the generator.
    void stop();

private:
    void generate_random_pair();
    bool read_csv_pair();
    bool open_csv();

    BlockingQueue<std::pair<uint8_t, uint8_t>>& output_queue_;
    int m_; // Number of columns, relevant for CSV structure
    long long t_ns_; // Process time T in nanoseconds
    std::string csv_filepath_;
    bool use_csv_mode_;
    bool running_;

    // For random number generation
    std::mt19937 random_engine_;
    std::uniform_int_distribution<int> uint8_distribution_;

    // For CSV reading
    std::ifstream csv_file_stream_;
    std::vector<uint8_t> current_row_values_;
    size_t current_row_idx_;
    int line_number_; // For error reporting

    // Buffer for holding elements when reading CSV to form pairs
    std::vector<uint8_t> csv_buffer_;
};

#endif // DATA_GENERATOR_H
