;
; S.T.A.L.K.E.R. - Lost Alpha
; Script for Inno Setup 5 compiler
; First version: 2014.04.08
; First author to blame: utak3r
; Last modification: 2014.04.18
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
;#define LA_disk_usage "6510000000"

; dirs used:
#define LA_game_files ".\game_distrib_files"
#define LA_3rd_party_files ".\game_distrib_files\3rdparties"
#define LA_installer_support_files "."

; versions, names etc.:
#define LA_shortcut_name "S.T.A.L.K.E.R. - Lost Alpha DC"
#define LA_app_name "S.T.A.L.K.E.R.: Lost Alpha DC"
#define LA_directory_name "S.T.A.L.K.E.R. - Lost Alpha DC"
#define LA_StartMenu_directory_name "STALKER Lost Alpha DC"
#define LA_copyright "dezowave"
#define LA_version "1.4005"
#define LA_version_text "1.4005"

#define  NeedSystem = "5.1.3";

[Files]
Source: Include\ISMD5.dll; Flags: dontcopy;
Source: "{#LA_installer_support_files}\7za.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
;Source: "{#LA_game_files}\lost_alpha_game_manual.pdf"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
;Source: "{#LA_installer_support_files}\fsgame_template.ltx"; DestDir: "{tmp}"; Flags: deleteafterinstall
#ifndef BundleRelease
Source: "{#LA_game_files}\gamedata\meshes\actors\trader\trader.ogf"; DestDir: "{app}\gamedata\meshes\actors\trader"; Components: sidor; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\textures\act\act_barman_bump.dds"; DestDir: "{app}\gamedata\textures\act"; Components: sidor; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\appdata\*"; DestDir: "{app}\appdata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\bins\*"; DestDir: "{app}\bins"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\anims.db0"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\levels.db0"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\meshes.db0"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\shaders.db0"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\sounds.db0"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\spawns.db0"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\textures.db0"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\levels.db1"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\sounds.db1"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\textures.db1"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\textures.db2"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\textures.db3"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\textures.db4"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\textures.db5"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\textures.db6"; DestDir: "{app}\gamedata"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\levels\*"; DestDir: "{app}\gamedata\levels"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\mods\*"; DestDir: "{app}\mods"; Components: main; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata.db0"; DestDir: "{app}"; Components: main; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata.db1"; DestDir: "{app}"; Components: main; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\fsgame.ltx"; DestDir: "{app}"; Components: main; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\readme.nfo"; DestDir: "{app}"; Components: main; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\manual.pdf"; DestDir: "{app}"; Components: main; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\1.4005_changelog_EN.txt"; DestDir: "{app}"; Components: main; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\1.4005_changelog_RU.txt"; DestDir: "{app}"; Components: main; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\readme.txt"; DestDir: "{app}"; Components: main; Flags: ignoreversion skipifsourcedoesntexist
;Source: "{#LA_game_files}\*"; DestDir: "{app}"; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist

Source: "{#LA_game_files}\3rdparties\oalinst.exe"; DestDir: "{app}\3rdparties"; Components: main; Flags: skipifsourcedoesntexist ignoreversion
Source: "{#LA_game_files}\3rdparties\vcredist_x86.exe"; DestDir: "{app}\3rdparties"; Components: main; Flags: skipifsourcedoesntexist ignoreversion
Source: "{#LA_game_files}\3rdparties\Xvid-1.3.2-20110601.exe"; DestDir: "{app}\3rdparties"; Components: main; Flags: skipifsourcedoesntexist ignoreversion
Source: "{#LA_game_files}\3rdparties\DirectX\*"; DestDir: "{app}\3rdparties\DirectX"; Components: main; Flags: skipifsourcedoesntexist ignoreversion createallsubdirs recursesubdirs
#endif

[Types]
Name: full; Description: Full installation
Name: compact; Description: Compact installation
Name: custom; Description: Custom installation; Flags: iscustom

