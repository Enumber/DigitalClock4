# See single_application.pri: the relative exists() check never matched in an
# out-of-source build, so HAVE_QHOTKEY was never defined and the global
# hotkeys in the alarm, countdown timer, scheduler and stopwatch plugins were
# compiled out without a word.
exists($$PWD/QHotkey/qhotkey.pri) {
  include($$PWD/QHotkey/qhotkey.pri)
  DEFINES += HAVE_QHOTKEY
}
