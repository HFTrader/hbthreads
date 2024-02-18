
#----------------------------------------------------------------------------------------
# The library itself and usage examples
#----------------------------------------------------------------------------------------
list( APPEND ALL_TESTS timertest udptest tcptest switchtest coroexample )

add_executable( timertest timertest.cpp  )
target_link_libraries( timertest hbthreads boost )
add_test( NAME timertest COMMAND timertest )

add_executable( udptest udptest.cpp  )
target_link_libraries( udptest hbthreads boost )
add_test( NAME udptest COMMAND timertest )

add_executable( tcptest tcptest.cpp  )
target_link_libraries( tcptest hbthreads boost )
add_test( NAME tcptest COMMAND tcptest )

add_executable( switchtest switchtest.cpp  )
target_link_libraries( switchtest hbthreads boost )

include(CheckCSourceRuns)
check_c_source_runs("#include <sys/eventfd.h>\nint main(){ return (eventfd(0,0)>=0) ? 0: 1;}" HAS_EVENTFD)

message( "Has evenfd:${HAS_EVENTFD}")

if (HAS_EVENTFD)
    add_executable( perftest perftest.cpp  )
    target_link_libraries( perftest hbthreads boost )
    list( APPEND ALL_TESTS perftest )
endif()

add_executable( coroexample coroexample.cpp )
target_link_libraries( coroexample hbthreads boost )

