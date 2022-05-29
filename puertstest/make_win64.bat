set ENGINE=%1
if "%ENGINE%"=="" (
  set ENGINE=v8
)

mkdir build64_%ENGINE% & pushd build64_%ENGINE%
cmake -DJS_ENGINE=%ENGINE% -G "Visual Studio 16 2019" -A x64 ..
popd
cmake --build build64_%ENGINE% --config Release
