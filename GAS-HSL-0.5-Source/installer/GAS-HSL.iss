[Setup]
AppId={{04D599B0-F9E5-4F68-AB2F-C9ACEEACF19D}
AppName=GAS-HSL
AppVersion=0.5
AppVerName=GAS-HSL 0.5
AppPublisher=Gassy
AppPublisherURL=https://gasbox.cn
AppSupportURL=https://gasbox.cn
DefaultDirName={commonappdata}\obs-studio\plugins\gas-hsl
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableWelcomePage=yes
DisableReadyPage=yes
OutputDir=..\..
OutputBaseFilename=GAS-HSL-0.5-Setup
SetupIconFile=logo.ico
UninstallDisplayIcon={app}\logo.ico
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
CloseApplications=yes
RestartApplications=no
Compression=lzma2
SolidCompression=yes

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Files]
; OBS 33+ uses the root DLL. OBS 31/32 uses the bin\64bit DLL.
Source: "..\release\gas-hsl\bin\64bit\gas-hsl.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\gas-hsl\bin\64bit\gas-hsl.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
Source: "..\release\gas-hsl\data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "logo.ico"; DestDir: "{app}"; Flags: ignoreversion