[Components]
Name: main; Description: Lost Alpha Developer's Cut 1.4005; Types: full compact custom; Flags: fixed;
Name: sidor; Description: Oldstyle Sidorovich visual(low quality); Types: full custom;

[Run]
; unpack game files
#ifdef BundleRelease
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}\appdata"" ""{src}\game\appdata.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingAppdata}"; StatusMsg: "{cm:msgInstallingAppdata}"
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}\bins"" ""{src}\game\bins.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingBins}"; StatusMsg: "{cm:msgInstallingBins}"
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}\gamedata"" ""{src}\game\gamedata.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingGamedata}"; StatusMsg: "{cm:msgInstallingGamedata}"
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}"" ""{src}\game\maindir.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingMaindir}"; StatusMsg: "{cm:msgInstallingMaindir}"
#endif

; install prerequisities
Filename: "{app}\3rdparties\vcredist_x86.exe"; Flags: hidewizard skipifdoesntexist; Description: "{cm:msgInstallingVcredist}"; StatusMsg: "{cm:msgInstallingVcredist}"; Check: VCRedistNeedsInstall
Filename: "{app}\3rdparties\oalinst.exe"; Flags: hidewizard skipifdoesntexist; Description: "{cm:msgInstallingOAL}"; StatusMsg: "{cm:msgInstallingOAL}"
Filename: "{app}\3rdparties\Xvid-1.3.2-20110601.exe"; Flags: hidewizard skipifdoesntexist; Description: "{cm:msgInstallingXvid}"; StatusMsg: "{cm:msgInstallingXvid}"
; include the unpacked version of DirectX runtimes:
Filename: "{app}\3rdparties\DirectX\DXSETUP.exe"; Flags: hidewizard skipifdoesntexist; Description: "{cm:msgInstallingDXredist}"; StatusMsg: "{cm:msgInstallingDXredist}"

[InstallDelete]
Type: files; Name: "{app}\bins\BugTrap.dll";
Type: files; Name: "{app}\bins\ode.dll";
Type: files; Name: "{app}\bins\OpenAL32.dll";
Type: files; Name: "{app}\bins\xrAPI.dll";
Type: files; Name: "{app}\bins\xrCDB.dll";
Type: files; Name: "{app}\bins\xrCore.dll";
Type: files; Name: "{app}\bins\xrCPU_Pipe.dll";
Type: files; Name: "{app}\bins\xrGame.dll";
Type: files; Name: "{app}\bins\xrGameSpy.dll";
Type: files; Name: "{app}\bins\xrLua.dll";
Type: files; Name: "{app}\bins\xrNetServer.dll";
Type: files; Name: "{app}\bins\xrParticles.dll";
Type: files; Name: "{app}\bins\xrRender_R1.dll";
Type: files; Name: "{app}\bins\xrRender_R2.dll";
Type: files; Name: "{app}\bins\xrRender_R3.dll";
Type: files; Name: "{app}\bins\xrRender_R4.dll";
Type: files; Name: "{app}\bins\xrSound.dll";
Type: files; Name: "{app}\bins\xrXMLParser.dll";
Type: files; Name: "{app}\bins\XR_3DA.exe";

Type: files; Name: "{app}\gamedata.db0";
Type: files; Name: "{app}\gamedata.db1";
Type: files; Name: "{app}\gamedata.db2";
Type: files; Name: "{app}\gamedata.db3";
Type: files; Name: "{app}\gamedata.db4";
Type: files; Name: "{app}\gamedata.db5";
Type: files; Name: "{app}\gamedata.db6";
Type: files; Name: "{app}\gamedata.db7";
Type: files; Name: "{app}\gamedata.db8";
Type: files; Name: "{app}\gamedata.db9";
Type: files; Name: "{app}\gamedata.dba";
Type: files; Name: "{app}\gamedata.dbb";
Type: files; Name: "{app}\gamedata.dbc";
Type: files; Name: "{app}\gamedata.dbd";
Type: files; Name: "{app}\gamedata.dbe";
Type: files; Name: "{app}\gamedata.dbf";
Type: files; Name: "{app}\gamedata.dbg";
Type: files; Name: "{app}\gamedata.dbh";
Type: files; Name: "{app}\gamedata.dbj";
Type: files; Name: "{app}\gamedata.dbk";
Type: files; Name: "{app}\gamedata.dbl";
Type: files; Name: "{app}\gamedata.dbm";
Type: files; Name: "{app}\gamedata.dbn";
Type: files; Name: "{app}\gamedata.dbo";
Type: files; Name: "{app}\gamedata.dbp";
Type: files; Name: "{app}\gamedata.dbq";
Type: files; Name: "{app}\gamedata.dbr";
Type: files; Name: "{app}\gamedata.dbs";
Type: files; Name: "{app}\gamedata.dbt";
Type: files; Name: "{app}\gamedata.dbu";

