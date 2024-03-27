
set( SOURCE_FILES 
            DateTime.cpp
             EpollReactor.cpp 
             LightThread.cpp 
             MallocHooks.cpp 
             PollReactor.cpp 
             Pointer.cpp 
             Reactor.cpp 
             SocketUtils.cpp 
             StringUtils.cpp
             Timer.cpp )
if ( BUILD_SHARED_LIBS ) 
    add_library( hbthreads SHARED ${SOURCE_FILES} )
else()
    add_library( hbthreads STATIC ${SOURCE_FILES} )
endif()

target_link_libraries( hbthreads boost )
set( HEADERS 
    AsmUtils.h
    BufferPrinter.h
    DateTime.h
    EpollReactor.h 
    Histogram.h
    ImportedTypes.h    
    LightThread.h 
    MallocHooks.h 
    Pointer.h 
    PollReactor.h 
    Reactor.h 
    SocketUtils.h 
    StringUtils.h
    Timer.h 
)
set_target_properties( hbthreads PROPERTIES PUBLIC_HEADER "${HEADERS}" )
target_include_directories( hbthreads PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. )

install (TARGETS hbthreads 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/hbthreads
)

#----------------------------------------------------------------------------------------
# Testing
#----------------------------------------------------------------------------------------
if ( BUILD_TESTS )
    add_executable( unit_tests 
    BufferPrinterUnitTests.cpp
    DateTimeUnitTests.cpp
    EpollReactorUnitTests.cpp
    ImportedTypesUnitTests.cpp
    IntrusiveIndexListUnitTests.cpp
    LightThreadUnitTests.cpp
    MallocHooksUnitTests.cpp
    PointerUnitTests.cpp
    PollReactorUnitTests.cpp
    ReactorUnitTests.cpp
    SocketUtilsUnitTests.cpp
    StringUtilsUnitTests.cpp
    TimerUnitTests.cpp )
    target_link_libraries( unit_tests GTest::gtest_main hbthreads  )

    gtest_discover_tests( unit_tests )
    list( APPEND ALL_TESTS unit_tests )
endif()
