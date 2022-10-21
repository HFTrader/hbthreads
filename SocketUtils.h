#pragma once

namespace hbthreads {

int createUDPSocket();
int createAndBindUDPSocket(const char *address, int port);

}  // namespace hbthreads