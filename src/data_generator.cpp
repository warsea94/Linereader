#include "data_generator.h"
#include <vector>
#include <iostream> // For std::cout, std::cerr

DataGenerator::DataGenerator(
    BlockingQueue<std::pair<uint8_t, uint8_t>>& output_queue,
    int m,
    long long t_ns,
    const std::string& csv_filepath)
    : output_queue_(output_queue),
      m_(m),
      t_ns_(t_ns),
      csv_filepath_(csv_filepath),
      use_csv_mode_(!csv_filepath.empty()),
      running_(true),
      random_engine_(std::random_device{}()), // Seed with a real random device
      uint8_distribution_(0, 255),
      current_row_idx_(0),
      line_number_(0) {
    if (use_csv_mode_) {
        if (!open_csv()) {
            // Error opening CSV, could throw or switch to random mode.
            // For now, let's print an error and it will likely fail in run() or do nothing.
            std::cerr << "Error: Could not open CSV file: " << csv_filepath_ << std::endl;
            // Potentially set running_ to false or throw an exception
            // For this exercise, it will try to run and exit if CSV mode fails.
        }
    }
}

void DataGenerator::stop() {
    running_ = false;
    // Note: If the run loop is blocked on queue push (if queue had max size)
    // or sleep, this flag alone might not be immediate.
    // For this problem, queue push is unlikely to block indefinitely.
    // If CSV is open, it should be closed.
    if (csv_file_stream_.is_open()) {
        csv_file_stream_.close();
    }
}

bool DataGenerator::open_csv() {
    if (!csv_filepath_.empty()) {
        csv_file_stream_.open(csv_filepath_);
        if (!csv_file_stream_.is_open()) {
            return false; // Failed to open
        }
        return true;
    }
    return false; // No CSV path provided
}

void DataGenerator::generate_random_pair() {
    uint8_t val1 = static_cast<uint8_t>(uint8_distribution_(random_engine_));
    uint8_t val2 = static_cast<uint8_t>(uint8_distribution_(random_engine_));
    output_queue_.push({val1, val2});
    // std::cout << "Generated: (" << (int)val1 << ", " << (int)val2 << ")" << std::endl;
}

bool DataGenerator::read_csv_pair() {
    if (!csv_file_stream_.is_open() && !open_csv()) {
        std::cerr << "CSV Error: File stream not open and cannot be reopened." << std::endl;
        return false; // Cannot proceed
    }

    // Fill the buffer if it has less than 2 elements
    while (csv_buffer_.size() < 2) {
        // Try to read more from the current row
        if (current_row_idx_ < current_row_values_.size()) {
            csv_buffer_.push_back(current_row_values_[current_row_idx_++]);
        } else {
            // Current row is exhausted, try to read a new line from CSV
            std::string line;
            if (std::getline(csv_file_stream_, line)) {
                line_number_++;
                current_row_values_.clear();
                current_row_idx_ = 0;
                std::stringstream ss(line);
                std::string cell;
                int count = 0;
                while (std::getline(ss, cell, ',')) {
                    if (count < m_ || m_ <= 0) { // Respect m if m > 0
                        try {
                            current_row_values_.push_back(static_cast<uint8_t>(std::stoi(cell)));
                        } catch (const std::invalid_argument& ia) {
                            std::cerr << "CSV Error: Invalid argument in CSV line " << line_number_ << ": " << cell << std::endl;
                            // Skip malformed cell
                        } catch (const std::out_of_range& oor) {
                            std::cerr << "CSV Error: Out of range in CSV line " << line_number_ << ": " << cell << std::endl;
                            // Skip malformed cell
                        }
                    }
                    count++;
                }
                if (m_ > 0 && count < m_) {
                     std::cerr << "CSV Warning: Line " << line_number_ << " has fewer than m=" << m_ << " columns. (" << count << ")" << std::endl;
                }


                // After reading a new line, try to fill the buffer again from this new line
                if (current_row_idx_ < current_row_values_.size()) {
                     csv_buffer_.push_back(current_row_values_[current_row_idx_++]);
                } else if (csv_file_stream_.eof() && current_row_values_.empty()) {
                    // No more data in the file and current_row_values is also empty
                    break;
                }

            } else {
                // EOF or error reading line
                break;
            }
        }
    }

    if (csv_buffer_.size() >= 2) {
        uint8_t val1 = csv_buffer_[0];
        uint8_t val2 = csv_buffer_[1];
        csv_buffer_.erase(csv_buffer_.begin(), csv_buffer_.begin() + 2); // Remove the two used elements
        output_queue_.push({val1, val2});
        // std::cout << "CSV Read: (" << (int)val1 << ", " << (int)val2 << ")" << std::endl;
        return true;
    } else if (!csv_buffer_.empty() && (csv_file_stream_.eof() && current_row_idx_ >= current_row_values_.size())) {
        // Handle the case where only one element is left at the end of the file
        // The problem asks for pairs. This design will discard the last single element.
        // Alternatively, it could pad with a zero or send a special marker.
        // For now, we only send full pairs.
        std::cerr << "CSV Info: End of file reached. One element (" << (int)csv_buffer_[0] << ") left in buffer, discarded as pairs are required." << std::endl;
        csv_buffer_.clear();
        return false; // No full pair to send
    }

    return false; // No pair was read (either EOF or not enough elements for a pair)
}


void DataGenerator::run() {
    if (use_csv_mode_ && !csv_file_stream_.is_open()) {
        std::cerr << "DataGenerator: CSV mode selected but file not open. Exiting run loop." << std::endl;
        running_ = false; // Ensure it stops
    }

    while (running_) {
        if (use_csv_mode_) {
            if (!read_csv_pair()) {
                // End of CSV file or error
                std::cout << "DataGenerator: End of CSV data or error reading file." << std::endl;
                stop(); // Signal to stop
            }
        } else {
            generate_random_pair();
        }

        if (running_) { // Check running_ again in case stop() was called by read_csv_pair
             // Ensure the loop iteration takes at least t_ns
            std::this_thread::sleep_for(std::chrono::nanoseconds(t_ns_));
        }
    }

    if (csv_file_stream_.is_open()) {
        csv_file_stream_.close();
    }
    // Signal end of data if necessary, e.g. by pushing a special pair.
    // For this problem, the consumer will just stop when the queue is empty and
    // the producer has finished. Or, for continuous random, it runs indefinitely.
    // If it was CSV mode, we can push a "poison pill" or rely on filter knowing producer is done.
    // For now, we won't push a poison pill. Filter will try to pop and eventually block or main will terminate threads.
    std::cout << "DataGenerator: Exiting run loop." << std::endl;
}
