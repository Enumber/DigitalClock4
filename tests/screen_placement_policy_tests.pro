# Pure screen-placement policy regression harness (no desktop required).
QT -= gui
QT += core
CONFIG += console c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = screen_placement_policy_tests

SOURCES += \
    screen_placement_policy_tests.cpp \
    ../digital_clock/core/screen_placement_policy.cpp

HEADERS += ../digital_clock/core/screen_placement_policy.h
