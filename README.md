# dji_droneid

*THIS IS A WORK IN PROGRESS!!!*

This project aims to demodulate DJI DroneID frames and eventually be able to craft arbitrary DroneID frames that can be sent with an SDR

The `.m` files in this project *should* work with Octave 5.2.0 and MATLAB

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
