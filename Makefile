CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude
LDFLAGS = -lasound -lpthread
TARGET = tea_animatronic
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/PCA9685.cpp $(SRC_DIR)/AudioMouth.cpp $(SRC_DIR)/Wings.cpp $(SRC_DIR)/TaroUI.cpp
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/PCA9685.o $(BUILD_DIR)/AudioMouth.o $(BUILD_DIR)/Wings.o $(BUILD_DIR)/TaroUI.o

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
