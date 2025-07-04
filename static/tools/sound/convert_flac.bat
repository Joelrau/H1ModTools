@ECHO OFF
TITLE WAV/MP3 TO FLAC
CLS

IF [%1] == [] GOTO NO_ARG

set flac_version=flac-1.3.1-win
set ffmpeg_version=ffmpeg-7.0.1-essentials_build
set script_dir=%~dp0

:LOOP
IF [%1] == [] GOTO END_LOOP

set filepath=%1
for %%F in (%filepath%) do (
    set basename=%%~nF
    set extension=%%~xF
)

set input_dir=%~dp1
set input_path=%input_dir%\%basename%%extension%

set output_dir=%~dp1
set output_path=%output_dir%\%basename%.flac

cd %script_dir%

IF /I "%extension%"==".wav" (
    cd %script_dir%%flac_version%\win64
    flac.exe --no-delete-input-file "%input_path%" --output-name="%output_path%" -6 -f --no-padding --keep-foreign-metadata --blocksize=1024
    cd %script_dir%
) ELSE IF /I "%extension%"==".mp3" (
    cd %script_dir%%ffmpeg_version%\bin
    ffmpeg.exe -i "%input_path%" "%output_dir%\%basename%.wav"
    
    cd %script_dir%%flac_version%\win64
    flac.exe --no-delete-input-file "%output_dir%\%basename%.wav" --output-name="%output_path%" -6 -f --no-padding --keep-foreign-metadata --blocksize=1024

    del "%output_dir%\%basename%.wav"
    cd %script_dir%
) ELSE (
    ECHO Unsupported file type: %extension%
)

SHIFT
GOTO LOOP

:END_LOOP
GOTO EXIT

:NO_ARG
ECHO "Insert your .wav or .mp3 files to the batch to convert."
GOTO EXIT

:EXIT
PAUSE
EXIT