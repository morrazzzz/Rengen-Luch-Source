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
#define LA_disk_usage "15510000000"

; dirs used:
#define LA_game_files ".\game_distrib_files"
#define LA_3rd_party_files ".\game_distrib_files\3rdparties"
#define LA_installer_support_files "."

; versions, names etc.:
#define LA_shortcut_name "S.T.A.L.K.E.R. - Patch for Lost Alpha DC"
#define LA_app_name "S.T.A.L.K.E.R.: Patch for Lost Alpha DC"
#define LA_directory_name "S.T.A.L.K.E.R. - Lost Alpha DC"
#define LA_copyright "dezowave"
#define LA_version "1.4005"
#define LA_version_text "1.4005 Patch 0.2"

[Files]
Source: Include\ISMD5.dll; Flags: dontcopy;
Source: "{#LA_installer_support_files}\7za.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
Source: "{#LA_game_files}\lost_alpha_game_manual.pdf"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
;Source: "{#LA_installer_support_files}\fsgame_template.ltx"; DestDir: "{tmp}"; Flags: deleteafterinstall
#ifndef BundleRelease
Source: "{#LA_game_files}\appdata\*"; DestDir: "{app}\appdata"; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\bins\*"; DestDir: "{app}\bins"; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata\*"; DestDir: "{app}\gamedata"; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
Source: "{#LA_game_files}\gamedata.db*"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#LA_game_files}\*"; DestDir: "{app}"; Flags: ignoreversion createallsubdirs recursesubdirs skipifsourcedoesntexist
#endif

[Run]
; unpack game files
#ifdef BundleRelease
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}\appdata"" ""{src}\game\appdata.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingAppdata}"; StatusMsg: "{cm:msgInstallingAppdata}"
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}\bins"" ""{src}\game\bins.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingBins}"; StatusMsg: "{cm:msgInstallingBins}"
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}\gamedata"" ""{src}\game\gamedata.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingGamedata}"; StatusMsg: "{cm:msgInstallingGamedata}"
Filename: "{tmp}\7za.exe"; Parameters: "x -y -p{#archpasswd} -o""{app}"" ""{src}\game\maindir.7z*"""; Flags: runhidden; Description: "{cm:msgInstallingMaindir}"; StatusMsg: "{cm:msgInstallingMaindir}"
#endif

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
Type: files; Name: "{app}\gamedata\anims.db0";
Type: files; Name: "{app}\gamedata\meshes.db1";
Type: files; Name: "{app}\gamedata\textures.db6";
Type: files; Name: "{app}\gamedata.db0";

[Icons]
;Name: "{commonprograms}\{#LA_shortcut_name}"; Filename: "{app}\bins\XR_3DA.exe"; WorkingDir: "{app}"; Parameters: "-external -noprefetch"
;Name: "{commondesktop}\{#LA_shortcut_name}"; Filename: "{app}\bins\XR_3DA.exe"; WorkingDir: "{app}"; Parameters: "-external -noprefetch"

[Registry]
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC\ver1.4005-0.2"; Flags: createvalueifdoesntexist uninsdeletekey;
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: dword; ValueName: "InstallPatchID"; ValueData: 2222
Root: HKLM; Subkey: "SOFTWARE\GSC Game World\STALKER-LADC"; ValueType: string; ValueName: "UninstallString"; ValueData: "{uninstallexe}"

[Setup]
;AppendDefaultDirName=no
DiskSpanning=no
SlicesPerDisk=1 
DiskSliceSize=1566000000
DiskClusterSize=4096
ReserveBytes=0
PrivilegesRequired=admin
#ifdef BundleRelease
ExtraDiskSpaceRequired={#LA_disk_usage}
#endif
AppName={#LA_app_name}
AppVersion={#LA_version_text}
AppCopyright={#LA_copyright}
;DefaultDirName={pf}\{#LA_directory_name}
DefaultDirName={reg:HKLM\SOFTWARE\GSC Game World\STALKER-LADC,InstallPath}
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
;AllowNoIcons=yes
;UninstallLogMode=overwrite
DisableDirPage=yes
;;DisableStartupPrompt=yes
;;DisableReadyPage=yes
;;RestartIfNeededByRun=no
;;UsePreviousAppDir=no
;UsePreviousGroup=no
;;UsePreviousSetupType=no
;;DisableFinishedPage=yes

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

function NumToStr(Float: Extended): String;
  Begin
    Result:= Format('%.2n', [Float]);
    StringChange(Result, ',', '.');
  while ((Result[Length(Result)] = '0') or (Result[Length(Result)] = '.')) and (Pos('.', Result) > 0) do
    SetLength(Result, Length(Result)-1);
end;

function MbOrTb(Byte: Extended): String;
begin
  if Byte < 1024 then Result:= NumToStr(Byte)+ExpandConstant(' {cm:MB}') else
    if Byte/1024 < 1024 then Result:= NumToStr((Byte/1024*100)/100)+ExpandConstant(' {cm:GB}') else
      Result:= NumToStr(((Byte/(1024*1024))*100)/100)+ExpandConstant(' {cm:TB}');
end;

function CheckMD5(Filename: PAnsiChar; MD5: PAnsiChar; Callback: TMD5Callback): Boolean; external 'CheckMD5_A@files:ISMD5.dll stdcall';

function MD5Progress(Progress: Longword): Boolean;
begin
	MD5PB.Position:= TotalProgress+round(Progress*CurSize div TSize);
	CurPersent:= round(Progress*CurSize div CurSize);
	CheckMD5Label.Caption:= FmtMessage(ExpandConstant('{cm:Comparing}'), [CurFilename, MbOrTb(CurSize)]);
	CheckMD5Label2.Caption:= FmtMessage(ExpandConstant('{cm:Foundfiles}'), [IntToStr(CurN), IntToStr(Total)]);
  CheckMD5Label3.Caption:= FmtMessage('%1.%2 %', [IntToStr(MD5PB.Position div 10), chr(48 + MD5PB.Position mod 10)]);
  CheckMD5Label4.Caption:= FmtMessage(ExpandConstant('{cm:GSize}'), [MbOrTb(round(TSize)*MD5PB.Position div 1000), MbOrTb(round(TSize))]);
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

function Size64(Hi, Lo: Integer): Extended;
begin
  Result:= Lo;
  if Lo<0 then Result:= Result + $7FFFFFFF + $7FFFFFFF + 2;
  for Hi:= Hi-1 Downto 0 do
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
        Result:= Size64(FSR.SizeHigh, FSR.SizeLow) div 1048576;
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
        begin
Result:=True;
If (not RegKeyExists(HKLM, 'SOFTWARE\GSC Game World\STALKER-LADC\ver1.4005-0.1')) then
begin
MsgBox('Установленная полная версия игры S.T.A.L.K.E.R. - Lost Alpha DC 1.4005 Patch 0.1 не найдена.' #13#13 'Для установки патча Patch 0.2 следует установить ее.', mbError, mb_Ok);
Result:=False;
end;
		if CloseForm then result:= true;

		ClickDown:= false;
	end;
	MD5Form.Free;
end;
end;