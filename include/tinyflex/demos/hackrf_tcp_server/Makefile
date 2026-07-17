# Simple Makefile for hackrf_tcp_server
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
LDFLAGS = -lhackrf
TARGET = hackrf_tcp_server
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

