; Inno Setup script for the CPUSim64 Windows installer.
;
; Compiled by .github/workflows/package.yml with:
;   ISCC /DAppVersion=<x.y.z> /DArch=<x64|arm64> /DRepoRoot=<repo root> /DAppImageDir=<jpackage app-image> /DOutputDir=<dir> CPUSim64.iss
;
; Install scope (command line, handled natively by Inno Setup):
;   CPUSim64-<ver>-<arch>.exe /ALLUSERS      -> all users (admin; Program Files, HKLM)
;   CPUSim64-<ver>-<arch>.exe /CURRENTUSER   -> one user  (no admin; %LocalAppData%\Programs, HKCU)
; Add /VERYSILENT /SUPPRESSMSGBOXES /NORESTART for unattended installs.
; With neither switch, the setup wizard asks the user to choose.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef Arch
  #define Arch "x64"
#endif
#ifndef RepoRoot
  #define RepoRoot "..\.."
#endif
#ifndef AppImageDir
  #define AppImageDir RepoRoot + "\CPUSim64"
#endif
#ifndef OutputDir
  #define OutputDir RepoRoot
#endif

#define AppName "CPUSim64"
#define AppExe "CPUSim64.exe"
#define IconDir RepoRoot + "\ide\src\main\resources"

[Setup]
; Fixed AppId: identifies this product for upgrades and uninstall. Do not change.
AppId={{B7E3D1A4-5C2F-4E8B-9A16-3F0D8C7E2B51}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Richard Lesh
AppCopyright=(C) 2025-2026 Richard Lesh
VersionInfoVersion={#AppVersion}
VersionInfoDescription=64-bit virtual CPU, assembler, and simulation environment
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
; Default to a per-user install; /ALLUSERS (or the wizard dialog) elevates.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=commandline dialog
ChangesAssociations=yes
SetupIconFile={#IconDir}\app_icon.ico
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
OutputDir={#OutputDir}
OutputBaseFilename={#AppName}-{#AppVersion}-{#Arch}
Compression=lzma2/ultra
SolidCompression=yes
WizardStyle=modern
#if Arch == "arm64"
ArchitecturesAllowed=arm64
ArchitecturesInstallIn64BitMode=arm64
#else
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#AppImageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#IconDir}\asm_icon.ico"; DestDir: "{app}\icons"; Flags: ignoreversion
Source: "{#IconDir}\o64_icon.ico"; DestDir: "{app}\icons"; Flags: ignoreversion
Source: "{#IconDir}\sym_icon.ico"; DestDir: "{app}\icons"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

; File associations. HKA resolves to HKLM for an all-users install and HKCU for a
; one-user install, so each scope registers only its own associations.
[Registry]
; .asm
Root: HKA; Subkey: "Software\Classes\.asm\OpenWithProgids"; ValueType: string; ValueName: "CPUSim64.asm"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CPUSim64.asm"; ValueType: string; ValueName: ""; ValueData: "CPUSim64 Assembly File"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CPUSim64.asm\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\icons\asm_icon.ico"
Root: HKA; Subkey: "Software\Classes\CPUSim64.asm\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""
; .o64
Root: HKA; Subkey: "Software\Classes\.o64\OpenWithProgids"; ValueType: string; ValueName: "CPUSim64.o64"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CPUSim64.o64"; ValueType: string; ValueName: ""; ValueData: "CPUSim64 Object File"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CPUSim64.o64\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\icons\o64_icon.ico"
Root: HKA; Subkey: "Software\Classes\CPUSim64.o64\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""
; .sym
Root: HKA; Subkey: "Software\Classes\.sym\OpenWithProgids"; ValueType: string; ValueName: "CPUSim64.sym"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CPUSim64.sym"; ValueType: string; ValueName: ""; ValueData: "CPUSim64 Symbol File"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CPUSim64.sym\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\icons\sym_icon.ico"
Root: HKA; Subkey: "Software\Classes\CPUSim64.sym\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""
; .sym1
Root: HKA; Subkey: "Software\Classes\.sym1\OpenWithProgids"; ValueType: string; ValueName: "CPUSim64.sym1"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CPUSim64.sym1"; ValueType: string; ValueName: ""; ValueData: "CPUSim64 Symbol File"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CPUSim64.sym1\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\icons\sym_icon.ico"
Root: HKA; Subkey: "Software\Classes\CPUSim64.sym1\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""
; .sym2
Root: HKA; Subkey: "Software\Classes\.sym2\OpenWithProgids"; ValueType: string; ValueName: "CPUSim64.sym2"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CPUSim64.sym2"; ValueType: string; ValueName: ""; ValueData: "CPUSim64 Symbol File"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CPUSim64.sym2\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\icons\sym_icon.ico"
Root: HKA; Subkey: "Software\Classes\CPUSim64.sym2\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""
; .srcmap
Root: HKA; Subkey: "Software\Classes\.srcmap\OpenWithProgids"; ValueType: string; ValueName: "CPUSim64.srcmap"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CPUSim64.srcmap"; ValueType: string; ValueName: ""; ValueData: "CPUSim64 Source Map File"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CPUSim64.srcmap\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\icons\sym_icon.ico"
Root: HKA; Subkey: "Software\Classes\CPUSim64.srcmap\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""
; .def
Root: HKA; Subkey: "Software\Classes\.def\OpenWithProgids"; ValueType: string; ValueName: "CPUSim64.def"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CPUSim64.def"; ValueType: string; ValueName: ""; ValueData: "CPUSim64 Definition File"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CPUSim64.def\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\icons\asm_icon.ico"
Root: HKA; Subkey: "Software\Classes\CPUSim64.def\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""
