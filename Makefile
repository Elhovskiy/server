# Makefile для сервера

# Компилятор и флаги
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2
LIBS = -lssl -lcrypto

# Цели и исходные файлы
TARGET = server
SRCS = server.cpp

# Правила сборки
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean


