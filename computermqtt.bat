@echo off
pushd %~dp0
powershell -WindowStyle Minimized .\monitor.ps1 -processName ComputerMqtt -commandLine .\ComputerMqtt.exe, 192.168.0.20, 1883
popd
pause