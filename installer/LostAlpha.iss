;
; S.T.A.L.K.E.R. - Lost Alpha
; Script for Inno Setup 5 compiler
; First version: 2014.04.08
; First author to blame: utak3r
; Last modification: 2017.04.27
; Last modifier: utak3r
;
; put the game files in the [game_distrib_files] folder.
; after this, call prepare_archives.cmd script.
;
; This script is prepared for and meant to be used with:
; Inno Setup 5.5.4 unicode [ http://www.jrsoftware.org/isdl.php ]
; Inno Script Studio 2.1.0.20  [ https://www.kymoto.org/products/inno-script-studio/downloads ]
;
;
; The directory structure to build the installer is as follows:
;
; LostAlpha.iss (this script)
; 7za.exe
; prepare_archives.cmd
;
; [installer_images]
; installer_images\LAinstallerImage.bmp
; installer_images\LAinstallerSmallImage.bmp
; installer_images\stalker.ico
;
; [Output\3rdparties] (additional software to be installed)
; Output\3rdparties\directx_Jun2010_redist.exe (from: http://www.microsoft.com/en-us/download/details.aspx?id=8109 )
; Output\3rdparties\oalinst.exe
; Output\3rdparties\vcredist_x86.exe (this should be from: http://www.microsoft.com/en-us/download/details.aspx?id=40784 )
; Output\3rdparties\Xvid-1.3.2-20110601.exe
;
; [game_distrib_files] (complete release of the game to be installed)
; [Output] (the compiled installer will be put here)
; [Output\game] (archives of the games files will be put here using prepare_archives.cmd script)
;
;

; for a patch, comment out the below line.
; uncomment it when releasing a bundle (with external 7z archives).
;#define BundleRelease

; password for 7z files!
#define archpasswd "sdu28042elmd"

; this is an estimated disk usage
; it cannot be determined by the installer itself,
; due to external archives used.
; It's in bytes!
#define LA_disk_usage "17510000000"

; dirs used:
#define LA_game_files ".\game_distrib_files"
#define LA_3rd_party_files ".\game_distrib_files\3rdparties"
#define LA_installer_support_files "."

; versions, names etc.:
#define LA_shortcut_name "S.T.A.L.K.E.R. - Lost Alpha"
#define LA_app_name "S.T.A.L.K.E.R.: Lost Alpha Developers Cut"
#define LA_directory_name "S.T.A.L.K.E.R. - Lost Alpha"
#define LA_copyright "dezowave"
#define LA_version "1.4004"
#define LA_version_text "1.4004"

[Files]
Source: "{#LA_installer_support_files}\7za.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
Source: "{#LA_game_files}\manual.pdf"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\start.bat"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\readme.nfo"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
;Source: "{#LA_installer_support_files}\fsgame_template.ltx"; DestDir: "{tmp}"; Flags: deleteafterinstall
#ifndef BundleRelease
Source: "{#LA_game_files}\appdata\*"; DestDir: "{app}\appdata"; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\bins\*"; DestDir: "{app}\bins"; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\*"; DestDir: "{app}\gamedata"; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\changelog*"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata.db*"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

;if other patches were installed already
;Source: "{#LA_3rd_party_files}\oalinst.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall skipifsourcedoesntexist ignoreversion
;Source: "{#LA_3rd_party_files}\vcredist_x86.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall skipifsourcedoesntexist ignoreversion
;Source: "{#LA_3rd_party_files}\Xvid-1.3.3-20140407.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall skipifsourcedoesntexist ignoreversion
;Source: "{#LA_3rd_party_files}\DirectX_runtime\*"; DestDir: "{tmp}"; Flags: deleteafterinstall skipifsourcedoesntexist ignoreversion createallsubdirs recursesubdirs
#endif

[Run]
; unpack game files
#ifdef BundleRelease
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}\appdata"" ""{src}\game\appdata.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingAppdata}"; StatusMsg: "{cm:msgInstallingAppdata}"
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}\bins"" ""{src}\game\bins.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingBins}"; StatusMsg: "{cm:msgInstallingBins}"
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}\gamedata"" ""{src}\game\gamedata.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingGamedata}"; StatusMsg: "{cm:msgInstallingGamedata}"
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}"" ""{src}\game\maindir.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingMaindir}"; StatusMsg: "{cm:msgInstallingMaindir}"
#endif

; install prerequisities
Filename: "{tmp}\vcredist_x86.exe"; Flags: hidewizard skipifdoesntexist; Description: "{cm:msgInstallingVcredist}"; StatusMsg: "{cm:msgInstallingVcredist}"; Check: VCRedistNeedsInstall
Filename: "{tmp}\oalinst.exe"; Flags: hidewizard skipifdoesntexist; Description: "{cm:msgInstallingOAL}"; StatusMsg: "{cm:msgInstallingOAL}"
;Filename: "{tmp}\Xvid-1.3.3-20140407.exe"; Flags: hidewizard skipifdoesntexist; Description: "{cm:msgInstallingXvid}"; StatusMsg: "{cm:msgInstallingXvid}"
; include the unpacked version of DirectX runtimes:
Filename: "{tmp}\DXSETUP.exe"; Flags: hidewizard skipifdoesntexist; Description: "{cm:msgInstallingDXredist}"; StatusMsg: "{cm:msgInstallingDXredist}"

