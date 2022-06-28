TEMPLATE = app

QT += core gui xml network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# OPTIONS

defined(GMIC_PATH, var) { message("GMIC_PATH is set ("$$GMIC_PATH")") }

!defined(GMIC_DYNAMIC_LINKING,var) { GMIC_DYNAMIC_LINKING = off }

defined(GMIC_LIB_PATH, var) { message("GMIC_LIB_PATH is set ("$$GMIC_LIB_PATH")") }

!defined(PREFIX, var) { PREFIX = /usr/bin }

# Sample qmake command to build with dynamic linking to libgmic.so
#    $qmake GMIC_DYNAMIC_LINKING=on GMIC_PATH=/usr/include GMIC_LIB_PATH=/usr/lib/x86_64-linux-gnu/
#    $make

#
#
#

CONFIG	+= qt

greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++11
!greaterThan(QT_MAJOR_VERSION, 4): QMAKE_CXXFLAGS += --std=c++11

CONFIG	+= warn_on
QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += fftw3 zlib

#
# OpenCV
#
packagesExist(opencv4) { # e.g. Ubuntu 20.04
   PKGCONFIG += opencv4
}
packagesExist(opencv) {
   PKGCONFIG += opencv
}
!packagesExist(opencv):!packagesExist(opencv4) {
  error(Cannot find OpenCV)
}

DEFINES += cimg_use_fftw3 cimg_use_zlib
DEFINES += QT_DEPRECATED_WARNINGS

!defined(GMIC_PATH, var):exists(../src/gmic.cpp) {
 message(GMIC_PATH was not set: Found gmic sources in ../src)
 GMIC_PATH = ../src
}
!defined(GMIC_PATH, var):exists(../gmic/src/gmic.cpp) {
 message(GMIC_PATH was not set: Found gmic sources in ../gmic/src)
 GMIC_PATH = ../gmic/src
}
!defined(GMIC_PATH, var):exists(./gmic/src/gmic.cpp) {
 message(GMIC_PATH was not set: Found gmic sources in ./gmic/src)
 GMIC_PATH = ./gmic/src
}
equals(GMIC_DYNAMIC_LINKING, "off" ){
 defined(GMIC_PATH, var):!exists( $$GMIC_PATH/gmic.cpp ) {
  error("G'MIC repository was not found ("$$GMIC_PATH")")
 }
 message("G'MIC repository was found ("$$GMIC_PATH")")
}
equals(GMIC_DYNAMIC_LINKING, "on" ){
 defined(GMIC_PATH, var):!exists( $$GMIC_PATH/gmic.h ) {
  error("G'MIC header was not found ("$$GMIC_PATH"/gmic.h)")
 }
 message("G'MIC header was found ("$$GMIC_PATH"/gmic.h)")
}

!defined(GMIC_LIB_PATH, var) {
 GMIC_LIB_PATH = $$GMIC_PATH
}

