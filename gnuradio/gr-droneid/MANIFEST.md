title: The DRONEID OOT Module
brief: Short description of gr-droneid
tags: # Tags are arbitrary, but look at CGRAN what other authors are using
  - sdr
author:
  - Author Name <authors@email.address>
copyright_owner:
  - Copyright Owner 1
license:
gr_supported_version: # Put a comma separated list of supported GR versions here
#repo: # Put the URL of the repository here, or leave blank for default
#website: <module_website> # If you have a separate project website, put it here
#icon: <icon_url> # Put a URL to a square image here that will be used as an icon on CGRAN
---
A longer, multi-line description of gr-droneid.
You may use some *basic* Markdown here.
If left empty, it will try to find a README file instead.

This module is currently *very much* an Alpha.  Please don't rely on it for *anything*.  It tends to trail behind the MATLAB/Octave code by weeks!

It currently does some very basic functions:
- Burst detection by using cross correlating for the ZC sequence
- Burst extraction using the correlation scores from the cross correlation above (see note 1)
- Coarse CFO estimation and correction (see note 2) 
- OFDM data carrier extraction

Some things still left to do:
- Channel equalization using the two ZC sequences (https://github.com/proto17/dji_droneid/blob/main/matlab/updated_scripts/process_file.m#L119-L134 and https://github.com/proto17/dji_droneid/blob/main/matlab/updated_scripts/process_file.m#L155-L166)
- Function-izing the Turbo decoder and rate matcher (https://github.com/proto17/dji_droneid/blob/main/cpp/remove_turbo.cc) and using the function(s) to deal with Turbo decoding and rate matching to output full frames
- Parse the frame to spit out something that users can actually use (maybe JSON?)
- Create a more robust timing offset estimator (likely autocorrelating the ZC sequence(s))
- Figure out how to deal with integer frequency offsets (currently just able to detect and correct for fractional offsets)
