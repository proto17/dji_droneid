/* -*- c++ -*- */

#define DRONEID_API

%include "gnuradio.i"           // the common stuff
%include "std_vector.i"

//load generated python docstrings
%include "droneid_swig_doc.i"

%{
#include "droneid/extractor.h"
#include "droneid/time_sync.h"
#include "droneid/demodulation.h"
#include "droneid/misc_utils.h"
//#include "droneid/utils.h"
%}

%include "droneid/extractor.h"
GR_SWIG_BLOCK_MAGIC2(droneid, extractor);
%include "droneid/time_sync.h"
GR_SWIG_BLOCK_MAGIC2(droneid, time_sync);
//%include "droneid/utils.h"

//%{
//    unsigned int get_long_cp_len(double sample_rate);
//unsigned int get_short_cp_len(double sample_rate);
//unsigned int get_fft_size(double sample_rate);
//%}
%include "droneid/demodulation.h"
GR_SWIG_BLOCK_MAGIC2(droneid, demodulation);
%include "droneid/misc_utils.h"
