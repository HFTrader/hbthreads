# hbthreads
A coroutine-based reactor library with a few unique features:
- simple to read for learning purposes
- minimal, voluntarily incomplete so it can be easily extended
- built-in intrusive pointer with integrated pool allocator (no new/malloc calls)
- uncomplicated coroutine semantics based on very basic (and fast) stack primitives 
- event reactor with poll() and epoll() support (anyone still using select?)
- coroutines embedded as reactor dispatch mechanism

# Motivation
The primary motivation is to show that network/socket/coroutine programming does not need
to be necessarily complicated. I hope also to provide some learning material for my charity blog at 
https://lucisqr.substack.com/

# TODO
- Conan recipe
- Unit Tests
- Performance Tests
-- Context switch 
-- Latency tests
