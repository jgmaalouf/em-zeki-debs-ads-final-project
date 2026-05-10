CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude -Werror

LIB_SRCS := src/UserDatabase.cpp src/Compatibility.cpp \
            src/SwipeGraph.cpp src/CityGraph.cpp \
            src/SwipeDeck.cpp src/Session.cpp src/main.cpp

all:
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(LIB_SRCS) -o build/app

clean:
	rm -rf build

.PHONY: all run clean
