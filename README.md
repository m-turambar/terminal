# terminal
Serial port asynchronous I/O

Basically a hyperterminal, but with colors, crc calculation, etc.

Dependencies:
boost::asio standalone version:
https://think-async.com/Asio/

To build, include the directory that contains the asio library, and link to the windows socket libraries.
e.g.
GCC: g++ main.cpp -o term -IC:\include -lwsock32 -lws2_32 -std=c++14