Type: files; Name: "{app}\fsgame.ltx";

Type: filesandordirs; Name: "{app}\gamedata";
Type: filesandordirs; Name: "{app}\appdata\shaders_cache";
Type: files; Name: "{app}\appdata\user.ltx";

[Icons]
Name: "{group}\{#LA_shortcut_name}"; Filename: "{app}\bins\XR_3DA.exe"; WorkingDir: "{app}"; Parameters: "-external -noprefetch"
Name: "{group}\Прочитать файл ReadMe"; Filename: "{app}\readme.txt"; WorkingDir: "{app}";
Name: "{group}\Руководство пользователя"; Filename: "{app}\manual.pdf"; WorkingDir: "{app}";
Name: "{group}\Удалить Lost Alpha DC"; Filename: "{uninstallexe}"; WorkingDir: "{app}";
Name: "{commondesktop}\{#LA_shortcut_name}"; Filename: "{app}\bins\XR_3DA.exe"; WorkingDir: "{app}"; Parameters: "-external -noprefetch"

[Registry]
Root: HKLM; Subkey: "SOFTWARE\GSC Game World"; Flags: createvalueifdoesntexist uninsdeletekeyifempty
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; Flags: createvalueifdoesntexist uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC\ver1.4005"; Flags: createvalueifdoesntexist uninsdeletekey;
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC\ver1.4005-0.0"; Flags: createvalueifdoesntexist uninsdeletekey;
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"
; Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: string; ValueName: "InstallDrive"; ValueData: "{src}"
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: string; ValueName: "InstallLang"; ValueData: "Ru"
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: string; ValueName: "InstallSource"; ValueData: "DEZOWAVE"
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: string; ValueName: "InstallVers"; ValueData: "1.4005"
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: string; ValueName: "InstallCDKEY"; ValueData: ""
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: string; ValueName: "InstallUserName"; ValueData: "admin"
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: dword; ValueName: "InstallPatchID"; ValueData: 0000
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: string; ValueName: "UninstallString"; ValueData: "{uninstallexe}"

