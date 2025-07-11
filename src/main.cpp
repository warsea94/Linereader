#include "data_generator.h"
#include "filter_threshold.h"
#include "blocking_queue.h"

#include <iostream>
#include <string>
#include <thread>
#include <limits> // Required for std::numeric_limits
#include <atomic> // For std::atomic_bool

// Helper function to get integer input safely
long long get_long_input(const std::string& prompt) {
    long long value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (std::cin.good() && value >= 0) { // Basic validation: non-negative
            // Clear the rest of the line
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        } else {
            std::cerr << "Invalid input. Please enter a non-negative number." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
}

double get_double_input(const std::string& prompt) {
    double value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (std::cin.good()) { // Basic validation
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        } else {
            std::cerr << "Invalid input. Please enter a valid number." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
}


int main() {
    std::cout << "--- Real-time Data Processing Pipeline Simulator ---" << std::endl;

    // Get user inputs
    int m = static_cast<int>(get_long_input("Enter number of columns (m, for CSV mode, 0 for non-CSV relevant): "));
    double tv = get_double_input("Enter Threshold Value (TV): ");
    long long t_ns = get_long_input("Enter Process Time T (in nanoseconds, >= 500): ");
    if (t_ns < 500) {
        std::cerr << "Warning: T is less than 500ns. Setting to 500ns." << std::endl;
        t_ns = 500;
    }

    std::string mode_choice;
    std::string csv_filepath = "";
    bool use_csv = false;

    while (true) {
        std::cout << "Select mode (random/csv): ";
        std::cin >> mode_choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // consume newline

        if (mode_choice == "csv") {
            use_csv = true;
            std::cout << "Enter CSV filepath: ";
            std::getline(std::cin, csv_filepath);
            // Basic check if filepath is empty, actual file check done in DataGenerator
            if (csv_filepath.empty()) {
                std::cerr << "CSV filepath cannot be empty for CSV mode." << std::endl;
                continue;
            }
            break;
        } else if (mode_choice == "random") {
            use_csv = false;
            break;
        } else {
            std::cerr << "Invalid mode. Please enter 'random' or 'csv'." << std::endl;
        }
    }

    // Shared flag to indicate producer (DataGenerator) has finished
    // This needs to be atomic if accessed by DataGenerator itself to set it,
    // but here DataGenerator's run() exits and main thread sets it.
    // For FilterThreshold to read it safely while DataGenerator might modify it (if DataGen set it),
    // it should be atomic. Let's make it atomic for good practice.
    // However, in the current design, DataGenerator.run() finishes, then main sets this flag.
    // FilterThreshold reads it. So, it's more about visibility after DataGenerator thread joins.
    // For simplicity in this version, a simple bool is used and set after DataGenerator's thread join.
    // A more robust solution for Filter to stop early might use an atomic flag set by DataGen before exiting.
    // Let's use a simple bool and manage its state from main.
    bool producer_is_finished = false;


    // Initialize components
    BlockingQueue<std::pair<uint8_t, uint8_t>> data_queue;

    DataGenerator data_gen(data_queue, m, t_ns, use_csv ? csv_filepath : "");
    // Pass the reference to the shared flag.
    FilterThreshold filter_thresh(data_queue, tv, t_ns, producer_is_finished);


    std::cout << "\nStarting simulation..." << std::endl;
    std::cout << "Press Ctrl+C to stop if in continuous random mode." << std::endl;
    if(use_csv) {
        std::cout << "CSV Mode: Processing file " << csv_filepath << std::endl;
    } else {
        std::cout << "Random Mode: Generating random data." << std::endl;
    }
    std::cout << "M=" << m << ", TV=" << tv << ", T=" << t_ns << "ns" << std::endl;


    // Create and start threads
    std::thread data_gen_thread(&DataGenerator::run, &data_gen);
    std::thread filter_thresh_thread(&FilterThreshold::run, &filter_thresh);

    // Wait for DataGenerator to finish
    // In CSV mode, it will finish when the file is processed.
    // In random mode, it runs indefinitely until stop() is called (or program terminates).
    // For this simulation, we'll let random mode run until Ctrl+C or similar.
    // If it's CSV mode, data_gen_thread.join() will return when DataGenerator::run() exits.
    data_gen_thread.join();
    std::cout << "DataGenerator thread finished." << std::endl;

    // Once DataGenerator is finished, signal FilterThreshold.
    // This approach is simple: main thread acts as coordinator.
    producer_is_finished = true;
    std::cout << "Signaled FilterThreshold that producer is finished." << std::endl;


    // Wait for FilterThreshold to finish processing remaining items.
    // FilterThreshold's run loop will exit once producer_is_finished is true AND queue is empty.
    filter_thresh_thread.join();
    std::cout << "FilterThreshold thread finished." << std::endl;

    std::cout << "\nSimulation complete." << std::endl;

    return 0;
}
