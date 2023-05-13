# Building for Windows

## Installing and Configuration Cygwin

1. Download Cygwin from https://www.cygwin.com/setup-x86_64.exe
2. Execute downloaded install executable
3. Select Yes when prompted by User Account Control
4. Click `Next`
5. Select `Install from Internet` and click `Next`
6. Leave `Root Install Directory` alone and click `Next`
7. Leave `Select Local Package Directory` alone and click `Next`
8. Leave `Select Your Internet Connection` alone and click `Next`
9. Select a mirror and click `Next`
10. From the `View` dropdown select `Full`
11. Enter `gcc-g++` in the `Search` box and select `11.3.0-1` in the `New` column for the `gcc-g++` row
13. Enter `autoconf` in the `Search` box and select `15-1` in the `New` column for the `autoconf` row
14. Enter `automake` in the `Search` box and select `11-1` in the `New` column for the `automake` row
15. Enter `make` in the `Search` box and select `4.4.1-2` in the `New` column for the `make` row
16. Enter `git` in the `Search` box and select `2.39.0-1` in the `New` column for the `git` row
17. Enter `wget` in the `Search` box and select `1.21.3-1` in the `New` column for the `wget` row
18. Click `Next` at the bottom right of the window
19. Click `Next` at the `Review and confirm changes` window
20. Click `Finish` to close the window

## Updating Windows PATH

1. Click on the Start menu
2. Type `PATH`
3. Select `Edit the system environment variables`
4. A new window called `System Properties` should come up
5. Click `Environment Variables` button
6. In the top panel click the `Path` button
7. In the new window click the `Edit` button
8. In the new window click the `New` button
9. Type in `C:\cygwin64\bin`
10. Click `OK` to close the edit window
11. Click `OK` to close the `Environment Variables` window
12. Click `OK` to close the `System Properties` window
13. At this point you might have to reboot the computer, but sometimes you don't have to

## Building the Turbo decode/encode executables

1. Open the Cygwin terminal (should be on your desktop, search the start menu if not)
2. Navigate to where you have extracted/cloned the `dji_droneid` repo in Windows
   If you have the code at `C:\Users\<user_name>\Downloads\dji_droneid` then you would run: `cd /cygdrive/c/Users/<user_name>/Downloads/dji_droneid` in the Cygwin terminal
3. Run the following set of commands in the Cygwin terminal
```
pushd cpp
wget https://raw.githubusercontent.com/d-bahr/CRCpp/master/inc/CRC.h -O CRC.h
mkdir -p deps; pushd deps
git clone https://github.com/ttsou/turbofec; pushd turbofec
autoreconf -i && ./configure && make && make install
popd && popd
g++ -Wall add_turbo.cc -o add_turbo -I. -I/usr/local/include -L/usr/local/lib -lturbofec
g++ -Wall remove_turbo.cc -o remove_turbo -I. -I/usr/local/include -L/usr/local/lib -lturbofec
```

You should now have two new executables: `add_turbo` and `remove_turbo`

These executables should be usable from Windows
