#define AppName "CombatLR GPS Simulator"
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

[Setup]
AppId={{C91E8A63-DA42-4D89-94E7-1C7C0DFA9A61}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=CombatLR
DefaultDirName={autopf}\CombatLR GPS Simulator
DefaultGroupName=CombatLR GPS Simulator
OutputDir=..\artifacts
OutputBaseFilename=CombatLR-GPS-Simulator-v{#AppVersion}-Setup
SetupIconFile=..\CombatLR_GpsSimulator\GpsSimulator.ico
UninstallDisplayIcon={app}\CombatLR_GpsSimulator.exe
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Files]
Source: "..\publish\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\CombatLR GPS Simulator"; Filename: "{app}\CombatLR_GpsSimulator.exe"
Name: "{autodesktop}\CombatLR GPS Simulator"; Filename: "{app}\CombatLR_GpsSimulator.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"
