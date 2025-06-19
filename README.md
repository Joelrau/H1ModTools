# H1 Mod Tools
A GUI interface for modding _Call of Duty: Modern Warfare Remastered (2016)_, inspired by the official mod tools released by Treyarch for _Call of Duty: Black Ops 3_.

## Setup
1. Download and install the Qt Online Installer [from this link here](https://doc.qt.io/qt-6/get-and-install-qt.html) (requires a account for a individual)
2. In the Qt installer, login and and agree to the terms and conditions of using Open Source Qt, and check "I'm an individual..."
3. Once at **Installation options**, check the **"Custom Installation"** at the bottom.
4. Once at **Customize**, you'll need to hit the dropdown box for your Qt version, and make sure you have `MSVC 2022 64-bit`, `MinGW 13.1.0 64-bit`, and `Build Tools` outside the Qt tab.
5. Sign the rest of the license agreemenets, and then Qt will finally install
6. Download and install Qt Visaul Studio Tools [from this link here](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2022)
7. Open the H1ModTools.sln, and go to Tools -> Options to open up the Visual Studio settings
8. Locate the Qt options, and go to Versions. Click **Import**, and then go to `C:\Qt`, and find your version _(mine is 6.9.1)_, and then click the `msvc2022_64` folder and select it. Once you select it, **make sure the name of the version is "MSVC 64-bit"** exactly.
9. You can now build!
