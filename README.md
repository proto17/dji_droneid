# dji_droneid

*THIS IS A WORK IN PROGRESS!!!*

This project aims to demodulate DJI DroneID frames and eventually be able to craft arbitrary DroneID frames that can be sent with an SDR

The `.m` files in this project *should* work with Octave 5.2.0 and MATLAB

The IQ file used in this example will not be made available publicly as it likely contains GPS information about where the drone was when the recording was taken.  The drone used in testing is the DJI Mini 2 with no modifications.  Recordings were taken with an Ettus B205-mini at a sampling rate of 30.72 MSPS.  The signal of interest is in 2.4 GHz and will show up every 600 ms or so.  It will be 10 MHz wide (15.56 MHz with guard carriers).  

List of tasks:
 - Identify ZC sequence
 - Detect ZC sequence (done)
 - Coarse frequency offset detection/correction (in progress)
 - Fine frequency offset detection/correction
 - Phase correction
 - Symbol extraction (done)
 - Turbo Product Code removal
 - Descrambling
 - Deframing
 - Profit!

## Identify ZC Sequence
There are two Zadoff Chu sequences in each burst.  It's unclear as to the correct parameters to generate these sequences.  It's possible that they will have to be brute forced

## Detect ZC Sequence
This has been done by exploiting the fact that the first ZC sequence is symmetric in the time domain (the second might be too) and that a ZC sequence is a CAZAC (constant amplitude, zero autocorrelation).  
To find the sequence you just need to search through the signal one sample at a time takeing a window of `fft_size` samples, reversing the second half, and cross correlating.

## Coarse CFO Detection/Adjustment
Without knowing the ZC sequence parameters, all that remains are the cyclic prefixes, and cross correlating the ZC sequence halves
Neither is great as both are spread pretty far apart.  Nothing like the 16 sample IEEE 802.11 preambles that are amazing for coarse CFO.  Perhaps the second ZC sequence can be used?  In the time domain it looks like it repeats itself over and over rapidly.

## Fine CFO Detection/Adjustment
This likely requires the knowledge of the ZC sequence parameters.  Fine CFO needs a long sequence of samples to work well, and they need to either be known ahead of time, or repeated in the transmission.

## Phase Correction
Another one that needs a known sequence in the burst.  The good news is that all of the data carriers are QPSK, so there's only 4 ways to rotate the constellation.

## Symbol Extraction
This is super simple and just requires being time and frequency aligned with knowledge of the cyclic prefixes

## Turbo Product Code Removal
It is assumed that the demodulated data contains an LTE standard TPC.  There are libraries that can handle this (hopefully)

## Descrambling
Rumor is that there is a scrambler in use.  It's not clear to me if the scrambler is before or after FEC, but it will need to be dealt with.  Supposedly it's LTE standard.

## Deframing
The underlying data should be in a known DJI format.  If so then this step should be pretty simple

