# H1 Mod Tools

A graphical interface for modding _Call of Duty: Modern Warfare Remastered (2016)_, inspired by the official mod tools released by Treyarch for _Call of Duty: Black Ops 3_.

## Setup

1. Download and install the Qt Online Installer [from this link](https://doc.qt.io/qt-6/get-and-install-qt.html) (requires a Qt account — select **"Individual"** during signup).
2. In the Qt installer, log in and agree to the terms and conditions for using the Open Source edition. Make sure to check **"I'm an individual using Qt under the open-source license."**
3. On the **Installation Options** screen, select **"Custom Installation"** at the bottom.
4. In the **Customize** step, expand the Qt version you want to install, and make sure to select:
   - `MSVC 2022 64-bit`
   - `MinGW 13.1.0 64-bit`
   - `Build Tools` (under the "Tools" section, not under the Qt version)
5. Accept the remaining license agreements and let the installer complete the setup.
6. Download and install the Qt Visual Studio Tools extension [from this link](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2022).
7. Open `H1ModTools.sln` in Visual Studio, then go to **Tools → Options** to open the Visual Studio settings.
8. In the **Qt** section, go to **Versions**, click **Import**, and browse to `C:\Qt`. Locate your installed version (e.g., `6.9.1`), go into the `msvc2022_64` folder, and select it.  
   **Important:** After importing, ensure the version name is exactly `"MSVC 64-bit"`.
9. You can now build the project!
