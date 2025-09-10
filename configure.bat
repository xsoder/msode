@echo off
setlocal enabledelayedexpansion

:: -----------------------
:: Configuration
set "CURL_PATH=%USERPROFILE%\Downloads\curl-8.15.0_7-win64-mingw\bin\curl.exe"
:: -----------------------

:: LOGGING
set "ERROR=[ERROR]"
set "LOG=[LOG]"
set "SUCCESS=[SUCCESS]"
set "WARNING=[WARNING]"

:: URLS
set "RAYLIB_URL=https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_mingw-w64.zip"
set "TINY_FILE_H=https://raw.githubusercontent.com/native-toolkit/libtinyfiledialogs/refs/heads/master/tinyfiledialogs.h"
set "TINY_FILE_C=https://raw.githubusercontent.com/native-toolkit/libtinyfiledialogs/refs/heads/master/tinyfiledialogs.c"

:: DIRECTORIES
set "DEPS=deps"
set "TINY_DIR=tinyfiledialog"
set "RAYLIB_DIR=raylib-5.5_win64_mingw-w64"

:: Resolve CURL_PATH
if defined CURL_PATH (
  if exist "%CURL_PATH%" (
    set "USE_CURL=1"
    echo %LOG% Using curl at: %CURL_PATH%
  ) else (
    echo %WARNING% Specified CURL_PATH does not exist: %CURL_PATH%
    echo %LOG% Falling back to PowerShell downloads.
    set "USE_CURL=0"
  )
) else (
  set "USE_CURL=0"
)

:: Ensure deps directory exists
if not exist "%DEPS%" (
  echo %LOG% Directory %DEPS% does not exist. Creating...
  mkdir "%DEPS%" 2>nul
  if errorlevel 1 (
    echo %ERROR% Failed to create %DEPS%
    goto :end
  )
) else (
  echo %LOG% Directory %DEPS% exists. Using existing directory.
)

pushd "%DEPS%" || (echo %ERROR% Failed to change directory to %DEPS% & goto :end)

:: STARTING DOWNLOADS
echo %LOG% STARTING DEPENDENCY DOWNLOAD

call :download "%RAYLIB_URL%"
call :download "%TINY_FILE_H%"
call :download "%TINY_FILE_C%"

:: EXTRACT RAYLIB
echo %LOG% Extracting raylib archive using PowerShell...

if exist "raylib-5.5_win64_mingw-w64.zip" (
  echo %LOG% Found raylib archive, proceeding with extraction...
  
  echo %LOG% Running PowerShell extraction...
  powershell -NoProfile -Command ^
    "try { " ^
    "  Expand-Archive -Path 'raylib-5.5_win64_mingw-w64.zip' -DestinationPath '.' -Force; " ^
    "  exit 0; " ^
    "} catch { " ^
    "  Write-Host 'PowerShell extraction failed, trying .NET method...'; " ^
    "  Add-Type -AssemblyName System.IO.Compression.FileSystem; " ^
    "  [System.IO.Compression.ZipFile]::ExtractToDirectory('raylib-5.5_win64_mingw-w64.zip', '.'); " ^
    "  exit 0; " ^
    "}"
  
  if errorlevel 1 (
    echo %ERROR% Failed to extract raylib-5.5_win64_mingw-w64.zip using PowerShell
    set "EXTRACTION_SUCCESS=0"
  ) else (
    echo %LOG% PowerShell completed, checking extraction results...
    
    timeout /t 2 /nobreak >nul 2>&1
    
    if exist "%RAYLIB_DIR%" (
      echo %SUCCESS% Successfully extracted raylib using PowerShell
      set "EXTRACTION_SUCCESS=1"
    ) else (
      echo %ERROR% Extraction appeared to succeed but raylib directory not found
      set "EXTRACTION_SUCCESS=0"
      echo %LOG% Listing current directory contents:
      dir /b
    )
  )
) else (
  echo %ERROR% raylib archive not found for extraction!
  set "EXTRACTION_SUCCESS=0"
)

:: Finalize
echo %LOG% Finalizing dependencies...

