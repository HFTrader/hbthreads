
#----------------------------------------------------------------------------------------
# The library itself and usage examples
#----------------------------------------------------------------------------------------
add_executable( timertest timertest.cpp  )
target_link_libraries( timertest hbthreads boost )
add_test( NAME timertest COMMAND timertest )

add_executable( udptest udptest.cpp  )
target_link_libraries( udptest hbthreads boost )
add_test( NAME udptest COMMAND timertest )

add_executable( tcptest tcptest.cpp  )
target_link_libraries( tcptest hbthreads boost )
add_test( NAME tcptest COMMAND tcptest )


