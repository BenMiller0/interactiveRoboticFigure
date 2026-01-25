CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude
TARGET = tea_animatronic
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/PCA9685.cpp
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/PCA9685.o

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
