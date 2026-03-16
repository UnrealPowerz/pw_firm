@echo off
set PATH=Z:\opt\H8\6_2_2\bin;Z:\opt\H8\6_1_3\bin;%PATH%
set CH38=Z:\opt\H8\6_2_2\include
cd Z:\opt\H8\6_2_2\bin
lbg38.exe -output=Z:\work\build\lib3hn.lib -head=Runtime -cpu=300HN
