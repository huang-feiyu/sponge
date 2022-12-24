# Lab 0 Writeup

> [Checkpoint 0](https://cs144.github.io/assignments/lab0.pdf):
> networking warmup

* Set up an installation of Linux
* Perform tasks over the Internet by hand
* Write a small program in C++ to fetch a Web page over the Internet
* Implement a reliable stream of bytes between a writer and a reader

## Prepare

Environment:

* OS: Fedora 36 (Kernel 6.0.11)
* IDE: CLion 2022.2.4
* CXX: 8.2.0 => `export CC=gcc82; export CXX=g++82`

## Task #1: Networking by hand

1. Fetch a Web page.
2. Send yourself an email.
3. Listening and connecting. (use `ncat`)

## Task #2: Writing a network program using an OS stream socket

> [Sponge Documentation](https://cs144.github.io/doc/lab0/)

OS provided feature: *Stream Socket*. It is supported by TCP (Transmission
Control Protocol): The two computers have to cooperate to make sure that each
byte in the stream eventually gets delivered, in its proper place in line, to
the stream socket on the other side.

In this task, what we need to do is to use OS's TCP to fetch a web page from a
web server. => "webget"

1. Init socket
2. Connect to host
3. Add request headers
4. Read from socket
5. Close socket

## Task #3: An in-memory reliable byte stream

Implement a reliable byte stream => `libsponge/byte_stream`

-- (I have to say that the C++ doc style is so weird.)

Use a deque as the main data structure to ensure FIFO order, and provide peek &
pop operations. Only one thing to note here: only if input ended and buffer is
empty, byte stream is done.