if not exist "%TINY_DIR%" (
  echo %LOG% Creating %TINY_DIR%...
  mkdir "%TINY_DIR%" 2>nul
  if errorlevel 1 (
    echo %ERROR% Failed to create directory %TINY_DIR%
    goto :cleanup
  )
)

if exist "tinyfiledialogs.h" (
  move /Y "tinyfiledialogs.h" "%TINY_DIR%\" >nul
  if errorlevel 1 (
    echo %ERROR% Failed to move tinyfiledialogs.h
  ) else (
    echo %SUCCESS% Moved tinyfiledialogs.h to %TINY_DIR%
  )
) else (
  echo %WARNING% tinyfiledialogs.h not found, skipping...
)

if exist "tinyfiledialogs.c" (
  move /Y "tinyfiledialogs.c" "%TINY_DIR%\" >nul
  if errorlevel 1 (
    echo %ERROR% Failed to move tinyfiledialogs.c
  ) else (
    echo %SUCCESS% Moved tinyfiledialogs.c to %TINY_DIR%
  )
) else (
  echo %WARNING% tinyfiledialogs.c not found, skipping...
)

:: Check raylib folder after extraction
if exist "%RAYLIB_DIR%" (
  echo %SUCCESS% raylib folder "%RAYLIB_DIR%" found after extraction.
) else (
  echo %ERROR% raylib folder "%RAYLIB_DIR%" not found after extraction!
)

:cleanup
echo %LOG% Cleaning up downloaded files...

if "%EXTRACTION_SUCCESS%"=="1" (
  del /F /Q *.zip >nul 2>&1
  if errorlevel 1 (
    echo %ERROR% Failed to delete .zip files.
  ) else (
    echo %SUCCESS% Removed .zip files.
  )
) else (
  echo %WARNING% Keeping .zip files due to extraction issues.
)

:: Clean up
del /F /Q *.txt >nul 2>&1
if errorlevel 1 (
  echo %LOG% No .txt files to clean up.
) else (
  echo %SUCCESS% Removed .txt files.
)

echo %LOG% Cleanup completed.

::Export line to PowerShell 
if exist "%RAYLIB_DIR%" (
  set "EXPORT_LINE= $env:Path += ';%CD%\build;%CD%\deps\%RAYLIB_DIR%\lib'"
  set "PROFILE=%USERPROFILE%\Documents\WindowsPowerShell\profile.ps1"

  if not exist "%USERPROFILE%\Documents\WindowsPowerShell" (
    mkdir "%USERPROFILE%\Documents\WindowsPowerShell"
  )

  findstr /C:"%CD%\deps\%RAYLIB_DIR%\lib" "%PROFILE%" >nul 2>&1
  if errorlevel 1 (
    echo %SUCCESS% Adding export to %PROFILE%
    >>"%PROFILE%" echo %EXPORT_LINE%
  ) else (
    echo %WARNING% Export already exists in %PROFILE%, skipping...
  )
) else (
  echo %WARNING% Skipping PowerShell profile update due to missing raylib directory.
)

popd
goto :end

:: Function: download
:download
set "URL=%~1"
if "%URL%"=="" (
  echo %ERROR% No URL provided to download function.
  goto :eof
)
for %%I in ("%URL%") do set "FNAME=%%~nxI"
echo %LOG% Downloading %URL% ...

if "%USE_CURL%"=="1" (
  "%CURL_PATH%" -L -o "%FNAME%" "%URL%" --silent --show-error
  if errorlevel 1 (
    echo %ERROR% Failed to download %URL% using curl
    echo Failed URL: %URL%>>download_errors.log
  ) else (
    echo %SUCCESS% Successfully downloaded %URL% using curl
  )
) else (
  powershell -NoProfile -Command ^
    "try { Invoke-WebRequest -Uri '%URL%' -OutFile '%FNAME%' -UseBasicParsing -ErrorAction Stop; exit 0 } catch { exit 1 }"
  if errorlevel 1 (
    echo %ERROR% Failed to download %URL% using PowerShell
    echo Failed URL: %URL%>>download_errors.log
  ) else (
    echo %SUCCESS% Successfully downloaded %URL% using PowerShell
  )
)
goto :eof

:end
echo %LOG% Done.
endlocal
