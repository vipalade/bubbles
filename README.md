# bubbles: sample client-server using SolidFrame

This is a sample project of how to use SolidFrame mpipc library in a secured (SSL) client-server scenario.
While the server is Linux only, there are two sample clients:
 * one for Linux (should work on FreeBSD and macOS with minimum effort), using Qt
 * another one for Android

[bubbles/client/android](bubbles/client/android) is an example of how to use SolidFrame framework and especially the solid::frame::mpipc library allong with BoringSSL in an Android application.

Start using:
 * check out: git clone --recursive https://github.com/vipalade/bubbles.git
 * load bubbles/client/android project in Android Studio
 * run the application from there on either emulator or a phisical device.

