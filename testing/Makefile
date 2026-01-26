CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
TARGET = servo_control
SRC = servo_control.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
