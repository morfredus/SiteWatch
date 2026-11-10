; SiteWatch installer (Inno Setup 6). Run by scripts\windows\package-win.ps1, which defines:
;   /DAppVersion=<version>  /DAppVersionNum=<a.b.c.0>  /DStageDir=<folder to install>
; ASCII only and no BOM, like every other text file of the repository.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef AppVersionNum
  #define AppVersionNum "0.0.0.0"
#endif
#ifndef StageDir
  #error StageDir must be defined (see package-win.ps1)
#endif

[Setup]
; Never change this identifier: it is how a new version finds and replaces the installed one.
AppId={{C1D3C5B2-6E0B-4F0C-9B1B-5D0A7C3E4F11}
AppName=SiteWatch
AppVersion={#AppVersion}
AppVerName=SiteWatch {#AppVersion}
AppPublisher=morfredus
AppPublisherURL=https://morfredus.fr
VersionInfoVersion={#AppVersionNum}
DefaultDirName={autopf}\SiteWatch
DefaultGroupName=SiteWatch
AllowNoIcons=yes
UninstallDisplayIcon={app}\SiteWatch.exe
OutputBaseFilename=SiteWatch-{#AppVersion}-win64-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Per-user install by default (no administrator rights); the dialog still offers all users.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; Close a running SiteWatch instead of failing on a locked file.
CloseApplications=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\SiteWatch"; Filename: "{app}\SiteWatch.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\SiteWatch"; Filename: "{app}\SiteWatch.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\SiteWatch.exe"; Description: "{cm:LaunchProgram,SiteWatch}"; Flags: nowait postinstall skipifsilent

[Code]
// The user's data (configuration under %APPDATA%\SiteWatch, cache under %LOCALAPPDATA%\SiteWatch, settings under HKCU\Software\morfredus\SiteWatch) is kept
// by default; the question defaults to "No".
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if (CurUninstallStep = usPostUninstall) and not UninstallSilent then
    if MsgBox(ExpandConstant('{cm:KeepData}'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
    begin
      DelTree(ExpandConstant('{userappdata}\SiteWatch'), True, True, True);
      DelTree(ExpandConstant('{localappdata}\SiteWatch'), True, True, True);
      RegDeleteKeyIncludingSubkeys(HKCU, 'Software\morfredus\SiteWatch');
    end;
end;

[CustomMessages]
english.KeepData=Also delete your SiteWatch data (configuration, cache and settings)?%n%nChoose No to keep them for a later reinstall.
french.KeepData=Supprimer aussi vos donnees SiteWatch (configuration, cache et reglages) ?%n%nChoisir Non pour les conserver en vue d'une reinstallation.
