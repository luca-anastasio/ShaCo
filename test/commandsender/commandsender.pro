# Check the config files exist
!include(../test.pri) {
    error("Couldn't find the test.pri file!")
}

TARGET = commandsender_test

SOURCES += \
        commandsender_test.cpp
