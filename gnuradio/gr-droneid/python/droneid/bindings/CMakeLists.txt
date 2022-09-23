# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT droneid_sources)
    MESSAGE(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT droneid_sources)

########################################################################
# Check for pygccxml
########################################################################
GR_PYTHON_CHECK_MODULE_RAW(
    "pygccxml"
    "import pygccxml"
    PYGCCXML_FOUND
    )

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND droneid_python_files
    misc_utils_python.cc
    decode_python.cc
    demodulation_python.cc
    lte_decode_python.cc
    extractor_python.cc
    normalized_xcorr_estimate_python.cc python_bindings.cc)

GR_PYBIND_MAKE_OOT(droneid
   ../../..
   gr::droneid
   "${droneid_python_files}")

# copy bindings extension for use in QA test module
add_custom_command(TARGET droneid_python POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:droneid_python>
    ${PROJECT_BINARY_DIR}/test_modules/gnuradio/droneid/
)

install(TARGETS droneid_python DESTINATION ${GR_PYTHON_DIR}/gnuradio/droneid COMPONENT pythonapi)
