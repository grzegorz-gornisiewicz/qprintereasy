TARGET = QPrinterEasy
TEMPLATE = lib
DEFINES += QPRINTEREASY_LIBRARY

# include configuration file
include( src/config.pri )

# include files to compile
include( src/src.pri )