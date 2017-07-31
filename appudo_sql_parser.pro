###########################################################################################
#    appudo_sql_parser.pro is part of Appudo
#
#    Copyright (C) 2015
#       543699f52901235482e5b2c38ffc606366c05ce2c371043aecdbeff00215914a source@appudo.com
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
###########################################################################################


TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

MACHINE = $$system(uname -m)
CONFIG(release, debug|release) : DESTDIR = $$_PRO_FILE_PWD_/Release.$$MACHINE
CONFIG(debug, debug|release)   : DESTDIR = $$_PRO_FILE_PWD_/Debug.$$MACHINE
CONFIG(force_debug_info)       : DESTDIR = $$_PRO_FILE_PWD_/Profile.$$MACHINE

CONFIG(release, debug|release) : LVL = 0
CONFIG(debug, debug|release)   : LVL = 1
CONFIG(force_debug_info)       : LVL = 1

QMAKE_PRE_LINK =
QMAKE_POST_LINK = $$_PRO_FILE_PWD_/strip.sh $$DESTDIR/$$TARGET $$LVL
QMAKE_RPATHDIR =
QMAKE_RPATH =

INCLUDEPATH += /usr/include/c++/v1 \
               /usr/include \
               src \
               src/parser \
               src/include \
               src/include/parser \

QMAKE_CXXFLAGS += -Xclang -dwarf-column-info -c -Wno-conversion-null -fno-rtti -fmessage-length=0 -fvisibility-inlines-hidden -fvisibility=hidden -std=c++11 -stdlib=libc++ -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
QMAKE_CXXFLAGS += -fPIC

LIBS += -lc++

QMAKE_LFLAGS += -shared -Wl,--version-script=$$_PRO_FILE_PWD_/EXPORT
# -pie
QMAKE_LFLAGS -= -ccc-gcc-name g++
QMAKE_LFLAGS += -Wl,-z,noexecstack

QMAKE_CXXFLAGS_DEBUG -= -g -g0 -g1 -g2 -O -O1 -O2 -O3
QMAKE_CXXFLAGS_RELEASE -= -g -g0 -g1 -g2 -O -O1 -O2 -O3
QMAKE_CFLAGS_DEBUG -= -g -g0 -g1 -g2 -O -O1 -O2 -O3
QMAKE_CFLAGS_RELEASE -= -g -g0 -g1 -g2 -O -O1 -O2 -O3

QMAKE_CXXFLAGS_DEBUG *= -g3 -O0
QMAKE_CFLAGS_DEBUG *= -g3 -O0

QMAKE_CXXFLAGS_RELEASE *= -g0 -O3 -DNDEBUG
QMAKE_CFLAGS_RELEASE *= -g0 -O3 -DNDEBUG

QMAKE_MAKEFILE = $$DESTDIR/Makefile
OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui



DISTFILES += \
    EXPORT \
    strip.sh \
    COPYING \
    pgpool2-3_6_STABLE.zip

HEADERS += \
    src/parser/gram.h \
    src/parser/gram.y \
    src/parser/scan.l \
    src/include/parser/explain.h \
    src/include/parser/extensible.h \
    src/include/parser/gramparse.h \
    src/include/parser/keywords.h \
    src/include/parser/kwlist.h \
    src/include/parser/lockoptions.h \
    src/include/parser/makefuncs.h \
    src/include/parser/nodes.h \
    src/include/parser/parsenodes.h \
    src/include/parser/parser.h \
    src/include/parser/pg_class.h \
    src/include/parser/pg_config_manual.h \
    src/include/parser/pg_list.h \
    src/include/parser/pg_trigger.h \
    src/include/parser/pg_wchar.h \
    src/include/parser/pool_parser.h \
    src/include/parser/pool_string.h \
    src/include/parser/primnodes.h \
    src/include/parser/scanner.h \
    src/include/parser/scansup.h \
    src/include/parser/stringinfo.h \
    src/include/parser/value.h \
    src/include/pcp/libpcp_ext.h \
    src/include/utils/elog.h \
    src/include/utils/palloc.h \
    src/include/utils/pool_signal.h \
    src/include/config.h \
    src/include/pool.h \
    src/include/pool_type.h \
    src/include/utils/memdebug.h \
    src/include/utils/memnodes.h \
    src/include/utils/memutils.h

SOURCES += \
    src/parser/copyfuncs.c \
    src/parser/gram.c \
    src/parser/keywords.c \
    src/parser/kwlookup.c \
    src/parser/list.c \
    src/parser/makefuncs.c \
    src/parser/nodes.c \
    src/parser/outfuncs.c \
    src/parser/parser.c \
    src/parser/pool_string.c \
    src/parser/scan.c \
    src/parser/scansup.c \
    src/parser/snprintf.c \
    src/parser/stringinfo.c \
    src/parser/value.c \
    src/parser/wchar.c \
    src/utils/error/elog.c \
    src/utils/mmgr/mcxt.c \
    src/utils/mmgr/aset.c

