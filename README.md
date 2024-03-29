# NeTest
🔫 A super simple network service testing tool. It just send and recv TCP/UDP packet, then generate the statistics.

# Get started

Very easy to start the network service testing:
1. Define your request & reply packet format in `pkt_impl.c`, and implement the interfaces in `pkt_impl.c`. *(or just use the default implementation, that is ok)*
2. Configure the testing arguments in `config.json`.
3. `make`, then you can run `./netest` to test your service.

# Configuration

Edit `config.json` to configure the test options.

```

"protocol"              - UDP or TCP

"totalSessions"         - total number of sessions
"concurrentNum"         - max concurrent sessions number at the same time
"messegeNumPerConn"     - sent message numbers after establish connect in each session
"recordUnit"            - the timepoint will be record every "recordUnit" sessions
	
"serverIP"              - IP address of the server to be test
"serverPort"            - Service Port of the server to be test

```

# Default Servers

Some default **echo servers** are provided under directory `TCPserver` and `UDPserver`. 

Some of them are stupid loop server. Some of them are multi-thread server. 

Some of them use `epoll`. 

And some of them use coroutine. The coroutine servers use another project of mine: [`dyco-coroutine`](https://github.com/piaodazhu/dyco-coroutine). To build and run these server, you need to take several simple steps:
1. `git clone https://github.com/piaodazhu/dyco-coroutine.git`. 
2. `cd dyco-coroutine`, then run `make && sudo make install`. 
3. go back to netest directory and run `make` again.

# Note

**In configuration file, `totalSessions` should be more than 3 times than `concurrentNum`.**

If any question, feel free to open issues. PR is welcome.
