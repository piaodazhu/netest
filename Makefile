all: netest server

server:
	make -C TCPserver
	make -C UDPserver

netest: cJSON.o pkt_impl.o netest.o
	gcc -o $@ $^ -O3

cJSON.o: cJSON.c
	gcc -c $^

pkt_impl.o: pkt_impl.c
	gcc -c $^

netest.o: netest.c
	gcc -c $^	

clean:
	rm -rf netest
	rm -rf *.o
	make -C TCPserver clean
	make -C UDPserver clean

.PHONY: clean