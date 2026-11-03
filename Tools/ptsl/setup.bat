@echo off
setlocal
set PY=
if exist "%LocalAppData%\Programs\Python\Python314\python.exe" set PY=%LocalAppData%\Programs\Python\Python314\python.exe
if "%PY%"=="" if exist "%LocalAppData%\Programs\Python\Python313\python.exe" set PY=%LocalAppData%\Programs\Python\Python313\python.exe
if "%PY%"=="" set PY=python

echo Using: %PY%
"%PY%" -m pip install --upgrade pip
"%PY%" -m pip install grpcio protobuf
"%PY%" -c "import grpc; print('grpc OK', grpc.__version__)"
echo Done.
endlocal
