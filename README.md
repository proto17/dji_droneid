# DJI DroneID RF Analysis

*THIS IS A WORK IN PROGRESS!!!*

Also, the credit for this work is not mine alone.  I have several people who are helping me through this process.  Without their hints about how the signal is structured I would not have gotten very far in this short a time.  So thank you very much to those who have assisted me to this point!!!!

To that point, huge thanks to the following people / groups:
- Chair for System Security @ Ruhr-Universität Bochum / CISPA Helmholtz Center for Information Security

![Title Picture](Octave.png?raw=true)


This project aims to demodulate DJI DroneID frames and eventually be able to craft arbitrary DroneID frames that can be sent with an SDR

The `.m` files in this project *should* work with Octave 5.2.0 and MATLAB.  If using Octave, make sure to install the `signal` package

The IQ file used in this example will not be made available publicly as it likely contains GPS information about where the drone was when the recording was taken.  The drone used in testing is the DJI Mini 2 with no modifications.  Recordings were taken with an Ettus B205-mini at a sampling rate of 30.72 MSPS.  The signal of interest is in 2.4 GHz and will show up every 600 ms or so.  It will be 10 MHz wide (15.56 MHz with guard carriers).  

# How to Process Samples
You will need to record DroneID bursts with an SDR, save the samples as 32-bit floating point IQ data, and then edit the `matlab/updated_scripts/process_file.m` script to read your file.  If there is a major frequency offset (say you recorded 7 MHz off center) you will need to specify that in the script.

List of tasks:
 - Identify ZC sequence (done)
 - Detect ZC sequence (done)
 - Coarse frequency offset detection/correction (done)
 - Fine frequency offset detection/correction (skipped)
 - Phase correction (done)
 - Equalization (done)
 - Symbol extraction (done)
 - Descrambling (done)
 - Turbo Product Code removal (done)
 - Deframing (done)
 - Profit!

## Identify ZC Sequence
This was completed by brute forcing through all of the possible ZC sequence root indexes for 601 output samples.  I was at a complete standstill until it was suggested that I try all possible combos, and it turns out that it wasn't all that difficult.

The tricky bit was that you need to generate 1 plus the number of data carriers (always 600) worth of ZC sequence, zero the center element, map the generated samples onto the center of an FFT (number of bins is based on sample rate), and compute the IFFT.  The resulting time domain samples can be cross correlated against a recording of the same sample rate to find the sequence.

The root indices are
- 600 for the first ZC sequence
- 147 for the second ZC sequence


## Detect ZC Sequence
This is being done using a normalized cross correlation with a golden reference.  The first ZC sequence is generated in MATLAB and that copy is used to find the ZC sequence in the collect.

## Coarse CFO Detection/Adjustment
Currently using the cyclic prefix of the first OFDM symbol to detect and correct for coarse frequency offset.  

I don't think this will work should the CFO be very large.  I suspect anything over 1 FFT bin (15 KHz) is going to fail to demodulate

## Fine CFO Detection/Adjustment
It seems that the coarse offset estimate using the cyclic prefix is doing just fine and does not need help from a fine offset adjustment.

## Phase Correction
This ended up being a little more difficult than expected.  If the time that the first sample is plucked from the airwaves does not happen at exactly the right time, then there will be a fractional time offset.  This fractional time offset will show up in the frequency domain as a walking phase offset.  Meaning that each OFDM carrier has an accumulating phase offset left to right.  If the first carrier might have an offset of 0.1 radians, the next 0.2 radians, then 0.3 radians, etc.  Since there are no pilots, the phase offset estimate has to come from the ZC sequences.  But, using the first or second ZC sequences to adjust phase for each OFDM symbol works fine for the OFDM symbols directly to the left and right (in time) of the used ZC sequence, but not so well for the others.  Each other OFDM symbol will have a constellation that is rotating more the further from the ZC sequence it's located.  

To deal with this, the channel for each ZC sequence is calculated, and the phase difference between those channels is calculated.  This difference divided by 2 is the walking phase offset.  That walking phase offset is then used to adjust each of the other OFDM symbols to get all of the constallations locked properly.

## Symbol Extraction
This is super simple and just requires being time and frequency aligned with knowledge of the cyclic prefixes

## Descrambling
Thanks to some kind souls this has been figured out.  In the case of the 9 OFDM symbol drones, the first OFDM symbol gets zero'ed out by the scrambler, and then the scrambler starts over for the remaining 8 OFDM symbols.

## Turbo Product Code Removal
There is a C++ application in the repository to handle this

## Deframing
This is known, and a deframer will be added at a later date

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
Each burst is 9 OFDM symbols with two symbols using a long cyclic prefix and the others using the short sequence (see below for edge case)

## OFDM Structure
As mentioned above, there are 9 OFDM symbols (see below for edge case)
The 4th and 6th symbols (1-based) appear to be Zadoff-Chu (ZC) sequences (these are used in LTE and I know for a fact they are present in the uplink signal for Ocusync).  The parameters for the ZC sequences are not known.  I have first hand knowledge that the sequences are almost certainly not following the standard.
The remaining OFDM symbols carry just a QPSK.  If there are pilots they are either QPSK pilots or a 45 degree offset BPSK.  As pointed out by https://github.com/tmbinc/random/tree/master/dji/ocusync2 the DC carrier appears to always be sitting around 45 degrees with a much smaller amplitude than the data carriers.  Not totally sure what's going on there.

### 8 OFDM Symbol Edge Case
It turns out that some drones don't send the first OFDM symbol.  Instead they skip it and end up with their OFDM symbol 1 being the 9 OFDM symbol cases 2nd OFDM symbol.

Since there's no change to the order of the symbols, and the 9 OFDM symbol case's first OFDM symbol being useless, the scripts in this repo treat all bursts as having 9 OFDM symbols and just don't do anything with the first OFDM symbol.

# Current Questions

## Cross Correlation
MATLAB can do a normalized cross correlation using `xcorr(X, Y, 0, 'normalized')` but it's *crazy* slow.  So I wrote my own function ![normalized_xcorr.m](matlab/updated_scripts/normalized_xcorr_fast.m) that is ~ 8x faster than `xcorr` but over 100x slower than the `filter` function that was being used.  If anyone has a better idea on how to do a truly normalized (0.0 - 1.0) cross correlation please let me know.

## Burst Extraction
Right now the normalized cross correlation mentioned above is used to find the bursts using ![process_file.m](matlab/updated_scripts/process_file.m).  Since the cross correlation is so slow, a file that contains tens of millions of samples takes a long time in MATLAB and an even longer time in Octave.  The old method of using the `filter` function was blazing fast, but there was no way to know what the correlation thresholds needed to be without making multiple passes through the file.  Energy detection would probably work, but that falls apart in low SNR conditions.  A normalized autocorrelation would likely be as slow as the cross correlation, which probably rules out autocorrelating for the ZC sequence.  I'd love to hear some ideas to help speed this process up.
