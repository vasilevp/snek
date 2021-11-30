CXX ?= g++ -std=c++17

snek: main.o
	${CXX} -o $@ $^ -lncurses

clean:
	rm -rf main.o snek

main.o: main.cpp *.hpp *.h
	${CXX} -c -o $@ main.cpp
