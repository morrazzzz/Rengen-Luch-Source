object frmLog: TfrmLog
  Left = 615
  Top = 906
  Width = 400
  Height = 225
  BorderStyle = bsSizeToolWin
  Caption = 'Log'
  Color = 2894892
  Constraints.MinHeight = 80
  Constraints.MinWidth = 400
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  KeyPreview = True
  OldCreateOrder = False
  Scaled = False
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 167
    Width = 384
    Height = 20
    Align = alBottom
    BevelOuter = bvNone
    Color = 6186823
    TabOrder = 0
    object ebClear: TExtBtn
      Left = 197
      Top = 1
      Width = 82
      Height = 18
      Align = alNone
      BevelShow = False
      BtnColor = 6186836
      Caption = 'Clear'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebClearClick
    end
    object ebClearSelected: TExtBtn
      Left = 283
      Top = 1
      Width = 82
      Height = 18
      Align = alNone
      BevelShow = False
      BtnColor = 6186836
      Caption = 'Clear Selected'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebClearSelectedClick
    end
    object ebClose: TExtBtn
      Left = 13
      Top = 1
      Width = 82
      Height = 18
      Align = alNone
      BevelShow = False
      BtnColor = 6186836
      Caption = 'Close'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebCloseClick
    end
    object ebFlush: TExtBtn
      Left = 99
      Top = 1
      Width = 82
      Height = 18
      Align = alNone
      BevelShow = False
      BtnColor = 6186836
      Caption = 'Flush'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      Transparent = False
      FlatAlwaysEdge = True
      OnClick = ebFlushClick
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 0
    Width = 384
    Height = 167
    Align = alClient
    BevelOuter = bvNone
    TabOrder = 1
    object lbLog: TListBox
      Left = 0
      Top = 0
      Width = 384
      Height = 167
      Style = lbOwnerDrawFixed
      AutoComplete = False
      Align = alClient
      BevelInner = bvNone
      BevelOuter = bvNone
      BorderStyle = bsNone
      Color = 4543544
      ItemHeight = 13
      MultiSelect = True
      PopupMenu = MxPopupMenu1
      TabOrder = 0
      OnDrawItem = lbLogDrawItem
      OnKeyDown = lbLogKeyDown
      OnKeyPress = lbLogKeyPress
    end
  end
  object fsStorage: TFormStorage
    IniSection = 'Log Form'
    Version = 1
    StoredValues = <>
    Left = 8
    Top = 8
  end
  object MxPopupMenu1: TMxPopupMenu
    MarginStartColor = 2960685
    MarginEndColor = 2960685
    BKColor = 3684408
    SelColor = 12171705
    SelFontColor = 0
    FontColor = clWindow
    SepHColor = 3618615
    SepLColor = 4802889
    LeftMargin = 10
    Left = 40
    Top = 8
    object imCopy: TMenuItem
      Caption = '&Copy'
      ShortCut = 16451
      OnClick = imCopyClick
    end
    object imSelectAll: TMenuItem
      Caption = 'Select &All'
      ShortCut = 16449
      OnClick = imSelectAllClick
    end
    object N1: TMenuItem
      Caption = '-'
    end
    object ClearAll1: TMenuItem
      Caption = 'Clear All'
      OnClick = ebClearClick
    end
    object ClearSelected1: TMenuItem
      Caption = 'Clear Selected'
      OnClick = ebClearSelectedClick
    end
  end
end
