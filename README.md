# DJI DroneID RF Analysis

*THIS IS A WORK IN PROGRESS!!!*

![Alt text](Octave.png?raw=true)


This project aims to demodulate DJI DroneID frames and eventually be able to craft arbitrary DroneID frames that can be sent with an SDR

The `.m` files in this project *should* work with Octave 5.2.0 and MATLAB.  If using Octave, make sure to install the `signal` package

The IQ file used in this example will not be made available publicly as it likely contains GPS information about where the drone was when the recording was taken.  The drone used in testing is the DJI Mini 2 with no modifications.  Recordings were taken with an Ettus B205-mini at a sampling rate of 30.72 MSPS.  The signal of interest is in 2.4 GHz and will show up every 600 ms or so.  It will be 10 MHz wide (15.56 MHz with guard carriers).  

List of tasks:
 - Identify ZC sequence
 - Detect ZC sequence (done)
 - Coarse frequency offset detection/correction (skipped - sorta)
 - Fine frequency offset detection/correction (skipped)
 - Phase correction (skipped)
 - Symbol extraction (done)
 - Descrambling (current struggle)
 - Turbo Product Code removal
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

Update: I've skipped this step as my primary goal is to get to bits.  For some reason my logic that uses the cyclic prefix is off by a factor of 9.  For now I have done the correction by hand.

## Fine CFO Detection/Adjustment
This likely requires the knowledge of the ZC sequence parameters.  Fine CFO needs a long sequence of samples to work well, and they need to either be known ahead of time, or repeated in the transmission.
Update: I've skipped this as well.  My corrections from the Coarse CFO have taken care of any fine offset as well.

## Phase Correction
Another one that needs a known sequence in the burst.  The good news is that all of the data carriers are QPSK, so there's only 4 ways to rotate the constellation.

Update: I've skipped this too.  Handily the burst I am looking at right now didn't need much in the way of phase adjustments

## Symbol Extraction
This is super simple and just requires being time and frequency aligned with knowledge of the cyclic prefixes

## Descrambling
Rumor is that there is a scrambler in use.  It's not clear to me if the scrambler is before or after FEC, but it will need to be dealt with.  Supposedly it's LTE standard.

Update: the scrambler is definitely before the FEC.  I found a really nice writeup about it at https://www.sharetechnote.com/html/Handbook_LTE_PseudoRandomSequence.html.  The scrambling sequence seems to be made out of two Linear Feedback Shift Registers (LFSRs) combined together.  The intital state of the first LFSR is a constant value, but the second LFSR needs certain parameters that relate to the link.  Unfortunately I don't have those parameters.  The only good news is that is *only* a 31-bit exhaust to brute force.  So ~ 2.5 billion attempts and you're assured success!  In the event that the Turbo decoder parameters are magically known then maybe this won't be so bad.

## Turbo Product Code Removal
It is assumed that the demodulated data contains an LTE standard TPC.  There are libraries that can handle this (hopefully)

A new wrinkle here is that under the Turbo code is going to be "Rate Matching" bits.  I have no idea if that's going to be a standard process that doesn't vary depending on link parameters that aren't already known.

## Deframing
The underlying data should be in a known DJI format.  If so then this step should be pretty simple

# Known Values
## Frequencies
Look in 2.4 GHz for the Drone ID frames.  Might be best to go into the DJI app and tell the downlink to use 5.8 GHz.  This will keep the downlink (and probably uplink) out of 2.4 GHz.  Drone ID will not move from 2.4 GHz
Some frequencies I have noted thus far:
- 2.4595 GHz
- 2.4445 GHz
- 2.4295 GHz
- 2.4145 GHz
- 2.3995 GHz

There might be others, but that's just what I've seen

## Burst Duration/Interval
The Drone ID bursts happen ~ every 600 milliseconds
The frequency varies and I've heard that there is a pattern to it, but cannot validate with my SDR as the bandwidth isn't high enough
Each burst is 9 OFDM symbols with two symbols using a long cyclic prefix and the others using the short sequence

## OFDM Structure
As mentioned above, there are 9 OFDM symbols.  
The 4th and 6th symbols (1-based) appear to be Zadoff-Chu (ZC) sequences (these are used in LTE and I know for a fact they are present in the uplink signal for Ocusync).  The parameters for the ZC sequences are not known.  I have first hand knowledge that the sequences are almost certainly not following the standard.
The remaining OFDM symbols carry just a QPSK.  If there are pilots they are either QPSK pilots or a 45 degree offset BPSK.  As pointed out by https://github.com/tmbinc/random/tree/master/dji/ocusync2 the DC carrier appears to always be sitting around 45 degrees with a much smaller amplitude than the data carriers.  Not totally sure what's going on there.

