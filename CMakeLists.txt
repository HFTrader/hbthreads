#----------------------------------------------------------------------------------------
# Preload options
# These have to be set before project()
#----------------------------------------------------------------------------------------

# Options to enable locally-compiled boost
set( USE_LOCAL_BOOST OFF CACHE BOOL "Use locally build boost (see build_minimal_boost.sh)")
set( LOCAL_BOOST_PREFIX "/usr/local" CACHE PATH "Location of boost for the local build")
set( LOCAL_BOOST_LIB_PREFIX "${LOCAL_BOOST_PREFIX}/lib" CACHE PATH "Location of boost libraries")
set( LOCAL_BOOST_INCLUDE_PREFIX "${LOCAL_BOOST_PREFIX}/include" CACHE PATH "Location of boost headers")
set( BUILD_SHARED_LIBS ON CACHE BOOL "Build shared or static libraries" )
set( BUILD_TESTS ON CACHE BOOL "Build internal unit tests")
set( CMAKE_BUILD_TYPE "RELEASE" CACHE STRING "Default build type" )
set( CMAKE_EXPORT_COMPILE_COMMANDS "ON" CACHE BOOL "Exports a database for clang-tidy")

set( HBTHREADS_USES_CLANG_TIDY OFF CACHE BOOL "Add clang-tidy during compilation" )
if ( HBTHREADS_USES_CLANG_TIDY )
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-p=${CMAKE_BINARY_DIR};--warnings-as-errors=*" CACHE STRING "Clang-tidy command line " )
endif()

# Turn this on only after careful consideration of your application usage
# Note that if you enable this, you will need to add -DUSE_SMALL_SIZE and/or
#  -DUSE_SMALL_COUNTER in your project as well.
set( USE_SMALL_SIZE OFF CACHE BOOL  "Use small size type for memory management" )
set( USE_SMALL_COUNTER OFF CACHE BOOL "Use small counter for intrusive pointers" )

# This will allow VSCode to pick up on dependencies
set( CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export dependency list")

#----------------------------------------------------------------------------------------
# Global project settings
#----------------------------------------------------------------------------------------

cmake_minimum_required( VERSION 3.10 )
project( hbthreads LANGUAGES C CXX VERSION 1.0.1 )

set(CMAKE_CXX_STANDARD 14)
if ( USE_SMALL_SIZE )
    add_compile_options( -DUSE_SMALL_SIZE )
endif()
if ( USE_SMALL_COUNTER )
    add_compile_options( -DUSE_SMALL_COUNTER )
endif()
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/hbthreads )

#----------------------------------------------------------------------------------------
# Dependencies
#----------------------------------------------------------------------------------------

# Let's include Google Tests as a downloadable content so we don't have 
# to force the user to install it in their system
if ( BUILD_TESTS )
    include(FetchContent)
    FetchContent_Declare( googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main )
    FetchContent_MakeAvailable(googletest)
endif()

#----------------------------------------------------------------------------------------
# BOOST complex dependencies
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
        INTERFACE_LINK_LIBRARIES "pthread;${BOOST_LIBS_STATIC}"
        INTERFACE_INCLUDE_DIRECTORIES "${LOCAL_BOOST_INCLUDE_PREFIX}"
        INTERFACE_COMPILE_DEFINITIONS "HAVE_LOCAL_BOOST" 
        INTERFACE_LINK_OPTIONS "-Wl,--no-as-needed"
    )
else()
    # This is the "normal"  cmake boost-supported 
    # On Ubuntu install with `apt install libboost-all-dev`
    find_package(Boost COMPONENTS context container REQUIRED)
    add_library( boost INTERFACE IMPORTED )
    set_target_properties( boost PROPERTIES 
        INTERFACE_LINK_LIBRARIES "pthread;Boost::context;Boost::container"
    )
    target_link_options( boost PUBLIC "-Wl,--no-as-needed" )
endif()

#----------------------------------------------------------------------------------------
# Documentation
#----------------------------------------------------------------------------------------

option(BUILD_DOCS "Build documentation" ON)
if ( BUILD_DOCS )
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        add_custom_target( doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
    endif (DOXYGEN_FOUND)
endif()

#----------------------------------------------------------------------------------------
# The library itself and usage examples
#----------------------------------------------------------------------------------------
if ( BUILD_TESTS )
    enable_testing()
    include(CTest)
    include(GoogleTest)
endif()

# hbthreads contains unit tests so it needs to be inside
add_subdirectory(hbthreads)


if ( BUILD_TESTS )
    add_subdirectory(tests)
    install (TARGETS ${ALL_TESTS}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include/hbthreads
    )
endif()