unix {
 VERSION = $$system(grep \"define.ZART_VERSION \" include/Common.h | sed -e \"s/.*VERSION //\")
 zart_program.path = $$PREFIX
 zart_program.files = ./zart
 INSTALLS += zart_program
}

isEmpty( VERSION ):{
 VERSION = 0.0.0
 message( Warning: VERSION was not found in include/Common.h. Set to $$VERSION )
}

# enable OpenMP by default on with g++, except on OS X
!macx:*g++* {
 CONFIG += openmp
}

!win32 {
 LIBS += -lfftw3_threads
}

# use qmake CONFIG+=openmp ... to force using openmp
# For example, on OS X with GCC 4.8 installed:
# qmake -spec unsupported/macx-clang QMAKE_CXX=g++-4.8 QMAKE_LINK=g++-4.8 CONFIG+=openmp
# Notes:
#  - the compiler name is g++-4.8 on Homebrew and g++-mp-4.8 on MacPorts
#  - we use the unsupported/macx-clang config because macx-g++ uses arch flags that are not recognized by GNU GCC
openmp {
 DEFINES += cimg_use_openmp
 QMAKE_CXXFLAGS += -fopenmp
 QMAKE_LFLAGS += -fopenmp
}

equals(GMIC_DYNAMIC_LINKING, "on" ) {
 message(Dynamic linking with libgmic)
 LIBS += -L$$GMIC_LIB_PATH -lgmic
}

equals(GMIC_DYNAMIC_LINKING, "off" ) {
 HEADERS += $$GMIC_PATH/gmic_stdlib.h
 SOURCES += $$GMIC_PATH/gmic.cpp
 DEFINES += gmic_core
}

#
# Check for V4L2
#
linux:exists(/usr/include/linux/videodev2.h) {
 message("v4l2 header file videodev2.h is available")
 DEFINES += HAS_V4L2
}

linux:!exists(/usr/include/linux/videodev2.h) {
 warning(v4l2 header file videodev2.h not found. It is HIGHLY recommended.)
}

DEFINES += gmic_is_parallel cimg_use_abort

INCLUDEPATH += $$PWD $$PWD/include

!equals(GMIC_PATH,/usr/include):!equals(GMIC_PATH,/include) {
 INCLUDEPATH += $$GMIC_PATH
}

DEPENDPATH += $$PWD/include

HEADERS	+= $$GMIC_PATH/gmic.h \
    $$GMIC_PATH/CImg.h \
    include/ImageView.h \
    include/MainWindow.h \
    include/FilterThread.h \
    include/CommandEditor.h \
    include/DialogAbout.h \
    include/ImageConverter.h \
    include/DialogLicense.h \
    include/ImageSource.h \
    include/WebcamSource.h \
    include/StillImageSource.h \
    include/VideoFileSource.h \
    include/Common.h \
    include/TreeWidgetPresetItem.h \
    include/AbstractParameter.h \
    include/IntParameter.h \
    include/CommandParamsWidget.h \
    include/SeparatorParameter.h \
    include/NoteParameter.h \
    include/FloatParameter.h \
    include/BoolParameter.h \
    include/ChoiceParameter.h \
    include/ColorParameter.h \
    include/CriticalRef.h \
    include/FullScreenWidget.h \
    include/FileParameter.h \
    include/FolderParameter.h \
    include/TextParameter.h \
    include/LinkParameter.h \
    include/ConstParameter.h \
    include/PointParameter.h \
    include/KeypointList.h\
    include/OverrideCursor.h\
    include/OutputWindow.h

SOURCES	+= \
    src/ImageView.cpp \
    src/MainWindow.cpp \
    src/ZArt.cpp \
    src/FilterThread.cpp \
    src/DialogAbout.cpp \
    src/CommandEditor.cpp \
    src/ImageConverter.cpp \
    src/DialogLicense.cpp \
    src/ImageSource.cpp \
    src/WebcamSource.cpp \
    src/StillImageSource.cpp \
    src/VideoFileSource.cpp \
    src/TreeWidgetPresetItem.cpp \
    src/AbstractParameter.cpp \
    src/IntParameter.cpp \
    src/CommandParamsWidget.cpp \
    src/SeparatorParameter.cpp \
    src/NoteParameter.cpp \
    src/FloatParameter.cpp \
    src/BoolParameter.cpp \
    src/ChoiceParameter.cpp \
    src/ColorParameter.cpp \
    src/FullScreenWidget.cpp \
    src/FileParameter.cpp \
    src/FolderParameter.cpp \
    src/TextParameter.cpp \
    src/LinkParameter.cpp \
    src/PointParameter.cpp \
    src/ConstParameter.cpp \
    src/KeypointList.cpp \
    src/OverrideCursor.cpp \
    src/OutputWindow.cpp

RESOURCES = zart.qrc
DEPENDPATH += $$PWD/images

FORMS = ui/MainWindow.ui \
    ui/DialogAbout.ui \
    ui/DialogLicense.ui \
    ui/FullScreenWidget.ui \
    ui/OutputWindow.ui

PRE_TARGETDEPS +=

CONFIG(release, debug|release) {
 message(Release build)
 DEFINES += QT_NO_DEBUG_OUTPUT
}

CONFIG(debug, debug|release) {
 message(Debug build)
 DEFINES += _ZART_DEBUG_
 QMAKE_CXXFLAGS_DEBUG += -fsanitize=address -Dcimg_verbosity=3
 QMAKE_LFLAGS_DEBUG +=  -fsanitize=address
}

UI_DIR = .ui
MOC_DIR = .moc
RCC_DIR = .qrc
OBJECTS_DIR = .obj

unix:!macx { DEFINES += _IS_UNIX_ }

freebsd {
 DEFINES += _IS_FREEBSD_
 message(Detected OS is PreeBSD)
}

macx {  DEFINES += _IS_MACOS_ }

DEFINES += cimg_display=0
