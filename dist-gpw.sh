#!/bin/sh

# Create a archive 'gpw32-*.zip' (gpwYMMDD.zip) with Win32 (MinGW) build
# of the Golded+.


name=../gpw32-115-`date +%Y%m%d`.zip

files="bin/gedcyg.exe bin/gncyg.exe bin/rddtcyg.exe"
files="${files} docs/copying docs/copying.lib"
files="${files} docs/golded.html docs/golded.txt docs/goldnode.html"
files="${files} docs/goldnode.txt docs/license.txt docs/notework.txt"
files="${files} docs/rddt.html docs/rddt.txt docs/readme.txt"
files="${files} docs/rusfaq.txt docs/tips.txt docs/todowork.txt"
files="${files} docs/tokencfg.txt docs/tokentpl.txt"

printf 'GoldED+1.1.5  [Win32 binaries]\r\n'  >bin/File_ID.Diz
printf '[Compiled using  MinGW/Cygwin]\r\n' >>bin/File_ID.Diz
printf 'Snapshot (development version)\r\n' >>bin/File_ID.Diz
printf 'This is  unstable release  and\r\n' >>bin/File_ID.Diz
printf 'it should be used for testing.\r\n' >>bin/File_ID.Diz
printf '------------------------------\r\n' >>bin/File_ID.Diz
printf 'GoldED+ is a  successor of the\r\n' >>bin/File_ID.Diz
printf 'wellknown  GoldED mail editor.\r\n' >>bin/File_ID.Diz
printf '------------------------------\r\n' >>bin/File_ID.Diz
printf '*golded-plus.sourceforge.net* \r\n' >>bin/File_ID.Diz

make
make strip
make docs
zip -9DXj ${name} bin/File_ID.Diz $files