@ECHO OFF
TITLE MP3/FLAC/ADPCM TO WAV (Files & Folders)
CLS

IF "%~1"=="" GOTO NO_ARG

set "ffmpeg_version=ffmpeg-7.0.1-essentials_build"
set "script_dir=%~dp0"

REM Loop through all arguments (files & folders)
:PROCESS
FOR %%A IN (%*) DO (
    CALL :CHECK_AND_PROCESS "%%~A"
)
GOTO EXIT

REM Check if input is a file or folder
:CHECK_AND_PROCESS
IF EXIST "%~1\*" (
    REM Input is a folder, process recursively
    FOR /R "%~1" %%F IN (*.mp3 *.flac *.wav) DO CALL :CONVERT "%%F"
) ELSE (
    REM Input is a single file
    CALL :CONVERT "%~1"
)
EXIT /B

REM Convert files using FFmpeg
:CONVERT
setlocal
set "input_path=%~1"
set "input_dir=%~dp1"
set "basename=%~n1"
set "extension=%~x1"
set "output_path=%input_dir%%basename%.wav"
set "temp_output_path=%output_path%.tmp.wav"

cd "%script_dir%%ffmpeg_version%\bin"

IF /I "%extension%"==".mp3" (
    ffmpeg.exe -y -i "%input_path%" "%output_path%"
) ELSE IF /I "%extension%"==".flac" (
    ffmpeg.exe -y -i "%input_path%" "%output_path%"
) ELSE IF /I "%extension%"==".wav" (
    ffmpeg.exe -y -i "%input_path%" -acodec pcm_s16le "%temp_output_path%"
    IF EXIST "%output_path%" del "%output_path%"
    IF EXIST "%temp_output_path%" ren "%temp_output_path%" "%basename%.wav"
) ELSE (
    ECHO Unsupported file type: "%input_path%"
)

cd "%script_dir%"
endlocal
EXIT /B

:NO_ARG
ECHO Drag and drop MP3, FLAC, ADPCM WAV files, or folders to convert them to PCM WAV.
GOTO EXIT

:EXIT
PAUSE
EXIT