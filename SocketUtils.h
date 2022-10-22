#pragma once

namespace hbthreads {

// Creates a tcp socket
int createTCPSocket();

// Creates a TCP socket and binds to a given address/port
int createAndBindTCPSocket(const char *address, int port);

// Creates a datagram socket
int createUDPSocket();

// Creates a datagram socket and binds to address/port
int createAndBindUDPSocket(const char *address, int port);

}  // namespace hbthreads