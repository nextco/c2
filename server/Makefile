CXX ?= g++
CXXFLAGS ?= -Wall -Wextra -O2 -std=c++17 -pthread
LDFLAGS ?= -pthread
TARGET := server
SRC := main.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	$(RM) $(TARGET)
