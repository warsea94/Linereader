# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I./src
LDFLAGS = -pthread

# Source files directory
SRCDIR = src

# Find all .cpp files in SRCDIR
SOURCES = $(wildcard $(SRCDIR)/*.cpp)

# Replace .cpp with .o to get object file names
OBJECTS = $(SOURCES:.cpp=.o)

# Name of the executable
EXECUTABLE = pipeline_simulator

# Default target: build the executable
all: $(EXECUTABLE)

# Rule to link the executable
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

# Generic rule to compile .cpp files into .o files
# For each .cpp file, this rule will be used.
# It automatically includes dependencies on any .h files in SRCDIR.
# This is a common pattern but might be too broad if headers are not well-organized.
# A more precise way is to list specific headers or use compiler-generated dependencies.
# For this project size, explicitly listing primary headers for objects is fine.
$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Explicit dependencies for object files on their corresponding headers and shared headers
$(SRCDIR)/main.o: $(SRCDIR)/main.cpp $(SRCDIR)/data_generator.h $(SRCDIR)/filter_threshold.h $(SRCDIR)/blocking_queue.h
$(SRCDIR)/data_generator.o: $(SRCDIR)/data_generator.cpp $(SRCDIR)/data_generator.h $(SRCDIR)/blocking_queue.h
$(SRCDIR)/filter_threshold.o: $(SRCDIR)/filter_threshold.cpp $(SRCDIR)/filter_threshold.h $(SRCDIR)/blocking_queue.h

# Clean target: remove object files and the executable
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

# Phony targets: targets that are not actual files
.PHONY: all clean
