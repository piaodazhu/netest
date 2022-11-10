all: tcp_coroutine tcp_coro_stack tcp_coropool tcp_epoll tcp_loop tcp_pthread

tcp_coropool: tcp_coropool.c 
	gcc -o $@ $@.c -ldyco -O3

tcp_coroutine: tcp_coroutine.c
	gcc -o $@ $@.c -ldyco -O3

tcp_coro_stack: tcp_coro_stack.c 
	gcc -o $@ $@.c -ldyco -O3

tcp_epoll: tcp_epoll.c
	gcc -o $@ $@.c -O3

tcp_loop: tcp_loop.c
	gcc -o $@ $@.c -O3

tcp_pthread: tcp_pthread.c
	gcc -o $@ $@.c -lpthread -O3

clean:
	rm -rf tcp_coroutine tcp_epoll tcp_loop tcp_pthread tcp_coro_stack tcp_coropool