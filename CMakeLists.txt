# Options to enable locally-compiled boost
set( USE_LOCAL_BOOST OFF CACHE BOOL "Use locally build boost (see build_minimal_boost.sh)")
set( LOCAL_BOOST_PREFIX "/usr/local" CACHE PATH "Location of boost for the local build")
set( LOCAL_BOOST_LIB_PREFIX "${LOCAL_BOOST_PREFIX}/lib" CACHE PATH "Location of boost libraries")
set( LOCAL_BOOST_INCLUDE_PREFIX "${LOCAL_BOOST_PREFIX}/include" CACHE PATH "Location of boost headers")

# Turn this on only after careful consideration of your application usage
# Note that if you enable this, you will need to add -DUSE_SMALL_SIZE and/or
#  -DUSE_SMALL_COUNTER in your project as well.
set( USE_SMALL_SIZE OFF CACHE BOOL  "Use small size type for memory management" )
set( USE_SMALL_COUNTER OFF CACHE BOOL "Use small counter for intrusive pointers" )

project( hbthreads LANGUAGES C CXX )
cmake_minimum_required( VERSION 3.10 )

if ( USE_SMALL_SIZE )
    add_compile_options( -DUSE_SMALL_SIZE )
endif()
if ( USE_SMALL_COUNTER )
    add_compile_options( -DUSE_SMALL_COUNTER )
endif()

#----------------------------------------------------------------------------------------
# BOOST dependencies
#----------------------------------------------------------------------------------------
if ( USE_LOCAL_BOOST )
# The intent is to use this together with the script ./build_minimal_boost.sh included
# with this repo. 
macro( add_boost_library libname )
string( TOUPPER ${libname} libname_upcase )
add_library( boost_${libname}_s STATIC IMPORTED GLOBAL )
set_target_properties( boost_${libname}_s PROPERTIES
    IMPORTED_LOCATION "${LOCAL_BOOST_LIB_PREFIX}/libboost_${libname}.a"
    INTERFACE_COMPILE_DEFINITIONS "HAVE_BOOST_${libname_upcase}"
)
list( APPEND BOOST_LIBS_STATIC "boost_${libname}_s")
endmacro()

add_boost_library( context )
add_boost_library( container )

add_library( boost INTERFACE IMPORTED )
set_target_properties( boost PROPERTIES 
    INTERFACE_LINK_LIBRARIES "${BOOST_LIBS_STATIC}"
    INTERFACE_INCLUDE_DIRECTORIES "${LOCAL_BOOST_INCLUDE_PREFIX}"
    INTERFACE_COMPILE_DEFINITIONS "HAVE_LOCAL_BOOST" 
)
else()
# On Ubuntu install with `apt install libboost-all-dev`
find_package(Boost COMPONENTS context container REQUIRED)
add_library( boost INTERFACE IMPORTED )
set_target_properties( boost PROPERTIES 
    INTERFACE_LINK_LIBRARIES "Boost::context;Boost::container"
)
endif()

#----------------------------------------------------------------------------------------
# The library itself and usage example
#----------------------------------------------------------------------------------------
add_library( hbthreads SHARED 
             LightThread.cpp Pointer.cpp 
             Reactor.cpp PollReactor.cpp EpollReactor.cpp 
             MallocHooks.cpp )
target_link_libraries( hbthreads boost )
set( HEADERS LightThread.h Pointer.h Reactor.h PollReactor.h 
    EpollReactor.h MallocHooks.h )
set_target_properties( hbthreads PROPERTIES PUBLIC_HEADER "${HEADERS}" )

add_executable( timertest timertest.cpp  )
target_link_libraries( timertest hbthreads boost   )

#----------------------------------------------------------------------------------------
# Install
#----------------------------------------------------------------------------------------
install (TARGETS hbthreads timertest 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)