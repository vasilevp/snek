GXX=g++ -std=c++17

snek: main.o
	${GXX} -o $@ $^ -lncurses

clean:
	rm -rf main.o snek

main.o: main.cpp
	${GXX} -c -o $@ $^
