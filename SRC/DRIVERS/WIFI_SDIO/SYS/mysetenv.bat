@echo off

echo Building Project...
set DEVIF=
set CHIPID=
set RF=
set HOSTTYPE=

FOR /F "eol=; tokens=1,2* " %%i in (.config) do (
	rem @echo "%%i|%%j"
	IF "%%i"=="DEVINTRF:" (
		SET DEVIF=%%j
	)
	IF "%%i"=="CHIPSID:" (
		SET CHIPID=%%j
	)
	IF "%%i"=="RF:" (
		SET RF=%%j
	)
	IF "%%i"=="HOST:" (
		SET HOSTTYPE=%%j
	)
	IF "%%i"=="MODE:" (
		SET 80211MODE=%%j
	)
)
