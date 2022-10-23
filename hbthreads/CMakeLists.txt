

add_library( hbthreads SHARED 
             LightThread.cpp Pointer.cpp 
             Reactor.cpp PollReactor.cpp EpollReactor.cpp 
             MallocHooks.cpp Timer.cpp SocketUtils.cpp )
target_link_libraries( hbthreads boost )
set( HEADERS LightThread.h Pointer.h Reactor.h PollReactor.h 
    EpollReactor.h MallocHooks.h Timer.h SocketUtils.h )
set_target_properties( hbthreads PROPERTIES PUBLIC_HEADER "${HEADERS}" )

#----------------------------------------------------------------------------------------
# Testing
#----------------------------------------------------------------------------------------
add_executable( unit_tests 
 DateTimeUnitTests.cpp
 EpollReactorUnitTests.cpp
 ImportedTypesUnitTests.cpp
 LightThreadUnitTests.cpp
 MallocHooksUnitTests.cpp
 PointerUnitTests.cpp
 PollReactorUnitTests.cpp
 ReactorUnitTests.cpp
 SocketUtilsUnitTests.cpp
 TimerUnitTests.cpp )
target_link_libraries( unit_tests hbthreads GTest::gtest_main )

gtest_discover_tests( unit_tests )

