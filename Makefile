CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g

all:
	$(CXX) $(CXXFLAGS) src/main.cpp -o planificador

clean:
	rm -f planificador
