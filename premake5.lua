local path = require("path")

function findQtInstallation()
    local qtInstallRoot = os.getenv("QTDIR") or "C:/Qt"
    local qtVersions = os.matchdirs(qtInstallRoot .. "/6.*")

    if #qtVersions == 0 then
        error("Could not find Qt installation in " .. qtInstallRoot .. ". Please set the QTDIR environment variable.")
    end

    table.sort(qtVersions)
    local latestQt = qtVersions[#qtVersions]
    local compilerDirs = os.matchdirs(latestQt .. "/msvc*")

    if #compilerDirs == 0 then
        error("No MSVC compiler folder found in Qt installation at: " .. latestQt)
    end

    return compilerDirs[#compilerDirs]
end
qtDir = findQtInstallation()

function runWindeployQt(qtDir, targetDir, debug)
    local windeployqt = path.join(qtDir, "bin", "windeployqt.exe")
    local modeFlag = debug and "--debug" or "--release"

    return {
        -- Make sure the target dir exists
        string.format('if not exist "%s" mkdir "%s"', targetDir, targetDir),

        -- Run windeployqt on the built executable with correct flag
        string.format('"%s" %s "%s/H1ModTools.exe"', windeployqt, modeFlag, targetDir)
    }
end

workspace "H1ModTools"
    architecture "x64"
    configurations { "Debug", "Release" }
    location "./build"
    objdir "%{wks.location}/obj"
    targetdir "%{wks.location}/bin/%{cfg.platform}/%{cfg.buildcfg}"
    startproject "H1ModTools"

project "H1ModTools"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++23"

    debugdir "%{cfg.targetdir}"

    files { "src/**.h", "src/**.cpp", "src/**.ts", "src/**.ui", "src/**.qrc" }

    local rootPath = path.getabsolute(".")

    -- Qt setup
    local package_path = package.path .. ";" .. rootPath .. "/deps/premake-qt/?.lua"
    package.path = package_path
    require("qt")
    local qt = premake.extensions.qt

    qt.enable()
    qtuseexternalinclude(true)
    qtpath(qtDir)
    qtmodules { "core", "gui", "widgets", "opengl" }
    qtprefix "Qt6"

    -- Copy static files to build folder
    local staticSource = path.getabsolute("static")
    local staticDest = path.join(path.translate("%{cfg.targetdir}"), "static")
    postbuildcommands {
        string.format('(robocopy "%s" "%s" /e) ^& exit 0', staticSource, staticDest)
    }

    filter "configurations:Debug"
        qtsuffix "d"
        defines { "DEBUG" }
        symbols "On"

        postbuildcommands(runWindeployQt(qtDir, path.translate("%{cfg.targetdir}"), true))

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

        postbuildcommands(runWindeployQt(qtDir, path.translate("%{cfg.targetdir}"), false))

    filter "action:vs*"
        buildoptions { "/Zc:__cplusplus" }

    filter {}