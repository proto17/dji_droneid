# DJI DroneID RF Analysis

*THIS IS A WORK IN PROGRESS!!!*

Also, the credit for this work is not mine alone.  I have several people who are helping me through this process.  Without their hints about how the signal is structured I would not have gotten very far in this short a time.  So thank you very much to those who have assisted me to this point!!!!

To that point, huge thanks to the following people / groups:
- Chair for System Security @ Ruhr-Universität Bochum / CISPA Helmholtz Center for Information Security

![Alt text](Octave.png?raw=true)


This project aims to demodulate DJI DroneID frames and eventually be able to craft arbitrary DroneID frames that can be sent with an SDR

The `.m` files in this project *should* work with Octave 5.2.0 and MATLAB.  If using Octave, make sure to install the `signal` package

Update(15 April 2022): **The main script (matlab/updated_scripts/process_file.m) is broken for Octave.  I will attempt to address this within the next few days.**

The IQ file used in this example will not be made available publicly as it likely contains GPS information about where the drone was when the recording was taken.  The drone used in testing is the DJI Mini 2 with no modifications.  Recordings were taken with an Ettus B205-mini at a sampling rate of 30.72 MSPS.  The signal of interest is in 2.4 GHz and will show up every 600 ms or so.  It will be 10 MHz wide (15.56 MHz with guard carriers).  

# GNU Radio OOT Module
This branch has the original GNU Radio OOT module for GNU Radio 3.10.  This OOT module does not do a good job of demodulating.  It has several issues with it's start time offset estimation, CFO estimation/correction, and equalization.

The best use of this module is to collect bursts for processing in MATLAB/Octave with `process_file.m`

# How to Process Samples
You will need to record DroneID bursts with an SDR, save the samples as 32-bit floating point IQ data, and then edit the `matlab/updated_scripts/process_file.m` script to read your file.  If there is a major frequency offset (say you recorded 7 MHz off center) you will need to specify that in the script.

__Currently the C++ program that does Turbo decoding and rate matching is not in the repo.  This will be added in the next week or so__

List of tasks:
 - Identify ZC sequence (done)
 - Detect ZC sequence (done)
 - Coarse frequency offset detection/correction (done)
 - Fine frequency offset detection/correction (skipped)
 - Phase correction (done)
 - Symbol extraction (done)
 - Descrambling (done)
 - Turbo Product Code removal (done)
 - Deframing (done)
 - Profit!

## Identify ZC Sequence
There are two Zadoff Chu sequences in each burst.  It's unclear as to the correct parameters to generate these sequences.  It's possible that they will have to be brute forced

Update (9 Apr 2022): Thanks to a tip from someone that's looked at this before I finally have the ZC root values for both symbols 4 and 6.  The tip was to brute force through some number of roots and correlate those roots against the received signal.  So, there is now a script called `brute_force_zc.m` that uses one of the received ZC sequences from the `automatic_detector.m` script to check against.  The recording I am working with is very clean with plenty of SNR and dynamic range (12-bit ADC).  For those that don't want to struggle through it, the roots are 600 for symbol 4, and 147 for symbol 6.  From what I can tell you need to create 601 samples and the middle sample gets set to zero since it maps to the DC subcarrier in an FFT.  Either I am doing something wrong or the first ZC sequence is astoundingly resiliant to frequency offset.  Just for grins I pushed the frequency offset to > 1 MHz off and the normalized cross correlation worked like a champ.

## Detect ZC Sequence
This has been done by exploiting the fact that the first ZC sequence is symmetric in the time domain (the second might be too) and that a ZC sequence is a CAZAC (constant amplitude, zero autocorrelation).  
To find the sequence you just need to search through the signal one sample at a time takeing a window of `fft_size` samples, reversing the second half, and cross correlating.

## Coarse CFO Detection/Adjustment
Without knowing the ZC sequence parameters, all that remains are the cyclic prefixes, and cross correlating the ZC sequence halves
Neither is great as both are spread pretty far apart.  Nothing like the 16 sample IEEE 802.11 preambles that are amazing for coarse CFO.  Perhaps the second ZC sequence can be used?  In the time domain it looks like it repeats itself over and over rapidly.

Update: I've skipped this step as my primary goal is to get to bits.  For some reason my logic that uses the cyclic prefix is off by a factor of 9.  For now I have done the correction by hand.

Update (15 April 2022): This is complete.  Currently using the cyclic prefix from the first data symbol to get a coarse estimate

## Fine CFO Detection/Adjustment
This likely requires the knowledge of the ZC sequence parameters.  Fine CFO needs a long sequence of samples to work well, and they need to either be known ahead of time, or repeated in the transmission.
Update: I've skipped this as well.  My corrections from the Coarse CFO have taken care of any fine offset as well.

