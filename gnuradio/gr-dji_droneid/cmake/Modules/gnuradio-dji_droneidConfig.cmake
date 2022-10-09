find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_DJI_DRONEID gnuradio-dji_droneid)

FIND_PATH(
    GR_DJI_DRONEID_INCLUDE_DIRS
    NAMES gnuradio/dji_droneid/api.h
    HINTS $ENV{DJI_DRONEID_DIR}/include
        ${PC_DJI_DRONEID_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_DJI_DRONEID_LIBRARIES
    NAMES gnuradio-dji_droneid
    HINTS $ENV{DJI_DRONEID_DIR}/lib
        ${PC_DJI_DRONEID_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-dji_droneidTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_DJI_DRONEID DEFAULT_MSG GR_DJI_DRONEID_LIBRARIES GR_DJI_DRONEID_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_DJI_DRONEID_LIBRARIES GR_DJI_DRONEID_INCLUDE_DIRS)
