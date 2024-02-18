
set( SOURCE_FILES 
            DateTime.cpp
             EpollReactor.cpp 
             LightThread.cpp 
             MallocHooks.cpp 
             PollReactor.cpp 
             Pointer.cpp 
             Reactor.cpp 
             SocketUtils.cpp 
             Timer.cpp )
if ( BUILD_SHARED_LIBS ) 
    add_library( hbthreads SHARED ${SOURCE_FILES} )
else()
    add_library( hbthreads STATIC ${SOURCE_FILES} )
endif()

target_link_libraries( hbthreads boost )
set( HEADERS LightThread.h Pointer.h Reactor.h PollReactor.h 
    EpollReactor.h MallocHooks.h Timer.h SocketUtils.h )
set_target_properties( hbthreads PROPERTIES PUBLIC_HEADER "${HEADERS}" )

#----------------------------------------------------------------------------------------
# Testing
#----------------------------------------------------------------------------------------
if ( BUILD_TESTS )
    add_executable( unit_tests 
    BufferPrinterUnitTests.cpp
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
    list( APPEND ALL_TESTS unit_tests )
endif()
