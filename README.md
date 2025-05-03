# Guide d’installation Windows 11 pour le projet Uno/SFML

## Prérequis

* Un PC sous **Windows 11** avec droits administrateur
* **Git** installé ([https://git-scm.com/download/win](https://git-scm.com/download/win))
* **PowerShell** ou **Windows Terminal** (recommandé)

---

## 1. Installer Visual Studio Build Tools

1. Téléchargez l’installateur :

   ```
   https://aka.ms/vs/17/release/vs_BuildTools.exe
   ```
2. Ouvrez-le en mode Admin
3. Lancez l’installation. Une fois terminée, `cl.exe` et `msbuild.exe` sont déjà dans votre PATH.

---

## 2. Installer vcpkg pour gérer SFML

1. Ouvrez PowerShell en Admin et tapez :

   ```powershell
   cd C:\Dev
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   ```
2. Ajoutez vcpkg au PATH et définissez la variable :

   ```powershell
   setx VCPKG_ROOT "C:\Dev\vcpkg" /M
   setx PATH "%PATH%;C:\Dev\vcpkg" /M
   ```
3. Vérifiez que tout est OK :

   ```powershell
   vcpkg --version
   ```

---

## 3. Installer Ninja (générateur de build)

1. Via Winget :

   ```powershell
   winget install Ninja-build.Ninja
   ```
2. Testez : `ninja --version`

---

## 4. Configurer CLion

1. **Settings → Build Tools → CMake**

    * **Generator** : choisissez `Ninja`
    * **CMake options** : ajoutez :

      ```
      -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
      -DVCPKG_MANIFEST_MODE=ON
      -DVCPKG_TARGET_TRIPLET=x64-windows
      ```
2. Rechargez/le reload le projet et laissez CLion indexer.
3. Cible **Debug** ou **Release**, à sélectionner en haut à droite.
4. Cliquez sur ▶️ **Run** ou 🐞 **Debug** pour compiler et lancer.

---

## 5. Configurer VS Code

1. Installez l’extension **CMake Tools**.
2. Dans `.vscode/settings.json`, ajoutez :

   ```json
   {
     "cmake.generator": "Ninja",
     "cmake.toolchain": "${env:VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
     "cmake.configureSettings": {
       "VCPKG_MANIFEST_MODE": "ON",
       "VCPKG_TARGET_TRIPLET": "x64-windows"
     }
   }
   ```
3. Ouvrez le dossier du projet, lancez **CMake: Configure**, puis **CMake: Build**.
4. Créez `.vscode/launch.json` pour le debug :

   ```json
   {
     "version": "0.2.0",
     "configurations": [
       {
         "name": "Debug Uno (MSVC)",
         "type": "cppvsdbg",
         "request": "launch",
         "program": "${workspaceFolder}/cmake-build-debug/Uno.exe",
         "cwd": "${workspaceFolder}",
         "console": "integratedTerminal"
       }
     ]
   }
   ```
5. Sélectionnez **Debug Uno (MSVC)**, appuyez sur F5.

---

## 6. Conseils et optimisations

* **Cache vcpkg** : mutualisez les dépôts entre projets.
* **ccache/sccache** : accélère les rebuilds MSVC.
* **Terminal intégré** : PowerShell Core ou Windows Terminal.
* **Versions** : gardez vos dépendances à jour (`vcpkg update`).

> Vous voilà prêt à compiler, lancer et déboguer votre Uno/SFML sous Windows 11 ! N’hésitez pas à adapter ce guide à vos habitudes.
