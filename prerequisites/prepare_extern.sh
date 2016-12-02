#!/usr/bin/env bash

printUsage()
{
	echo
	echo "Usage:"
	echo
	echo "./prepare_extern.sh [--all] [--qt] [--force-download] [--debug] [-h|--help]"
	echo
	echo "Examples:"
	echo
	echo "Build study_cpp dependencies:"
	echo "$ ./prepare_extern.sh"
	echo
	echo "Build all supported dependencies:"
	echo "$ ./prepare_extern.sh --all"
	echo
	echo "Build only qt:"
	echo "$ ./prepare_extern.sh --qt"
	echo
	echo "Build only qt with debug simbols and force download the archive:"
	echo "$ ./prepare_extern.sh --qt --force-download -d"
	echo
	echo
}

QT_ADDR="http://download.qt.io/official_releases/qt/5.7/5.7.0/single/qt-everywhere-opensource-src-5.7.0.tar.gz"

SYSTEM=

downloadArchive()
{
	local url="$1"
	local arc="$(basename "${url}")"
	echo "Downloading: [$arc] from [$url]"
	#wget --no-check-certificate -O $arc $url
	#wget -O $arc $url
	curl -L -O $url
}

extractTarBz2()
{
#    bzip2 -dc "$1" | tar -xf -
	tar -xjf "$1"
}

extractTarGz()
{
#    gzip -dc "$1" | tar -xf -
	tar -xzf "$1"
}



buildQt()
{
	WHAT="qt"
	ADDR_NAME=$QT_ADDR
	echo
	echo "Building $WHAT..."
	echo

	OLD_DIR=`ls . | grep "$WHAT" | grep -v "tar"`
	echo
	echo "Cleanup previous builds..."
	echo

	rm -rf $OLD_DIR
	rm -rf include/Qt*
	rm -rf lib/libQt*
	
	echo
	echo "Prepare the $WHAT archive..."
	echo

	ARCH_NAME=`find . -name "$WHAT-*.tar.gz" | grep -v "old/"`
	if [ -z "$ARCH_NAME" -o -n "$DOWNLOAD" ] ; then
		mkdir old
		mv $ARCH_NAME old/
		echo "No $WHAT archive found or forced - try download: $ADDR_NAME"
		downloadArchive $ADDR_NAME
		ARCH_NAME=`find . -name "$WHAT-*.tar.gz" | grep -v "old/"`
	fi
	
	echo "Extracting $WHAT [$ARCH_NAME]..."
	extractTarGz $ARCH_NAME

	DIR_NAME=`ls . | grep "$WHAT" | grep -v "tar"`
	echo
	echo "Making $WHAT [$DIR_NAME]..."
	echo

	cd $DIR_NAME
	export CXXFLAGS="-I$EXT_DIR/include -L$EXT_DIR/lib"
	#./configure  -prefix "${EXT_DIR}" -opensource -confirm-license -verbose -static -release -qt-libpng -qt-xcb -nomake examples -no-compile-examples -c++std c++11 -opensource -confirm-license 
	./configure -prefix "${EXT_DIR}" -opensource -confirm-license -verbose -release -qt-libpng -no-openssl -no-sql-db2 -no-sql-ibase -no-sql-mysql -no-sql-oci -no-sql-odbc -no-sql-psql -no-sql-tds -no-opengl -no-cups -no-libudev -no-libproxy -no-pulseaudio -no-alsa -nomake examples -no-compile-examples  -skip qtandroidextras -skip qtconnectivity -skip qtlocation -skip qtserialport -skip qtwebengine -skip qtwinextras -skip qtgraphicaleffects -skip qtquickcontrols -qt-xcb -skip qtsensors -skip qtsvg -skip qtwayland -skip qtactiveqt -skip qtcanvas3d -skip qtmultimedia -skip qtquickcontrols2 -skip qtserialbus -skip qtwebview -skip qtxmlpatterns -c++std c++11 -opensource -confirm-license
	
	#./configure -prefix "${EXT_DIR}" -opensource -confirm-license -verbose -static -release -nomake examples -no-compile-examples -qt-xcb -c++std c++11 -skip qtquickcontrols -skip qtquickcontrols2
	
	#make -j$(cpu_count) && make install
	make && make install
	cd ..
	
	echo
	echo "Done $WHAT!"
	echo
}



EXT_DIR="`pwd`"

BUILD_QT=

BUILD_SOMETHING=

ARCHIVE=
DOWNLOAD=
DEBUG=
HELP=


while [ "$#" -gt 0 ]; do
	CURRENT_OPT="$1"
	UNKNOWN_ARG=no
	HELP=
	case "$1" in
	--all)
		BUILD_QT="yes"
		;;
	--qt)
		BUILD_QT="yes"
		BUILD_SOMETHING="yes"
		;;
	--debug)
		DEBUG="yes"
		;;
	--force-download)
		DOWNLOAD="yes"
		;;
	--system)
		shift
		SYSTEM="$1"
		;;
	-h|--help)
		HELP="yes"
		BUILD_SOMETHING="yes"
		;;
	*)
		HELP="yes"
		;;
	esac
	shift
done

#echo "Debug build = $DEBUG"
#echo "Force download = $DOWNLOAD"

if [ "$HELP" = "yes" ]; then
	printUsage
	exit
fi

if [[ -z "${SYSTEM}" ]]; then
	SYSTEM=$(uname)
fi


echo "Extern folder: $EXT_DIR"
echo "System: $SYSTEM"

if [[ -z "${BUILD_SOMETHING}" ]]; then
	BUILD_QT="yes"
fi

if [ $BUILD_QT ]; then
	buildQt
fi


if [ -d lib64 ]; then
	cd lib

	for filename in ../lib64/*
	do
	echo "SimLink to $filename"
	ln -s $filename .
	done;
fi

if [ $ARCHIVE ]; then
	cd $EXT_DIR
	cd ../
	CWD="`basename $EXT_DIR`"
	echo $CWD

	echo "tar -cjf $CWD.tar.bz2 $CWD/include $CWD/lib $CWD/lib64 $CWD/bin $CWD/sbin $CWD/share"
	tar -cjf $CWD.tar.bz2 $CWD/include $CWD/lib $CWD/bin $CWD/sbin $CWD/share
fi

echo
echo "DONE!"
