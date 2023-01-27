# DJI DroneID RF Demodulator (Non-WiFi)

*This repository is a work in progress!!!*

## Thanks

The credit for this code is not mine alone.  I have had several people help me through this process.  Without their hints about certain parts of the signal I would have hit some very serious roadblocks.  So, thank you very much to those that have assisted me in this work!!!

To that point, huge thanks to the following people (just the folks that are okay with public thanks)
- Chair for System Security @ Ruhr-Universität Bochum / CISPA Helmholtz Center for Information Security

## Drone Compatibility

This repo has only really been tested against my DJI Mini 2.  It's known that there are at least two flavors of DroneID and mine appears to be in the minority of types.  That being said, the code here is written to support both types that I know of.

It's very important to remember that DroneID is present on (AFAIK) all modern DJI drones, but this repo is **ONLY FOR NON-WIFI DRONES**  If you are after a WiFi DroneID detector/demodualtor, check out [Kismet](https://www.kismetwireless.net/development/droneid/).

## Required Software

### MATLAB
The most up to date code in this repo will always be the MATLAB scripts.  This is the code that is used to generate and test the GNU Radio modules.  To prevent people needing to spend ~ $100/yr on a MATLAB license, the MATLAB scripts all *should* work with Octave (at least 5.2.0).  It should be noted that Octave will be a **LOT** slower than MATLAB.  In order to do this there have been some functions that MATLAB natively supports that have had to be written by hand to support both tools.

If you have MATLAB, please use at least R2022a.

### GNU Radio
There are GNU Radio modules, but at the time of writing (Oct 31, 2022) they are not very good.  There is a re-write effort going on that aims to have all of the same features as the MATLAB.  I would not expect that update to be complete for another 2-3 months as it's a lot of work to test and make performant.

You might notice that there several GNU Radio branches.  My apologies as there have been multiple iterations of the GNU Radio module and I did a bad job of orgnaizing everything :(  Here's the breakdown:
- `gr-droneid`: Outdated, probably best to not use
- `gr-droneid-3.8`: Probably best to not use
- `gr-droneid-3.10`: Probably best to not use
- `gr-droneid-update`: Use this if you have GNU Radio 3.8
- `gr-droneid-update-3.10`: Use this if you have GNU Radio 3.10
- `gr-droneid-rewrite`: This is the ongoing re-write

### LTE Rate Matching
There are two libraries required to build the applications that remove/add the LTE rate matching:
- [turbofec](https://github.com/ttsou/turbofec)
- [CRCpp](https://github.com/d-bahr/CRCpp)

`turbofec` must be fully built and installed.  `CRCpp` doesn't need to be installed, you just need to the `CRC.h` header file.

At the time of writing (31 Oct 2022) only Linux is supported for the LTE rate matching logic.  [Cygwin](https://www.cygwin.com/) might work, but no promises.

See the files in [cpp](cpp/) for instructions on how to build

## Wiki
*WORK IN PROGRESS*
Check out the [Wiki](../../wiki/) for more details about how to use this repo.  
