all: udp_epoll udp_loop

udp_epoll: udp_epoll.c
	gcc -o $@ $@.c -O3

udp_loop: udp_loop.c
	gcc -o $@ $@.c -O3

clean:
	rm -rf udp_epoll udp_loop