## Phase Correction
Another one that needs a known sequence in the burst.  The good news is that all of the data carriers are QPSK, so there's only 4 ways to rotate the constellation.

Update: I've skipped this too.  Handily the burst I am looking at right now didn't need much in the way of phase adjustments

Update (9 April 2022): In very quick and rough experiments it seems like I can use the cross correlation results from the ZC sequence to directly adjust for absolute phase offsets :D  More experimentation is needed to be sure, but it looks hopeful and sorta makes sense.  I wasn't sure if cross correlation of the ZC sequence would give me frequency or phase information.  I think that autocorrelating the ZC sequence will give me the frequency offset, and cross correlating with the generated sequence will give me phase information (also amplitude if I ever get that far).

Update (15 April 2022): This is now being done with an equalizer that uses the first ZC sequence to get channel taps.  I'm likely not doing it right, but it works for the high SNR environment I'm in :)

## Symbol Extraction
This is super simple and just requires being time and frequency aligned with knowledge of the cyclic prefixes

## Descrambling
Rumor is that there is a scrambler in use.  It's not clear to me if the scrambler is before or after FEC, but it will need to be dealt with.  Supposedly it's LTE standard.

Update: the scrambler is definitely before the FEC.  I found a really nice writeup about it at https://www.sharetechnote.com/html/Handbook_LTE_PseudoRandomSequence.html.  The scrambling sequence seems to be made out of two Linear Feedback Shift Registers (LFSRs) combined together.  The intital state of the first LFSR is a constant value, but the second LFSR needs certain parameters that relate to the link.  Unfortunately I don't have those parameters.  The only good news is that is *only* a 31-bit exhaust to brute force.  So ~ 2.5 billion attempts and you're assured success!  In the event that the Turbo decoder parameters are magically known then maybe this won't be so bad.

Update(9 April 2022): I've been told that I should expect that the first OFDM symbol will drop out to all zeros when the correct scrambler is applied.  I'm not sure if that's true just yet.  I tried using the recommended initial value of the second LFSR of 0x12345678 (0b001_0010_0011_0100_0101_0110_0111_1000 since the LFSR is 31 bits long).  Another hint is that I need to collect several frames from different drones.  I'm hoping to find out that the first symbol is a constant.  This should become evident when I can get more frames demodulated.  The issue here is that the current process is very manual.  To solve that issue I am working on a MATLAB/Octave script that will use the newly found ZC sequences to locate the bursts and extract them for me.  There's still the issue of the frequency offsets and absolute phase offsets that will have to be done by hand.  Though I should be able to use the ZC sequence to fix the absolute phase offset.
Also, I think that if collecting multiple bursts shows that the first symbol doesn't change, then I can use a C++ version of the scrambler generator to brute force finding of the correct initial value (assuming that I don't need to also guess the first intial value...)  It'll take for facking ever, but it's possible.

Update(10 April 2022): The initial value I was told turned out to be 100% correct, and I was having an issue with matrix dimensions.  After changing the code to only look at the first OFDM symbol in the descrambler, everything works out!  Huge thanks to those that helped me through the process!

## Turbo Product Code Removal
It is assumed that the demodulated data contains an LTE standard TPC.  There are libraries that can handle this (hopefully)

A new wrinkle here is that under the Turbo code is going to be "Rate Matching" bits.  I have no idea if that's going to be a standard process that doesn't vary depending on link parameters that aren't already known.

Update (15 April 2022): This is complete.  Code will be uploaded at a later date.

## Deframing
The underlying data should be in a known DJI format.  If so then this step should be pretty simple

Update (15 April 2022): This is complete.  Code will be uploaded at a later date.

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

## Open Source LTE Libs
I have had zero luck with my few attempts at getting open source LTE tools to understand what the DJI Mini 2 is sending out.  My hunch is that this is due to DJI not following the standard to the letter.  But, I know exceedingly little about LTE, so take that for what it's worth.

# Current Questions
## Descrambler
- ~~Is there some special sequence that should appear if descrambling is successful?
- ~~Is there any real point to trying to get the first frame to drop out to all ones or zeros?
- ~~Is the first LFSR seeded with the LTE standard value?
- ~~Are there any known bits for the second LFSR that could reduce the search space?

## Turbo Decoder
- ~~Are there any special parameters needed, or do I just use something like https://github.com/ttsou/turbofec and feed it the raw data from each symbol without any deviation from the LTE spec?

## Rate Matching
- ~~I have zero clue how this works, so *any* advice is welcome (I have been told to look at a specific IEEE paper, but don't have an account)
