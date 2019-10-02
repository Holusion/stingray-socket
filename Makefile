
all: build

tester:
	g++ main.cpp -I./include -std=c++11 -o tester $(CPPFLAGS)

build:
	g++ socket.cpp -I./include  -std=c++11 -fPIC -DPIC -shared -o socket.so -Wl,--no-whole-archive $(CPPFLAGS)

install:
	install -d -g root -o root -m 744 /etc/stingray
	install -d -g root -o root -m 744 /etc/stingray/modules.d/
	cp socket.so /etc/stingray/modules.d/socket.so
