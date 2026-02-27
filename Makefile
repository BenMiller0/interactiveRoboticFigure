CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude
LDFLAGS = -lasound -lpthread
TARGET = tea_animatronic
SRC_DIR = src
BUILD_DIR = build

SOURCES = $(SRC_DIR)/main.cpp \
          $(SRC_DIR)/i2c/PCA9685.cpp \
          $(SRC_DIR)/audio/Audio.cpp \
          $(SRC_DIR)/control/TaroUI.cpp \
          $(SRC_DIR)/control/RandomController.cpp \
          $(SRC_DIR)/actuation/Mouth.cpp \
          $(SRC_DIR)/actuation/Wings.cpp \
          $(SRC_DIR)/actuation/Neck.cpp \
          $(SRC_DIR)/ai/AIVoice.cpp

OBJECTS = $(BUILD_DIR)/main.o \
          $(BUILD_DIR)/PCA9685.o \
          $(BUILD_DIR)/Audio.o \
          $(BUILD_DIR)/TaroUI.o \
          $(BUILD_DIR)/RandomController.o \
          $(BUILD_DIR)/Mouth.o \
          $(BUILD_DIR)/Wings.o \
          $(BUILD_DIR)/Neck.o \
          $(BUILD_DIR)/AIVoice.o

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/actuation/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/audio/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/i2c/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/control/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/ai/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean