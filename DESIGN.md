# Design Overview Document: Real-time Data Processing Pipeline (C++ Implementation)

## 1. Introduction

This document outlines the design for a system that simulates a real-time data processing pipeline, specifically for scanning and detecting defects in a continuous material (like a newspaper or textile roll). The system consists of two main processing blocks: a Data Generation block and a Filter & Threshold block. This version of the design specifies a **C++ implementation**, emphasizing modularity, scalability, and adherence to specified processing time constraints using C++ standard libraries.

## 2. Requirements Overview

The core requirements remain the same:
*   **Data Generation Block:**
    *   Simulates a line scan camera.
    *   Operates in two modes:
        1.  **Random Mode:** Generates two consecutive `uint8_t` random numbers.
        2.  **CSV Mode:** Reads data from a 2D array provided in a CSV file, two consecutive elements at a time, row by row.
    *   Outputs data continuously at a constant time interval `T`.
*   **Filter & Threshold Block:**
    *   Receives data from the Data Generation block.
    *   For each element, collects 4 past and 4 future elements (total 9 elements including the current).
    *   Applies a 9-element convolution filter: `[0.05, 0.1, 0.15, 0.2, 0.25, 0.2, 0.15, 0.1, 0.05]`.
    *   Thresholds the filtered value: `1` if `>= TV` (Threshold Value), `0` if `< TV`.
    *   Outputs the binary result.
    *   The processing cycle for each element must also respect the time interval `T`.
*   **System Constraints:**
    *   All blocks transfer two consecutive elements by default.
    *   Maximum cycle/iteration time for any block is `T` (>= 500ns).
    *   Blocks are not necessarily synchronous but the time difference between two consecutive data operations must be `<= T`.
*   **User Inputs:** Number of columns (`m`), Threshold Value (`TV`), Process Time (`T` in nanoseconds).

## 3. Architecture and Design Pattern

### 3.1. Chosen Architecture: Pipeline Pattern

The **Pipeline Pattern** remains the most suitable architecture.
*   **Concept:** Data flows sequentially through a series of processing stages.
*   **Fit for the Problem:** Aligns with the sequential nature of Data Generation -> Filter & Threshold, allowing concurrent processing.

### 3.2. Components (C++)

*   **`DataGenerator` Class:** The first stage. Produces pairs of `uint8_t` values.
*   **`FilterThreshold` Class:** The second stage. Consumes data, applies filtering and thresholding.
*   **`BlockingQueue<std::pair<uint8_t, uint8_t>>`:** A custom thread-safe queue connecting `DataGenerator` to `FilterThreshold`.

```
+-----------------+     (std::pair<uint8_t, uint8_t>)     +-------------------+
| DataGenerator   | ------------------------------------> | FilterThreshold   |
| (CSV or Random) |       BlockingQueue                   | (Filter & Binary) |
+-----------------+                                       +-------------------+
```

## 4. Communication Mechanism (C++)

### 4.1. Mechanism: Custom Thread-Safe Blocking Queue

A custom **`BlockingQueue<T>`** class will be implemented using C++ standard library primitives:
*   **Internal Storage:** `std::queue<T>` to hold the data elements.
*   **Synchronization:**
    *   `std::mutex`: To protect access to the shared internal queue and other state variables.
    *   `std::condition_variable`:
        *   One for signaling when an item is pushed (to wake up a waiting consumer).
        *   Optionally, another for signaling when an item is popped (to wake up a waiting producer if a maximum queue size is implemented, though not strictly required for this problem's current scope).
*   **Operations:**
    *   `void push(T item)`: Adds an item to the queue. Notifies one waiting consumer.
    *   `T pop()`: Removes and returns an item from the queue. Waits if the queue is empty.
*   **Advantages:**
    *   **Decoupling:** Producer (`DataGenerator`) and consumer (`FilterThreshold`) are decoupled.
    *   **Buffering:** The queue acts as a buffer.
    *   **Thread Safety:** Explicitly managed synchronization ensures safe concurrent access from different threads.
    *   **Blocking Semantics:** `pop()` will block if the queue is empty, and `push()` could block if a max size were enforced and reached.

### 4.2. Data Transferred

*   The data unit transferred will be `std::pair<uint8_t, uint8_t>`, representing two consecutive pixel values.

## 5. Scalability and Modularity

The principles of scalability and modularity remain the same as in the original design, adapted for C++:

### 5.1. Scalability

*   **Adding More Blocks:** New C++ classes representing new processing stages can be added, connected by additional `BlockingQueue` instances.
*   **Parallelism (Multi-threading):** Each block (`DataGenerator`, `FilterThreshold`) will run in its own `std::thread`.

### 5.2. Modularity

*   **Independent Components:** `DataGenerator` and `FilterThreshold` will be separate C++ classes with clear responsibilities.
*   **Well-Defined Interfaces:** Interaction occurs through the `BlockingQueue`'s `push` and `pop` methods.
*   **Testability:** Individual classes can be unit-tested.
*   **Reusability:** Classes can be reused if designed generically.

## 6. C++ Specific Considerations

*   **Concurrency:**
    *   `std::thread` for running blocks concurrently.
    *   `std::mutex`, `std::condition_variable` for implementing the `BlockingQueue`.
    *   Careful management of shared resources and synchronization to prevent deadlocks and race conditions.
*   **Time Interval `T`:**
    *   `std::this_thread::sleep_for(std::chrono::nanoseconds(T_ns))` will be used in each block's loop to simulate the processing time `T`.
*   **Data Types:** `uint8_t` for pixel values. `double` or `float` for filter calculations.
*   **Random Number Generation:** The `<random>` header will be used (e.g., `std::mt19937`, `std::uniform_int_distribution`).
*   **File I/O (CSV):** `std::ifstream` for reading the CSV file. String parsing (`std::getline`, `std::stringstream`) will be needed to extract numbers.
*   **Filter Window Edge Cases:**
    *   The `FilterThreshold` block will use a `std::deque<uint8_t>` as an internal buffer. It receives pairs `(val1, val2)` and pushes `val1` then `val2` into this deque.
    *   Filtering of an element occurs when it becomes the 5th element in a 9-element segment of the deque.
    *   Output is generated only when a full 9-element window is available.
*   **`m` Columns (Number of Columns):**
    *   In CSV mode, `m` is used by `DataGenerator` to guide the reading process, ensuring it simulates reading row by row. The `DataGenerator` will flatten the 2D array structure into a stream of pairs.
*   **Build System:** A `Makefile` or `CMakeLists.txt` will be required to manage compilation and linking. C++11 or newer is necessary for `std::thread`, `<chrono>`, `<random>`, etc.

## 7. Project Structure (Typical)

A possible directory and file structure:

```
.
├── src/
│   ├── data_generator.h
│   ├── data_generator.cpp
│   ├── filter_threshold.h
│   ├── filter_threshold.cpp
│   ├── blocking_queue.h
│   └── main.cpp
├── include/  (if headers are separated, though for this size, src/ is fine)
├── data/
│   └── sample_input.csv
├── Makefile  (or CMakeLists.txt)
└── DESIGN.md
```

## 8. User Inputs (C++ context)

*   **`m` (Number of columns):** `int`, used by `DataGenerator`.
*   **`TV` (Threshold Value):** `int` or `double`, used by `FilterThreshold`.
*   **`T` (Process Time):** `long long` (for nanoseconds), used by both blocks with `std::chrono`.

## 9. Conclusion

The Pipeline Pattern, implemented in C++ using standard concurrency and utility libraries, provides a performant, modular, and scalable solution. The custom `BlockingQueue` is central to inter-thread communication, ensuring safe and efficient data transfer between the processing stages.
