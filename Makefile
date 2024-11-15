CXX = g++
CXXFLAGS = -Wall -std=c++17 -I src

TARGET = server
SRCS = main.cpp server.cpp data_processor.cpp logger.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) -lssl -lcrypto

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) src/*.o


