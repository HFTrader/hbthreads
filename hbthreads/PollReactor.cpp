#include "PollReactor.h"

using namespace hbthreads;

PollReactor::PollReactor(MemoryStorage* mem, DateTime timeout)
    : Reactor(mem), _fds(mem), _sockets(mem) {
    assert(mem != nullptr && "MemoryStorage must not be null");
    _timeout = timeout;
    _dirty = false;
}

void PollReactor::work() {
    // Delayed rebuild: Only rebuild when dirty flag is set.
    // This allows batching multiple monitor()/removeSocket() calls.
    if (_dirty) rebuild();
    if (_fds.empty()) return;
    int nd = ::poll(_fds.data(), _fds.size(), _timeout.msecs());
    if (nd > 0) {
        for (pollfd& pfd : _fds) {
            if ((pfd.revents & POLLIN) != 0) {
                notifyEvent(pfd.fd, EventType::SocketRead);
            }
            if ((pfd.revents & (POLLNVAL | POLLERR)) != 0) {
                notifyEvent(pfd.fd, EventType::SocketError);
            }
        }
    }
}

void PollReactor::onSocketOps(int fd, Operation ops) {
    _dirty = true;
    switch (ops) {
        case Operation::Added: _sockets.insert(fd); break;
        case Operation::Removed: _sockets.erase(fd); break;
        // We do not handle these yet - TODO
        case Operation::Modified:
        case Operation::NA: break;
    }
}

void PollReactor::rebuild() {
    _fds.resize(_sockets.size());
    int index = 0;
    for (int fd : _sockets) {
        pollfd& pfd(_fds[index]);
        pfd.fd = fd;
        pfd.events = POLLIN;
        index++;
    }
    _dirty = false;
}
