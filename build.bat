@echo off

#set opts=-FC -GR- -EHa- -nologo -Zi
#set code=%cd%
#pushd bin
#cl %opts% %code%\src\*.cpp -Fevklib
#popd

pushd %cd%\build
cmake --build .
popd