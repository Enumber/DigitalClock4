# $$PWD is this .pri file's own directory. The old check used a path relative
# to the *including* file, which never resolved in an out-of-source build — so
# HAVE_SINGLEAPPLICATION was silently never defined and the "only one instance"
# setting did nothing at all.
exists($$PWD/SingleApplication/singleapplication.pri) {
  include($$PWD/SingleApplication/singleapplication.pri)
  DEFINES += HAVE_SINGLEAPPLICATION
  DEFINES += QAPPLICATION_CLASS=QApplication
}
