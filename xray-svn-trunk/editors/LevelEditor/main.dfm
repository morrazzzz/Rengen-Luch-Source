object frmMain: TfrmMain
  Left = 1425
  Top = 539
  Width = 783
  Height = 452
  Color = clBtnFace
  Constraints.MinHeight = 446
  Constraints.MinWidth = 660
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  Position = poDefault
  Scaled = False
  OnClose = FormClose
  OnCloseQuery = FormCloseQuery
  OnCreate = FormCreate
  OnKeyDown = FormKeyDown
  OnMouseWheel = OnMouseWheel
  OnMouseWheelDown = OnMouseWheelDown
  OnMouseWheelUp = OnMouseWheelUp
  OnResize = FormResize
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object paLeftBar: TPanel
    Left = 522
    Top = 0
    Width = 245
    Height = 381
    Align = alRight
    BevelOuter = bvNone
    Color = 2960685
    TabOrder = 0
    object paTools: TPanel
      Left = 0
      Top = 0
      Width = 245
      Height = 20
      Align = alTop
      BevelOuter = bvNone
      Caption = 'Toolbar'
      Color = 6842472
      Constraints.MaxHeight = 20
      Constraints.MaxWidth = 240
      Constraints.MinHeight = 20
      Constraints.MinWidth = 240
      TabOrder = 0
      object sbToolsMin: TExtBtn
        Left = 224
        Top = 2
        Width = 17
        Height = 13
        Align = alNone
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          DE000000424DDE00000000000000360000002800000007000000070000000100
          180000000000A8000000120B0000120B00000000000000000000FFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFF000000FFFFFF000000FFFFFFFFFFFFFFFFFFFFFFFF0000
          00000000FFFFFF000000FFFFFFFFFFFFFFFFFF000000000000000000FFFFFF00
          0000FFFFFFFFFFFF000000000000000000000000FFFFFF000000FFFFFFFFFFFF
          FFFFFF000000000000000000FFFFFF000000FFFFFFFFFFFFFFFFFFFFFFFF0000
          00000000FFFFFF000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000FFFFFF00
          0000}
        ParentFont = False
        OnClick = sbToolsMinClick
      end
      object ebAllMin: TExtBtn
        Left = 3
        Top = 4
        Width = 11
        Height = 11
        Align = alNone
        BtnColor = 2960685
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          AE000000424DAE00000000000000360000002800000006000000060000000100
          18000000000078000000120B0000120B000000000000000000002B2B2B2B2B2B
          2B2B2B2B2B2B2B2B2B2B2B2B2BFF2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B
          2BFF6B6B6B6B6B6B6B6B6B6B6B6B6B6B6B6B6B6B6BFF6B6B6B6B6B6B6B6B6B6B
          6B6B6B6B6B6B6B6B6BFF2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2BFF2B2B
          2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2B2BFF}
        ParentFont = False
        Transparent = False
        OnClick = ebAllMinClick
      end
      object ebAllMax: TExtBtn
        Left = 15
        Top = 4
        Width = 11
        Height = 11
        Align = alNone
        BtnColor = 2960685
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          AE000000424DAE00000000000000360000002800000006000000060000000100
          18000000000078000000120B0000120B000000000000000000002B2B2B2B2B2B
          6B6B6B6B6B6B2B2B2B2B2B2B2BFF2B2B2B2B2B2B6B6B6B6B6B6B2B2B2B2B2B2B
          2BFF6B6B6B6B6B6B6B6B6B6B6B6B6B6B6B6B6B6B6BFF6B6B6B6B6B6B6B6B6B6B
          6B6B6B6B6B6B6B6B6BFF2B2B2B2B2B2B6B6B6B6B6B6B2B2B2B2B2B2B6BFF2B2B
          2B2B2B2B6B6B6B6B6B6B2B2B2B2B2B2B2BFF}
        ParentFont = False
        Transparent = False
        OnClick = ebAllMaxClick
      end
    end
  end
  object paBottomBar: TPanel
    Left = 0
    Top = 381
    Width = 767
    Height = 33
    Align = alBottom
    BevelOuter = bvNone
    Color = 2960685
    TabOrder = 1
  end
  object paMain: TPanel
    Left = 0
    Top = 0
    Width = 522
    Height = 381
    Align = alClient
    BevelOuter = bvNone
    TabOrder = 2
    object paTopBar: TPanel
      Left = 0
      Top = 0
      Width = 522
      Height = 33
      Align = alTop
      BevelOuter = bvNone
      Color = 2960685
      TabOrder = 0
    end
    object paRender: TPanel
      Left = 0
      Top = 33
      Width = 522
      Height = 348
      Align = alClient
      BevelOuter = bvNone
      Color = 4868682
      TabOrder = 1
      OnResize = paRenderResize
      object D3DWindow: TD3DWindow
        Left = 4
        Top = 4
        Width = 516
        Height = 367
        FocusedColor = 329033
        UnfocusedColor = 657930
        OnChangeFocus = D3DWindowChangeFocus
        BorderStyle = bsNone
        Color = 5592405
        TabOrder = 0
        OnKeyDown = D3DWindowKeyDown
        OnKeyPress = D3DWindowKeyPress
        OnKeyUp = D3DWindowKeyUp
        OnMouseDown = D3DWindowMouseDown
        OnMouseMove = D3DWindowMouseMove
        OnMouseUp = D3DWindowMouseUp
        OnResize = D3DWindowResize
        OnPaint = D3DWindowPaint
      end
    end
  end
  object fsStorage: TFormStorage
    IniSection = 'Main Form'
    Options = [fpPosition]
    RegistryRoot = prLocalMachine
    Version = 1
    OnSavePlacement = fsStorageSavePlacement
    StoredProps.Strings = (
      'paLeftBar.Tag')
    StoredValues = <>
    Left = 161
    Top = 33
  end
  object tmRefresh: TTimer
    Enabled = False
    OnTimer = tmRefreshTimer
    Left = 129
    Top = 33
  end
end
