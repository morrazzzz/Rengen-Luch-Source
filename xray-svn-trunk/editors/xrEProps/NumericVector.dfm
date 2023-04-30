object frmNumericVector: TfrmNumericVector
  Left = 1040
  Top = 392
  BorderIcons = [biSystemMenu]
  BorderStyle = bsToolWindow
  Caption = 'Numeric'
  ClientHeight = 57
  ClientWidth = 158
  Color = 6842472
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  KeyPreview = True
  OldCreateOrder = False
  Position = poDefault
  Scaled = False
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object RxLabel3: TLabel
    Left = 3
    Top = 3
    Width = 10
    Height = 13
    Caption = 'X:'
  end
  object RxLabel1: TLabel
    Left = 3
    Top = 21
    Width = 10
    Height = 13
    Caption = 'Y:'
  end
  object RxLabel2: TLabel
    Left = 3
    Top = 39
    Width = 10
    Height = 13
    Caption = 'Z:'
  end
  object Bevel1: TBevel
    Left = 14
    Top = 2
    Width = 84
    Height = 16
  end
  object Bevel2: TBevel
    Left = 14
    Top = 20
    Width = 84
    Height = 16
  end
  object Bevel3: TBevel
    Left = 14
    Top = 38
    Width = 84
    Height = 16
  end
  object paBottom: TPanel
    Left = 100
    Top = 0
    Width = 58
    Height = 57
    Align = alRight
    BevelOuter = bvNone
    Color = 6842472
    TabOrder = 0
    object ebOk: TExtBtn
      Left = 1
      Top = 21
      Width = 56
      Height = 17
      Align = alNone
      BevelShow = False
      BtnColor = 6842472
      Caption = 'Ok'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebOkClick
    end
    object ebCancel: TExtBtn
      Left = 1
      Top = 39
      Width = 56
      Height = 17
      Align = alNone
      BevelShow = False
      BtnColor = 6842472
      Caption = 'Cancel'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebCancelClick
    end
    object ebReset: TExtBtn
      Left = 1
      Top = 1
      Width = 56
      Height = 17
      Align = alNone
      BevelShow = False
      BtnColor = 6842472
      Caption = 'Reset'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebResetClick
    end
  end
  object seX: TMultiObjSpinEdit
    Left = 15
    Top = 4
    Width = 82
    Height = 13
    LWSensitivity = 0.1
    ButtonKind = bkLightWave
    Decimal = 4
    Increment = 0.01
    ValueType = vtFloat
    AutoSize = False
    BorderStyle = bsNone
    Color = 10526880
    TabOrder = 1
    OnChange = OnModified
  end
  object seY: TMultiObjSpinEdit
    Left = 15
    Top = 22
    Width = 82
    Height = 13
    LWSensitivity = 0.1
    ButtonKind = bkLightWave
    Decimal = 4
    Increment = 0.01
    ValueType = vtFloat
    AutoSize = False
    BorderStyle = bsNone
    Color = 10526880
    TabOrder = 2
    OnChange = OnModified
  end
  object seZ: TMultiObjSpinEdit
    Left = 15
    Top = 40
    Width = 82
    Height = 13
    LWSensitivity = 0.1
    ButtonKind = bkLightWave
    Decimal = 4
    Increment = 0.01
    ValueType = vtFloat
    AutoSize = False
    BorderStyle = bsNone
    Color = 10526880
    TabOrder = 3
    OnChange = OnModified
  end
end
