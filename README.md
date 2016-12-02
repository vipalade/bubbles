# bubbles: sample client-server using SolidFrame

This is a sample project of how to use SolidFrame mpipc library in a secured (SSL) client-server scenario.
While the server is Linux only, there are two sample clients:
 * one for Linux (should work on FreeBSD and macOS with minimum effort), using Qt
 * another one for Android

[bubbles/client/android](bubbles/client/android) is an example of how to use SolidFrame framework and especially the solid::frame::mpipc library allong with BoringSSL in an Android application.

Getting started with the server:
```bash
$ mkdir ~/work
$ cd ~/work
$ git clone git@github.com:vipalade/solidframe.git
$ mkdir extern
$ cd extern
$ ../solidframe/prerequisites/prepare_extern.sh
# ... wait until the prerequisites are built
$ cd ../solidframe
$ ./configure -e ~/work/extern --prefix ~/work/extern
$ cd build/release
$ make install
# clone bubbles
$ cd ~/work
$ git clone https://github.com/vipalade/bubbles.git
$ cd ~/work/bubbles/
$ mkdir -p build/debug
$ cd build/debug
$ cmake -DEXTERN_PATH=~/work/extern ../../
$ cd server/main
$ make
# run the server:
$ ./bubbles_server -p 4444 -s
```

Getting started with Qt client:
```bash
$ mkdir ~/work
$ cd ~/work
$ git clone git@github.com:vipalade/solidframe.git
$ mkdir extern
$ cd extern
$ ../solidframe/prerequisites/prepare_extern.sh
# ... wait until the prerequisites are built
$ cd ../solidframe
$ ./configure -e ~/work/extern --prefix ~/work/extern
$ cd build/release
$ make install
# clone bubbles
$ cd ~/work
$ git clone https://github.com/vipalade/bubbles.git
$ cd extern
$ ../bubbles/prerequisites/prepare_extern.sh --qt
# wait while Qt gets compiled - will take a while - be carefull to install develop packages needed by Qt
$ cd ~/work/bubbles/
$ mkdir -p build/debug
$ cd build/debug
$ cmake -DEXTERN_PATH=~/work/extern ../../
$ cd client/qt
$ make
# run the client:
$ ./bubbles_client --help
```

Getting started Android client:

```bash
$ git clone --recursive https://github.com/vipalade/bubbles.git
```

 * load bubbles/client/android project in Android Studio >2.2
 * run the application from there on either emulator or a phisical device.