[InstallDelete]
;Type: files; Name: "{app}\appdata\user.ltx"
Type: filesandordirs; Name: "{app}\appdata\shaders_cache"
;Type: files; Name: "{app}\gamedata\scripts\ui_main_dik_keys.script"

[Icons]
Name: "{commonprograms}\{#LA_shortcut_name}"; Filename: "{app}\bins\XR_3DA.exe"; WorkingDir: "{app}"; Parameters: "-external -noprefetch"
Name: "{commondesktop}\{#LA_shortcut_name}"; Filename: "{app}\bins\XR_3DA.exe"; WorkingDir: "{app}"; Parameters: "-external -noprefetch"

[Setup]
PrivilegesRequired=admin
#ifdef BundleRelease
ExtraDiskSpaceRequired={#LA_disk_usage}
#endif
AppName={#LA_app_name}
AppVersion={#LA_version_text}
AppCopyright={#LA_copyright}
DefaultDirName={pf}\{#LA_directory_name}
DisableDirPage=no
DisableProgramGroupPage=yes
AppPublisher={#LA_copyright}
VersionInfoVersion={#LA_version}
VersionInfoCompany={#LA_copyright}
VersionInfoDescription={#LA_app_name}
VersionInfoTextVersion={#LA_version_text}
VersionInfoCopyright={#LA_copyright}
VersionInfoProductName={#LA_app_name}
VersionInfoProductVersion={#LA_version}
VersionInfoProductTextVersion={#LA_version_text}
MinVersion=0,5.01sp3
WizardImageFile={#LA_installer_support_files}\installer_images\LAinstallerImage.bmp
SetupIconFile={#LA_installer_support_files}\installer_images\stalker.ico
WizardSmallImageFile={#LA_installer_support_files}\installer_images\LAinstallerSmallImage.bmp

[CustomMessages]
msgInstallingBins=Installing binaries
msgInstallingGamedata=Installing game data files
msgInstallingAppdata=Installing application data files
msgInstallingMaindir=Installing main game files
msgInstallingVcredist=Installing Microsoft VC++ runtimes
msgInstallingDXredist=DirectX runtimes
msgInstallingOAL=Installing audio codec
msgInstallingXvid=Installing video codec
msgDeletingUnwantedFiles=Deleting not needed files

[UninstallDelete]
; don't delete: appdata, screenshots, logs.
; maybe add some question if user wants to remove his player's data?
Type: filesandordirs; Name: "{app}\bins"
Type: filesandordirs; Name: "{app}\gamedata"
Type: files; Name: "{app}\start.bat"
;Type: files; Name: "{app}\gamedata.db*"
;Type: files; Name: "{app}\fsgame.ltx"

[Code]
#IFDEF UNICODE
  #DEFINE AW "W"
#ELSE
  #DEFINE AW "A"
#ENDIF
type
  INSTALLSTATE = Longint;
const
  INSTALLSTATE_INVALIDARG = -2;  // An invalid parameter was passed to the function.
  INSTALLSTATE_UNKNOWN = -1;     // The product is neither advertised or installed.
  INSTALLSTATE_ADVERTISED = 1;   // The product is advertised but not installed.
  INSTALLSTATE_ABSENT = 2;       // The product is installed for a different user.
  INSTALLSTATE_DEFAULT = 5;      // The product is installed for the current user.

  VC_2005_REDIST_X86 = '{A49F249F-0C91-497F-86DF-B2585E8E76B7}';
  VC_2005_REDIST_X64 = '{6E8E85E8-CE4B-4FF5-91F7-04999C9FAE6A}';
  VC_2005_REDIST_IA64 = '{03ED71EA-F531-4927-AABD-1C31BCE8E187}';
  VC_2005_SP1_REDIST_X86 = '{7299052B-02A4-4627-81F2-1818DA5D550D}';
  VC_2005_SP1_REDIST_X64 = '{071C9B48-7C32-4621-A0AC-3F809523288F}';
  VC_2005_SP1_REDIST_IA64 = '{0F8FB34E-675E-42ED-850B-29D98C2ECE08}';
  VC_2005_SP1_ATL_SEC_UPD_REDIST_X86 = '{837B34E3-7C30-493C-8F6A-2B0F04E2912C}';
  VC_2005_SP1_ATL_SEC_UPD_REDIST_X64 = '{6CE5BAE9-D3CA-4B99-891A-1DC6C118A5FC}';
  VC_2005_SP1_ATL_SEC_UPD_REDIST_IA64 = '{85025851-A784-46D8-950D-05CB3CA43A13}';

  VC_2008_REDIST_X86 = '{FF66E9F6-83E7-3A3E-AF14-8DE9A809A6A4}';
  VC_2008_REDIST_X64 = '{350AA351-21FA-3270-8B7A-835434E766AD}';
  VC_2008_REDIST_IA64 = '{2B547B43-DB50-3139-9EBE-37D419E0F5FA}';
  VC_2008_SP1_REDIST_X86 = '{9A25302D-30C0-39D9-BD6F-21E6EC160475}';
  VC_2008_SP1_REDIST_X64 = '{8220EEFE-38CD-377E-8595-13398D740ACE}';
  VC_2008_SP1_REDIST_IA64 = '{5827ECE1-AEB0-328E-B813-6FC68622C1F9}';
  VC_2008_SP1_ATL_SEC_UPD_REDIST_X86 = '{1F1C2DFC-2D24-3E06-BCB8-725134ADF989}';
  VC_2008_SP1_ATL_SEC_UPD_REDIST_X64 = '{4B6C7001-C7D6-3710-913E-5BC23FCE91E6}';
  VC_2008_SP1_ATL_SEC_UPD_REDIST_IA64 = '{977AD349-C2A8-39DD-9273-285C08987C7B}';
  VC_2008_SP1_MFC_SEC_UPD_REDIST_X86 = '{9BE518E6-ECC6-35A9-88E4-87755C07200F}';
  VC_2008_SP1_MFC_SEC_UPD_REDIST_X64 = '{5FCE6D76-F5DC-37AB-B2B8-22AB8CEDB1D4}';
  VC_2008_SP1_MFC_SEC_UPD_REDIST_IA64 = '{515643D1-4E9E-342F-A75A-D1F16448DC04}';

  VC_2010_REDIST_X86 = '{196BB40D-1578-3D01-B289-BEFC77A11A1E}';
  VC_2010_REDIST_X64 = '{DA5E371C-6333-3D8A-93A4-6FD5B20BCC6E}';
  VC_2010_REDIST_IA64 = '{C1A35166-4301-38E9-BA67-02823AD72A1B}';
  VC_2010_SP1_REDIST_X86 = '{F0C3E5D1-1ADE-321E-8167-68EF0DE699A5}';
  VC_2010_SP1_REDIST_X64 = '{1D8E6291-B0D5-35EC-8441-6616F567A0F7}';
  VC_2010_SP1_REDIST_IA64 = '{88C73C1C-2DE5-3B01-AFB8-B46EF4AB41CD}';

  VC_2013_REDIST_X86 = '{CE085A78-074E-4823-8DC1-8A721B94B76D}'; // 12.0.21005

function MsiQueryProductState(szProduct: string): INSTALLSTATE;
  external 'MsiQueryProductState{#AW}@msi.dll stdcall';

function VCVersionInstalled(const ProductID: string): Boolean;
begin
  Result := MsiQueryProductState(ProductID) = INSTALLSTATE_DEFAULT;
end;

function VCRedistNeedsInstall: Boolean;
begin
  Result := not (VCVersionInstalled(VC_2013_REDIST_X86));
end;

procedure ModifyUserLtx(variable: string; value: string);
var
  FileName: String;
  LineIndex: Integer;
  StringList: TStringList;
begin
  { NOTE: it still doesn't work, don't use it yet! }
  FileName := ExpandConstant('{app}\appdata\user.ltx');
  StringList := TStringList.Create;
  try
    StringList.LoadFromFile(FileName);
    MsgBox(StringList.Text, mbInformation, MB_OK);
    if StringList.Find(variable, LineIndex) then
    begin
      MsgBox('Found: '+StringList[LineIndex], mbInformation, MB_OK);
      StringList[LineIndex] := value;
      StringList.SaveToFile(FileName);
    end;
  finally
    StringList.Free;
  end;
end;

procedure WipeUserLtx;
begin
  if MsgBox('It is recommended to recreate your settings for the game. Do you want to do it?',
            mbConfirmation, MB_YESNO or MB_DEFBUTTON1)
            = IDYES
  then
  begin
    DeleteFile(ExpandConstant('{app}\appdata\user.ltx'));
    //u3: so - I thought we were going to let engine create user.ltx from scratch...
    //RenameFile(ExpandConstant('{app}\appdata\user.ltx'), ExpandConstant('{app}\appdata\user_old.ltx'))
    //RenameFile(ExpandConstant('{app}\appdata\user_default.ltx'), ExpandConstant('{app}\appdata\user.ltx'))
  end;
end;

// procedure CreateFsGameLtx(targetPath: String);
// var
//   data: String;
// begin
//   if LoadStringFromFile(ExpandConstant('{tmp}\fsgame_template.ltx'), AnsiString(data)) then
//   begin
//     StringChangeEx(data, '$app_data_root_installer_template$',
//                    '$app_data_root$		= true|		false|	' + targetPath, True);
//     SaveStringToFile(ExpandConstant('{app}\fsgame.ltx'), data, False);
//   end;
// end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then begin
    DelTree(ExpandConstant('{app}\bins'), True, True, True);
    DelTree(ExpandConstant('{app}\gamedata'), True, True, True);
    //DeleteFile(ExpandConstant('{app}\gamedata.dbh'));
  end;
  if CurStep = ssPostInstall then begin
    //WipeUserLtx;
    //CreateFsGameLtx(ExpandConstant('{userdocs}'));
    //ModifyUserLtx('r2_gloss_factor', '1.');
  end;
end;
