# bubbles: sample client-server using SolidFrame

Exemplifies how to use SolidFrame mpipc library in a secured (SSL) client-server scenario.

[client/android](client/android) is an example of how to use SolidFrame framework and especially the solid::frame::mpipc library allong with BoringSSL in an Android application.

## Insight

__bubbles__ is a client-server application. It consists of:
 * a server running on Linux
 * a client running on
   * Linux (macOS and FreeBSD should also work - not tested) using Qt for GUI
   * Android using Java Native Interface

### The client

The client displays multiple moving bubbles:
 * one bigger bubble - the personal one which can be moved by the user (via mouse or touch) and/or automatically by a bubble auto pilot.
 * other smaller bubbles, one for every other client registered on the same room on the server.

So, every client will display in real time all the bubbles in a room at the position in the client they originate.

#### Implementation
The frontend is implemented using either Qt (for desktop clients) or Java for Android application.
The backend engine is implemented in C++ and relies on solid_frame libraries (especially on solid_frame_mpipc for communication) and on OpenSSL (on desktop)/BoringSSL (on Android) to secure the communication.


### The server

 * keeps multiple connections grouped by the room they registered onto
 * register clients giving them an unique per room color
 * receives position updates from all the clients and further
 * pushes the updates to all other clients in the room 

#### Implementation

The server is a C++ application using solid_frame libraries (most important solid_frame_mpipc for communication), OpenSSL to secure communication and boost for parsing command line parameters.

### Workflow
 * the client connects to the server and registers on a room using either a given color or requesting a new color
 * the server will respond with a unique color (which may not be the requested one) and push to the client the positions and colors of all other bubbles in the room
 * the client will start displaying the bubbles
 * the client will send its initial bubble position
 * the client will continue sending the personal bubble position when it changes
 * the server will continue to push other bubbles position changes to the client.


## Getting started

__With the server__:

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
```
or directly use the SolidFrame build - without installing it

```bash
$ mkdir ~/work
$ cd ~/work
$ git clone git@github.com:vipalade/solidframe.git
$ mkdir external
$ cd extern
$ ../solidframe/prerequisites/prepare_extern.sh
# ... wait until the prerequisites are built
$ cd ../solidframe
$ ./configure -e ~/work/external
$ cd build/release
$ make
# clone bubbles
$ cd ~/work
$ git clone https://github.com/vipalade/bubbles.git
$ cd ~/work/bubbles/
$ mkdir -p build/release
$ cd build/release
$ cmake -DCMAKE_BUILD_TYPE=release -DEXTERN_PATH=~/work/external -DSolidFrame_DIR=~/work/solidframe/build/release ../../
$ cd server/main
$ make

```

run the server:

```bash
# run the server:
$ ./bubbles_server -p 4444 -s
```

__With Qt client__:

First you'll need to download precompiled Qt from [here](http://download.qt.io/official_releases/qt/5.9/5.9.2/qt-opensource-linux-x64-5.9.2.run) like this:

```bash
$ cd ~/work/external
$ curl -L -O http://download.qt.io/official_releases/qt/5.9/5.9.2/qt-opensource-linux-x64-5.9.2.run
$ chmod +x qt-opensource-linux-x64-5.9.2.run
$ ./qt-opensource-linux-x64-5.9.2.run
# ... follow the installation steps, and install qt in ~/work/external/qt/
```

Next, you'll have to install Qt dependencies. E.g. on Fedora:

```bash
$ sudo dnf install mesa-libGL-devel
```

next, configure the bubbles build to also compile the Qt client:

```bash
$ cd ~/work/bubbles/
$ cd build/release
# reconfigure with path to QtWidget:
$ cmake -DCMAKE_BUILD_TYPE=release -DEXTERN_PATH=~/work/external -DSolidFrame_DIR=~/work/solidframe/build/release -DQt5Widgets_DIR=~/work/external/qt/5.9.2/gcc_64/lib/cmake/Qt5Widgets ../../
$ cd client/qt
$ make
```

run the client:

```bash
# launch multiple bubbles clients:
$ ./bubbles_client -c localhost:4444 &
$ ./bubbles_client -c localhost:4444 &
$ ./bubbles_client -c localhost:4444 &
$ ./bubbles_client -c localhost:4444 &
```

__With Android client__:

```bash
$ git clone --recursive https://github.com/vipalade/bubbles.git
```

 * load bubbles/client/android project in Android Studio >2.2
 * run the application from there on either emulator or a phisical device.

