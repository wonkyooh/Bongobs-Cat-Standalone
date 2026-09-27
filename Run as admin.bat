@echo off
powershell -NoProfile -Command "Start-Process -FilePath '%~dp0BongobsCat.exe' -Verb RunAs"
