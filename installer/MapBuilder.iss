#define AppName "Combatlr Map Builder"
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

[Setup]
AppId={{B2F17A5D-5A0B-4B4B-9B0D-4D7F7A3E0C21}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=CombatLR
DefaultDirName={autopf}\Combatlr Map Builder
DefaultGroupName=Combatlr Map Builder
OutputDir=..\artifacts
OutputBaseFilename=Combatlr-MapBuilder-v{#AppVersion}-Setup
SetupIconFile=..\CombatlrMapBuilder\MapBuilder.ico
UninstallDisplayIcon={app}\CombatlrMapBuilder.exe
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Files]
Source: "..\publish\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Combatlr Map Builder"; Filename: "{app}\CombatlrMapBuilder.exe"
Name: "{autodesktop}\Combatlr Map Builder"; Filename: "{app}\CombatlrMapBuilder.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"