[Setup]
DiskSpanning=yes
SlicesPerDisk=6 
DiskSliceSize=1566000000
DiskClusterSize=4096
ReserveBytes=0
PrivilegesRequired=admin
#ifdef BundleRelease
;ExtraDiskSpaceRequired={#LA_disk_usage}
#endif
AppName={#LA_app_name}
AppVersion={#LA_version_text}
AppCopyright={#LA_copyright}
DefaultDirName={pf}\{#LA_directory_name}
DisableProgramGroupPage=auto
DefaultGroupName={#LA_StartMenu_directory_name}
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
rus.Wait=Подождите, идет проверка хеш-сумм файлов...
rus.Check=Проверка MD5 — «%1» — %2
rus.Check1=Проверка MD5
rus.Close=Вы действительно хотите пропустить проверку MD5?
rus.Close1=Отмена проверки MD5
rus.Error1=Хеш-сумма файла — «%1» — не совпадает!
rus.Yes=Да
rus.No=Нет
rus.MB=мб
rus.GB=гб
rus.TB=тб
rus.Comparing=Сравнение хеш-суммы файла: «%1» (%2)
rus.Skip=Пропустить
rus.Foundfiles=Проверяется файл: %1 из %2
rus.GSize=Проверено: %1 из %2
rus.Next=Далее
rus.FilesNotFound=Не найдены файлы для проверки!
rus.CheckCompleteWa=Проверка хеш-сумм установочных файлов успешно завершена!\par Проверено файлов:  %1 из %2
rus.CheckCompleteWb=Проверка хеш-сумм установочных файлов завершена.\par Проверено файлов:  %1 из %2\par Хеш-суммы файлов:%3\par\cf1 не совпадают!
rus.CheckCompleteWc=Проверка хеш-сумм установочных файлов успешно завершена!%nПроверено файлов:  %1 из %2
rus.CheckCompleteWd=Проверка хеш-сумм установочных файлов завершена.%nПроверено файлов:  %1 из %2%nХеш-суммы файлов:%3%nне совпадают!%n%nВы можете на свой страх и риск продолжить установку.%nПродолжить установку?

eng.Wait=Wait, checking the hash-sum of files...
eng.Check=Checking the MD5 — «%1» — %2
eng.Check1=Checking the MD5
eng.Close=You really want to skip MD5 check?
eng.Close1=Canceling of check MD5
eng.Error1=Hash sum of file — «%1» — is incorrect!
eng.Yes=Yes
eng.No=No
eng.MB=mb
eng.GB=gb
eng.TB=tb
eng.Comparing=Comparing file hash-sum: «%1» (%2)
eng.Skip=Skip
eng.FilesNotFound=Files for check aren't found!
eng.Foundfiles=Check files: %1 of %2
eng.GSize=Checked: %1 of %2
eng.Next=Next
eng.CheckCompleteWa=Check a hash sums of installation files is successfully complete!\par It is checked files:  %1 of %2
eng.CheckCompleteWb=Check a hash sums of installation files is complete.\par It is checked files:  %1 of %2\par Hash sums of files:%3\par\cf1 don't match!
eng.CheckCompleteWc=Check a hash sums of installation files is successfully complete!%nIt is checked files:  %1 of %2
eng.CheckCompleteWd=Check a hash sums of installation files is complete.%nIt is checked files:  %1 of %2%nHash sums of files:%3%ndon't match!%n%nYou can continue installation on your own risk.%nContinue installation?

[Languages]
Name: rus; MessagesFile: compiler:Languages\Russian.isl
Name: eng; MessagesFile: compiler:Languages\English.isl

[UninstallDelete]
; don't delete: appdata, screenshots, logs.
; maybe add some question if user wants to remove his player's data?
Type: filesandordirs; Name: "{app}\appdata"
Type: filesandordirs; Name: "{app}\bins"
Type: filesandordirs; Name: "{app}\gamedata"
Type: files; Name: "{app}\start.bat"
Type: files; Name: "{app}\gamedata.db*"
Type: files; Name: "{app}\fsgame.ltx"

[Code]
const
  ID_QUESTION = 65579;     // вопрос
  ID_HAND = 65581;         // ошибка

type
  TMD5Callback = function (Progress: Longword): Boolean;

var
  MD5Form,MyExit,MyError: TSetupForm;
  MD5PB: TNewProgressBar;
  Res,Total,CurN,i,n,CurPersent: Integer;
  CloseForm,Md5Error,Md5Pass,CanClose,FlagFinish,ClickDown: boolean;
  OkButton, CancelButton,Md5CancelButton: TButton;
  Ico: TNewIconImage;
  CheckMD5Label,CheckMD5Label2,CheckMD5Label3,CheckMD5Label4: TLabel;
  CurFilename,FilePath,DirMd5,g: String;
  TotalProgress: Longword;
  CurSize,TSize: Extended;
	strArrMd5,strArrHach: TArrayOfString;
	FilesMemo: TRichEditViewer;
	Sender: TObject;
	StringOfRTF,f,k,t,j,S,H,l,m: AnsiString;

function GetValue(strFilename,keyFind: string): string;
begin
  LoadStringsFromFile(strFilename, strArrMd5);
	LoadStringsFromFile(strFilename, strArrHach);
  for i:= 0 to GetArrayLength(strArrMd5)-1 do begin
    if Pos(keyFind,strArrMd5[i])>0 then begin
      Delete(strArrMd5[i],1,Pos(keyFind,strArrMd5[i])+Length(keyFind)-1);
			StringChange(strArrMd5[i],'/','\');
		end;
		if Pos(keyFind,strArrHach[i])>0 then
			Delete(strArrHach[i],Pos(' ',strArrHach[i]),Length(strArrHach[i]));
	end;
end;

procedure FileMd5Find(RootDir: String);
var
	FindRec: TFindRec;
begin
	if FindFirst(RootDir+'*.md5',FindRec) then begin  // здесь указываем с каким расширением искать файлы
		try
		repeat
			FilePath:= RootDir + FindRec.Name;
		until NOT FindNext(FindRec);
		finally
			FindClose(FindRec);
		end;
	end;
end;

function NumToStr1(Float: Extended): String;
  Begin
    Result:= Format('%.2n', [Float]);
    StringChange(Result, ',', '.');
  while ((Result[Length(Result)] = '0') or (Result[Length(Result)] = '.')) and (Pos('.', Result) > 0) do
    SetLength(Result, Length(Result)-1);
end;

function MbOrTb1(Byte: Extended): String;
begin
  if Byte < 1024 then Result:= NumToStr1(Byte)+ExpandConstant(' {cm:MB}') else
    if Byte/1024 < 1024 then Result:= NumToStr1((Byte/1024*100)/100)+ExpandConstant(' {cm:GB}') else
      Result:= NumToStr1(((Byte/(1024*1024))*100)/100)+ExpandConstant(' {cm:TB}');
end;

function CheckMD5(Filename: PAnsiChar; MD5: PAnsiChar; Callback: TMD5Callback): Boolean; external 'CheckMD5_A@files:ISMD5.dll stdcall';

function MD5Progress(Progress: Longword): Boolean;
begin
	MD5PB.Position:= TotalProgress+round(Progress*CurSize div TSize);
	CurPersent:= round(Progress*CurSize div CurSize);
	CheckMD5Label.Caption:= FmtMessage(ExpandConstant('{cm:Comparing}'), [CurFilename, MbOrTb1(CurSize)]);
	CheckMD5Label2.Caption:= FmtMessage(ExpandConstant('{cm:Foundfiles}'), [IntToStr(CurN), IntToStr(Total)]);
  CheckMD5Label3.Caption:= FmtMessage('%1.%2 %', [IntToStr(MD5PB.Position div 10), chr(48 + MD5PB.Position mod 10)]);
  CheckMD5Label4.Caption:= FmtMessage(ExpandConstant('{cm:GSize}'), [MbOrTb1(round(TSize)*MD5PB.Position div 1000), MbOrTb1(round(TSize))]);
	j:= FmtMessage('%1.%2%', [IntToStr(CurPersent div 10), chr(48 + CurPersent mod 10)]);
  MD5Form.Caption:= FmtMessage(ExpandConstant('{cm:Check}'), [CurFilename, CheckMD5Label3.Caption]);
  Application.ProcessMessages;
	if CloseForm then
		Result:= false
	else
		Result:= true;

	if FileExists(DirMd5+strArrMd5[n]) then begin
		g:= strArrMd5[n];
		StringChange(g,'\','\\');
		S:= '\cf3 '+Format('\\%s%s', [g, '\cf1\tab   ... '+j]);
		with FilesMemo do begin
			if not CloseForm then begin
				RTFText:= StringOfRTF+'{\pard\tx3900\fs16'+t+S+'\par}}';
				if ClickDown then SelStart:= Length(Lines.Text);
			end;
		end;
	end;
end;

function Size32(Ny, Lu: Integer): Extended;
begin
  Result:= Lu;
  if Lu<0 then Result:= Result + $7FFFFFFF + $7FFFFFFF + 2;
  for Ny:= Ny-1 Downto 0 do
  Result:= Result + $7FFFFFFF + $7FFFFFFF + 2;
end;

function GetFileSize(const FileName: string): Extended;
var
  FSR: TFindRec;
begin
  Result:= 0;
  if FindFirst(FileName, FSR) then
    try
      if FSR.Attributes and FILE_ATTRIBUTE_DIRECTORY = 0 then
        Result:= Size32(FSR.SizeHigh, FSR.SizeLow) div 1048576;
    finally
      FindClose(FSR);
    end;
end;

function SizeAndTotal(const FileName: string): Extended;
var
  Size: Extended;
begin
	if (FileExists(FileName)) and (GetFileSize(FileName) <> 0) then begin
		Size:= GetFileSize(FileName);
		TSize:= TSize+Size;
		Total:= Total+1;
	end;
end;

function CheckMD(Filename, MD5: String): Boolean;
begin
  TotalProgress:= MD5PB.Position;
  CurFilename:= ExtractFilename(FileName);
	CurN:= CurN+1;
	CurSize:= GetFileSize(FileName);
	Result:= CheckMD5(Filename, MD5, @MD5Progress);
end;

procedure MD5FormClose(Sender: TObject; var CanClose: Boolean);
begin
	MD5Form.Hide;
  CanClose:= false;

  MyExit:= CreateCustomForm();
  with MyExit do begin
    ClientWidth:= ScaleX(360);
    ClientHeight:= ScaleY(150);
    Caption:= ExpandConstant('{cm:Close1}');

    Ico:= TNewIconImage.Create(MyExit);
    with Ico do begin
      Parent:= MyExit;
      Left:= ScaleX(20);
      Top:= ScaleY(40);
      Icon.Handle:= ID_QUESTION;
    end;

    with TBevel.Create(MyExit) do begin
      SetBounds(ScaleX(0),ScaleY(105),MyExit.Width,ScaleY(2));
      Parent:= MyExit;
    end;

    with TNewStaticText.Create(MyExit) do begin
      Left:= ScaleX(65);
      Top:= ScaleY(45);
      Width:= ScaleX(278);
      Height:= ScaleY(60);
      AutoSize:= False;
      WordWrap:= True;
      Caption:= ExpandConstant('{cm:Close}');
      Parent:= MyExit;
    end;

    CancelButton:= TButton.Create(MyExit);
    with CancelButton do begin
      Width:= ScaleX(75);
      Height:= ScaleY(23);
      Left:= MyExit.Width - Width - ScaleX(15);
      Top:= MyExit.Height - Height * 2 - ScaleY(13);
      Caption:= ExpandConstant('{cm:No}');
      ModalResult:= mrCancel;
      Parent:= MyExit;
    end;

    OkButton:= TButton.Create(MyExit);
    with OkButton do begin
      Width:= CancelButton.Width;
      Height:= CancelButton.Height;
      Left:= CancelButton.Left - Width - ScaleX(5);
      Top:= CancelButton.Top;
      Caption:= ExpandConstant('{cm:Yes}');
      ModalResult:= mrOk;
      Parent:= MyExit;
    end;

    ActiveControl:= CancelButton;
    Center;
  end;
  if MyExit.ShowModal() = mrCancel then begin
    CloseForm:= false;
		MD5Form.Show;
  end else begin
    CloseForm:= true;
		CanClose:= true;
  end;
end;

procedure CBClick(Sender: TObject);
begin
  MD5FormClose(Sender,CanClose);
end;

procedure FMOnclick(Sender: TObject);
begin
	ClickDown:= true;
	if not CloseForm then
		with FilesMemo do begin
			if not FlagFinish then
				Lines.add('')
			else begin
				SelStart:= Length(Lines.Text);
				SetFocus;
			end;
		end;
		Md5CancelButton.SetFocus;
end;

function InitializeSetup(): Boolean;
begin
	DirMd5:= ExpandConstant('{src}\');
	FileMd5Find(DirMd5);
	GetValue(FilePath,' *');

	StringOfRTF:= '{\rtf1\ansi\ansicpg1251{\colortbl;\red0\green0\blue0;\red255\green0\blue0;\red0\green0\blue255;\red0\green180\blue0;}';

	for i:= 0 to GetArrayLength(strArrMd5)-1 do SizeAndTotal(DirMd5+strArrMd5[i]);

  MD5Form:= CreateCustomForm();
  with MD5Form do begin
    ClientWidth:= ScaleX(360);
    ClientHeight:= ScaleY(225);
    OnCloseQuery:= @MD5FormClose;
    Center;

		with TLabel.Create(MD5Form) do begin
			SetBounds(ScaleX(5),ScaleY(5),ScaleX(350),ScaleY(15));
			Caption:= ExpandConstant('{cm:Wait}');
			Transparent:= True;
			Parent:= MD5Form;
		end;
		CheckMD5Label:= TLabel.Create(MD5Form);
		with CheckMD5Label do begin
			SetBounds(ScaleX(5),ScaleY(25),ScaleX(350),ScaleY(15));
			Transparent:= True;
			Parent:= MD5Form;
		end;
		CheckMD5Label2:= TLabel.Create(MD5Form);
		with CheckMD5Label2 do begin
			SetBounds(ScaleX(5),ScaleY(65),ScaleX(300),ScaleY(15));
			Transparent:= True;
			Parent:= MD5Form;
		end;
		CheckMD5Label3:= TLabel.Create(MD5Form);
		with CheckMD5Label3 do begin
			SetBounds(ScaleX(240),ScaleY(68),ScaleX(50),ScaleY(15));
			Transparent:= True;
			Font.Size:= 18;
			Enabled:= false;
			Parent:= MD5Form;
		end;
		CheckMD5Label4:= TLabel.Create(MD5Form);
		with CheckMD5Label4 do begin
			SetBounds(ScaleX(5),ScaleY(85),ScaleX(300),ScaleY(15));
			Transparent:= True;
			Parent:= MD5Form;
		end;

		MD5PB:= TNewProgressBar.Create(MD5Form);
		with MD5PB do begin
			Min:= 0;
			Max:= 1000;
			SetBounds(ScaleX(5),ScaleY(45),ScaleX(350),ScaleY(15));
			Parent:= MD5Form;
		end;

		with TBevel.Create(MD5Form) do begin
			SetBounds(ScaleX(0),ScaleY(105),MD5Form.Width,ScaleY(2));
			Parent:= MD5Form;
		end;

		FilesMemo:= TRichEditViewer.Create(MD5Form);
		with FilesMemo do begin
			SetBounds(ScaleX(10), ScaleY(115), ScaleX(340), ScaleY(70));
			Parent:= MD5Form;
			ParentColor:= True;
			ScrollBars:= ssVertical;
			ReadOnly:= true;
			OnClick:= @FMOnclick;
#ifdef UNICODE
			DoubleBuffered:= false;
#endif
		end;

		MD5Form.Show;
		Application.Title:= ExpandConstant('{cm:Check1}');

		Md5CancelButton:= TButton.Create(MD5Form);
		with Md5CancelButton do begin
			Width:= ScaleX(75);
			Height:= ScaleY(23);
			Left:= MD5Form.Width - Width - ScaleX(15);
			Top:= MD5Form.Height - Height * 2 - ScaleY(13);
			Caption:= ExpandConstant('{cm:Skip}');
			OnClick:= @CBClick;
			Parent:= MD5Form;
		end;

		for n:= 0 to GetArrayLength(strArrMd5)-1 do begin
			if CloseForm then Exit;
			repeat if not FileExists(DirMd5+strArrMd5[n]) and (n < (GetArrayLength(strArrMd5)-1)) then n:= n+1;
			until (FileExists(DirMd5+strArrMd5[n]) or (n = (GetArrayLength(strArrMd5)-1)));
			repeat if (n < (GetArrayLength(strArrMd5)-1)) and (GetFileSize(DirMd5+strArrMd5[n]) = 0) then n:= n+1;
			until (GetFileSize(DirMd5+strArrMd5[n]) <> 0) or (n = (GetArrayLength(strArrMd5)-1));
			if (FileExists(DirMd5+strArrMd5[n]) and (GetFileSize(DirMd5+strArrMd5[n]) > 0)) and not CloseForm then begin
				with FilesMemo do begin
					Lines.add('');
					SelStart:= Length(Lines.Text);
					SetFocus;
					Md5CancelButton.SetFocus;
				end;
				if CheckMD(DirMd5+strArrMd5[n],strArrHach[n]) then begin
					t:= t+'\cf4 '+Format('\\%s%s', [g, '\tx3900\tab   ......  Ok!'])+'\line'+#13#10;
				end else begin
					t:= t+'\cf2 '+Format('\\%s%s', [g, '\tx3900\tab   ...... Fail!'])+'\line'+#13#10;
					k:= k+'\par'+'  '+'\cf2'+CurFilename;
					m:= m+#10#13+' '+CurFilename;
					Md5Error:= true;
				end;
				with FilesMemo do
					if not CloseForm then
						RTFText:= StringOfRTF+'{\pard\tx3900\fs16'+t+'\par}}';
			end;
		end;

		if not CloseForm and not MD5Error and (CurN = 0) then begin
			FlagFinish:= true;
			Md5CancelButton.Caption:= ExpandConstant('{cm:Next}');
			FilesMemo.RTFText:= StringOfRTF+'\pard\qc\line\fs28\cf2\qc '+ExpandConstant('{cm:FilesNotFound}')+'}';
			if MsgBox(ExpandConstant('{cm:FilesNotFound}'), mbInformation, MB_OK) = IDOK then
				result:= false;
		end;

		if not CloseForm and not MD5Error and (CurN <> 0) then begin
			FlagFinish:= true;
			Md5CancelButton.Caption:= ExpandConstant('{cm:Next}');
			with FilesMemo do
			if not CloseForm then begin
				RTFText:= StringOfRTF+'{\pard\tx3900\fs16'+t+'\par}'+FmtMessage(ExpandConstant('{cm:CheckCompleteWa}'), [IntToStr(CurN), IntToStr(Total)])+'}';
			end;
			if MsgBox(FmtMessage(ExpandConstant('{cm:CheckCompleteWc}'), [IntToStr(CurN), IntToStr(Total)]), mbInformation, MB_OK) = IDOK then begin
				result:= true;
			end;
		end;

		if not CloseForm and MD5Error then begin
			FlagFinish:= true;
			Md5CancelButton.Caption:= ExpandConstant('{cm:Next}');
			with FilesMemo do
			if not CloseForm then begin
				RTFText:= StringOfRTF+'{\pard\tx3900\fs16'+t+'\par}'+FmtMessage(ExpandConstant('{cm:CheckCompleteWb}'), [IntToStr(CurN), IntToStr(Total), '{\pard\tx200\fs16\cf2\ql'+k+'\par}'])+'}';
			end;
			if MsgBox(FmtMessage(ExpandConstant('{cm:CheckCompleteWd}'), [IntToStr(CurN), IntToStr(Total), m]), mbInformation, MB_YESNO) = IDYES then
				result:= true
			else
				result:= false;
		end;

		if CloseForm then result:= true;

		ClickDown:= false;
	end;
	MD5Form.Free;
end;
#define A = (Defined UNICODE) ? "W" : "A"

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
    DelTree(ExpandConstant('{app}\Mods'), True, True, True);
    DeleteFile(ExpandConstant('{app}\gamedata.dbb'));
  end;
  if CurStep = ssPostInstall then begin
    //WipeUserLtx;
    //CreateFsGameLtx(ExpandConstant('{userdocs}'));
    //ModifyUserLtx('r2_gloss_factor', '1.');
  end;
end;
