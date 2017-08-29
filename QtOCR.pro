#-------------------------------------------------
#
# Project created by QtCreator 2015-05-13T20:56:57
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = QtOCR
CONFIG   += console qt
CONFIG   -= app_bundle

TEMPLATE = app
DESTDIR = bin

HEADERS +=\
           OCREngine/ExtractBlack.h\
           OCREngine/OCREngine.h\
           OCREngine/Binarization/Binarization.h\
           OCREngine/FindString/FindString.h\
           OCREngine/FindString/RemoveNoise.h \
           OCREngine/FindString/PosterEdge.h \
           OCREngine/FindString/Array.h \
           OCREngine/InclCorr/InclCorr.h\
           OCREngine/Segmentation/NoiseReduction.h\
           OCREngine/Segmentation/Segmentation.h\
           OCREngine/OCR/OCR.h\
           OCREngine/global.h \
           SocketDefine.h \
           QtService/qtunixsocket.h \
           QtService/qtunixserversocket.h \
           QtService/qtservice_p.h \
           QtService/qtservice.h \
           CVAThread.h

SOURCES += main.cpp\
           OCREngine/ExtractBlack.cpp\
           OCREngine/OCREngine.cpp\
           OCREngine/Binarization/Binarization.cpp\
           OCREngine/FindString/FindString.cpp\
           OCREngine/FindString/RemoveNoise.cpp \
           OCREngine/FindString/PosterEdge.cpp \
           OCREngine/FindString/Array.cpp \
           OCREngine/InclCorr/InclCorr.cpp\
           OCREngine/Segmentation/NoiseReduction.cpp\
           OCREngine/Segmentation/Segmentation.cpp\
           OCREngine/OCR/OCR.cpp \
           QtService/qtunixsocket.cpp \
           QtService/qtunixserversocket.cpp \
           QtService/qtservice_unix.cpp \
           QtService/qtservice.cpp \
           CVAthread.cpp

INCLUDEPATH += ./OpenCV/include \
INCLUDEPATH += ./QtService \
INCLUDEPATH += ./OCREngine\
INCLUDEPATH += ./OCREngine/Binarization\
INCLUDEPATH += ./OCREngine/FindString\
INCLUDEPATH += ./OCREngine/InclCorr\
INCLUDEPATH += ./OCREngine/OCR\
INCLUDEPATH += ./OCREngine/Segmentation\
#INCLUDEPATH += /opt/Qt5.0.1/5.0.1/gcc_64/include/QtCore/5.0.1/QtCore/ \
#INCLUDEPATH += /opt/Qt5.0.1/5.0.1/gcc_64/include/QtNetwork/5.0.1/QtNetwork/ \

LIBS += -L"/usr/lib/ocr/" \
        -lopencv_highgui \
        -lopencv_imgproc \
        -lopencv_core
