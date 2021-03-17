@echo off

del *.pdb
del *.ilk
del *.obj

del /s *.exe

del client\build\x64_debug\*.pdb
del client\build\x64_debug\*.ilk

del server\build\x64_debug\*.pdb
del server\build\x64_debug\*.ilk

del /s server\*.pdb
del /s server\*.ilk
del /s server\*.obj

del /s client\*.pdb
del /s client\*.ilk
del /s client\*.obj

cls