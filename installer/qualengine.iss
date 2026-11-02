; =============================================================================
; QualEngine - Inno Setup Kurulum Betiği
; Sürüm: v0.2.0-alpha
; =============================================================================

#define MyAppName "QualEngine"
#define MyAppVersion "0.2.0-alpha"
#define MyAppPublisher "QualEngine Team"
#define MyAppURL "https://github.com/yusufkerman/qual_engine"
#define MyAppExeName "Application.exe"
#define MyPlayerExeName "GameRuntime.exe"

[Setup]
; Kurulum Kimlik Bilgileri
AppId={{8B2C4F1A-4D3E-4B5C-9A8B-7C6D5E4F3A2B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} v{#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}/releases

; Kurulum Konumu
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=auto

; Çıkış Paketi Ayarları
OutputDir=..\dist
OutputBaseFilename=QualEngine-v{#MyAppVersion}-Setup
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

; Modern Arayüz Tasarımı
WizardStyle=modern
DisableWelcomePage=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Hazırlanmış staging dizinindeki tüm motor ve editör dosyaları
Source: "..\dist\staging\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Başlat Menüsü Kısayolları
Name: "{group}\{#MyAppName} (Vesta Editor)"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{group}\QualEngine Standalone Player"; Filename: "{app}\{#MyPlayerExeName}"; WorkingDir: "{app}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

; Masaüstü Kısayolu
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
; Kurulum tamamlandığında editörü başlatma seçeneği
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent

