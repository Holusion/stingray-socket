
all: build


build:
	g++ socket.cpp -I./include  -std=c++11 -fPIC -DPIC -shared -o socket.so -Wl,--no-whole-archive 
