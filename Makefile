CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude
LDFLAGS = -lasound -lpthread
TARGET = tea_animatronic
SRC_DIR = src
BUILD_DIR = build

SOURCES = $(SRC_DIR)/main.cpp \
          $(SRC_DIR)/PCA9685.cpp \
          $(SRC_DIR)/Audio.cpp \
          $(SRC_DIR)/TaroUI.cpp \
          $(SRC_DIR)/RandomController.cpp \
          $(SRC_DIR)/actuation/Mouth.cpp \
          $(SRC_DIR)/actuation/Wings.cpp \
          $(SRC_DIR)/actuation/Neck.cpp

OBJECTS = $(BUILD_DIR)/main.o \
          $(BUILD_DIR)/PCA9685.o \
          $(BUILD_DIR)/Audio.o \
          $(BUILD_DIR)/TaroUI.o \
          $(BUILD_DIR)/RandomController.o \
          $(BUILD_DIR)/Mouth.o \
          $(BUILD_DIR)/Wings.o \
          $(BUILD_DIR)/Neck.o

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Rule for src/*.cpp
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule for src/joints/*.cpp
$(BUILD_DIR)/%.o: $(SRC_DIR)/actuation/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean