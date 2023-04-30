object frmDOShuffle: TfrmDOShuffle
  Left = 1170
  Top = 786
  Width = 512
  Height = 534
  BiDiMode = bdLeftToRight
  BorderIcons = [biSystemMenu]
  Caption = 'Detail objects'
  Color = 5131854
  Constraints.MaxWidth = 1024
  Constraints.MinHeight = 512
  Constraints.MinWidth = 512
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  KeyPreview = True
  OldCreateOrder = False
  ParentBiDiMode = False
  OnClose = FormClose
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object paTools: TPanel
    Left = 219
    Top = 0
    Width = 277
    Height = 496
    Align = alRight
    BevelOuter = bvNone
    ParentColor = True
    TabOrder = 0
    object Panel3: TPanel
      Left = 0
      Top = 0
      Width = 277
      Height = 19
      Align = alTop
      BevelOuter = bvNone
      ParentColor = True
      TabOrder = 0
      object ebAppendIndex: TExtBtn
        Left = 32
        Top = 0
        Width = 243
        Height = 18
        Align = alNone
        BevelShow = False
        BtnColor = 6842472
        Caption = 'Append Color Index'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebAppendIndexClick
      end
      object ebMultiClear: TExtBtn
        Left = 4
        Top = 0
        Width = 26
        Height = 18
        Align = alNone
        BtnColor = 6842472
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          C2010000424DC20100000000000036000000280000000C0000000B0000000100
          1800000000008C010000120B0000120B00000000000000000000FFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFF000000000000FF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFF000000000000
          FFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFF0000000000000000000000
          00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFF00000000
          0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000
          000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0000
          00000000FFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00
          0000000000000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFF
          000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FF000000000000FFFFFFFFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFF}
        ParentFont = False
        Spacing = 0
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebMultiClearClick
      end
    end
    object sbDO: TScrollBox
      Left = 0
      Top = 19
      Width = 277
      Height = 477
      VertScrollBar.ButtonSize = 11
      VertScrollBar.Increment = 10
      VertScrollBar.Size = 11
      VertScrollBar.Style = ssHotTrack
      VertScrollBar.Tracking = True
      Align = alClient
      BorderStyle = bsNone
      TabOrder = 1
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 0
    Width = 219
    Height = 496
    Align = alClient
    BevelOuter = bvNone
    ParentColor = True
    TabOrder = 1
    object Splitter1: TSplitter
      Left = 0
      Top = 241
      Width = 219
      Height = 3
      Cursor = crVSplit
      Align = alTop
      Beveled = True
      ResizeStyle = rsUpdate
    end
    object tvItems: TElTree
      Left = 0
      Top = 244
      Width = 219
      Height = 233
      Cursor = crDefault
      LeftPosition = 0
      DragCursor = crDrag
      Align = alClient
      AlwaysKeepSelection = False
      AutoExpand = True
      DockOrientation = doNoOrient
      DefaultSectionWidth = 120
      BorderSides = [ebsLeft, ebsRight, ebsTop, ebsBottom]
      Color = 5592405
      DragAllowed = True
      DrawFocusRect = False
      ExplorerEditMode = False
      Flat = True
      FocusedSelectColor = 12171705
      FocusedSelectTextColor = clBlack
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      FullRowSelect = False
      HeaderHeight = 19
      HeaderHotTrack = False
      HeaderSections.Data = {
        F4FFFFFF0700000084901F1200000000FFFFFFFF000001010100000078000000
        000000001027000000010061401C7A0300000000000000000000000000000100
        000000000000000000010000000000000F000000656243616D65726141726342
        616C6C00F01B0410F01B041030000000656243616D65726141726342616C6C00
        10ADD31110ADD31114000000446F776E90000000010000000001000000000000
        00000000000084901F1200000000FFFFFFFF0000010101000000780000000000
        00001027000000010061F4410610010000000000000000000000000001000000
        00000000000000010000000000000F000000656243616D65726141726342616C
        6C00F01B0410F01B041030000000656243616D65726141726342616C6C0010AD
        D31110ADD31114000000446F776E900000000100000000010000000000000000
        0000000084901F1200000000FFFFFFFF00000101010000007800000000000000
        1027000000010061A86C620E0200000000000000000000000000010000000000
        0000000000010000000000000F000000656243616D65726141726342616C6C00
        F01B0410F01B041030000000656243616D65726141726342616C6C0010ADD311
        10ADD31114000000446F776E9000000001000000000100000000000000000000
        000084901F1200000000FFFFFFFF000001010100000078000000000000001027
        0000000100610051320F03000000000000000000000000000100000000000000
        000000010000000000000F000000656243616D65726141726342616C6C00F01B
        0410F01B041030000000656243616D65726141726342616C6C0010ADD31110AD
        D31114000000446F776E90000000010000000001000000000000000000000000
        84901F1200000000FFFFFFFF0000010101000000780000000000000010270000
        000100617802F702040000000000000000000000000001000000000000000000
        00010000000000000F000000656243616D65726141726342616C6C00F01B0410
        F01B041030000000656243616D65726141726342616C6C0010ADD31110ADD311
        14000000446F776E900000000100000000010000000000000000000000008490
        1F1200000000FFFFFFFF00000101010000007800000000000000102700000001
        0061B4E1DA040500000000000000000000000000010000000000000000000001
        0000000000000F000000656243616D65726141726342616C6C00F01B0410F01B
        041030000000656243616D65726141726342616C6C0010ADD31110ADD3111400
        0000446F776E9000000001000000000100000000000000000000000084901F12
        00000000FFFFFFFF000001010100000078000000000000001027000000010061
        288C870D06000000000000000000000000000100000000000000000000010000
        000000000F000000656243616D65726141726342616C6C00F01B0410F01B0410
        30000000656243616D65726141726342616C6C0010ADD31110ADD31114000000
        446F776E90000000010000000001000000000000000000000000}
      HeaderFont.Charset = DEFAULT_CHARSET
      HeaderFont.Color = clBlack
      HeaderFont.Height = -11
      HeaderFont.Name = 'MS Sans Serif'
      HeaderFont.Style = []
      HorizontalLines = True
      HorzDivLinesColor = 7368816
      HorzScrollBarStyles.ShowTrackHint = False
      HorzScrollBarStyles.Width = 16
      HorzScrollBarStyles.ButtonSize = 14
      HorzScrollBarStyles.UseSystemMetrics = False
      HorzScrollBarStyles.UseXPThemes = False
      IgnoreEnabled = False
      IncrementalSearch = False
      ItemIndent = 14
      KeepSelectionWithinLevel = False
      LineBorderActiveColor = clBlack
      LineBorderInactiveColor = clBlack
      LineHeight = 16
      LinesColor = clBlack
      MultiSelect = False
      OwnerDrawMask = '~~@~~'
      ParentFont = False
      PlusMinusTransparent = True
      ScrollbarOpposite = False
      ScrollTracking = True
      ShowButtons = False
      ShowImages = False
      ShowLeafButton = False
      ShowLines = False
      ShowRootButtons = False
      SortMode = smAdd
      StoragePath = '\Tree'
      TabOrder = 0
      TabStop = True
      Tracking = False
      TrackColor = clBlack
      VertScrollBarStyles.ShowTrackHint = True
      VertScrollBarStyles.Width = 16
      VertScrollBarStyles.ButtonSize = 14
      VertScrollBarStyles.UseSystemMetrics = False
      VertScrollBarStyles.UseXPThemes = False
      VirtualityLevel = vlNone
      UseXPThemes = False
      TextColor = clBtnText
      BkColor = 5592405
      OnItemFocused = tvItemsItemFocused
      LinkStyle = [fsUnderline]
      OnDragDrop = tvItemsDragDrop
      OnDragOver = tvItemsDragOver
      OnStartDrag = tvItemsStartDrag
      OnKeyPress = tvItemsKeyPress
    end
    object paObject: TPanel
      Left = 0
      Top = 0
      Width = 219
      Height = 241
      Align = alTop
      BevelOuter = bvNone
      Constraints.MinHeight = 240
      ParentColor = True
      TabOrder = 1
      object paObjectProps: TPanel
        Left = 0
        Top = 158
        Width = 219
        Height = 83
        Align = alClient
        BevelOuter = bvNone
        Color = 5592405
        TabOrder = 0
      end
      object paImage: TMxPanel
        Left = 0
        Top = 0
        Width = 219
        Height = 158
        Align = alTop
        BevelOuter = bvLowered
        Caption = '<no image>'
        Color = 5592405
        TabOrder = 1
        OnPaint = paImagePaint
      end
    end
    object Panel1: TPanel
      Left = 0
      Top = 477
      Width = 219
      Height = 19
      Align = alBottom
      BevelOuter = bvNone
      ParentColor = True
      TabOrder = 2
      object ebAddObject: TExtBtn
        Left = 1
        Top = 1
        Width = 26
        Height = 17
        Align = alNone
        BevelShow = False
        BtnColor = 6842472
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          AE000000424DAE00000000000000360000002800000006000000060000000100
          18000000000078000000120B0000120B00000000000000000000FFFFFFFFFFFF
          1C1C1C1C1C1CFFFFFFFFFFFF0000FFFFFFFFFFFF191919191919FFFFFFFFFFFF
          00001B1B1B1717170303030303031717171B1B1B00001B1B1B17171703030303
          03031717171B1B1B0000FFFFFFFFFFFF191919191919FFFFFFFFFFFF0000FFFF
          FFFFFFFF1C1C1C1C1C1CFFFFFFFFFFFF0000}
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebAddObjectClick
      end
      object ebDelObject: TExtBtn
        Left = 31
        Top = 1
        Width = 26
        Height = 17
        Align = alNone
        BevelShow = False
        BtnColor = 6842472
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          AE000000424DAE00000000000000360000002800000006000000060000000100
          18000000000078000000120B0000120B00000000000000000000FFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFF0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          00000B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B00000B0B0B0B0B0B0B0B0B0B
          0B0B0B0B0B0B0B0B0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0000FFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0000}
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebDelObjectClick
      end
      object ebLoadList: TExtBtn
        Left = 91
        Top = 1
        Width = 39
        Height = 17
        Align = alNone
        BevelShow = False
        BtnColor = 6842472
        Caption = 'Load...'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebLoadListClick
      end
      object ebSaveList: TExtBtn
        Left = 134
        Top = 1
        Width = 39
        Height = 17
        Align = alNone
        BevelShow = False
        BtnColor = 6842472
        Caption = 'Save...'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebSaveListClick
      end
      object ebClearList: TExtBtn
        Left = 61
        Top = 1
        Width = 26
        Height = 17
        Align = alNone
        BevelShow = False
        BtnColor = 6842472
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clBlack
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        Glyph.Data = {
          C2010000424DC20100000000000036000000280000000C0000000B0000000100
          1800000000008C010000120B0000120B00000000000000000000FFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFF000000000000FF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFF000000000000
          FFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFF0000000000000000000000
          00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFF00000000
          0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000
          000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0000
          00000000FFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00
          0000000000000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFF
          000000000000FFFFFFFFFFFF000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FF000000000000FFFFFFFFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFFFFFF
          FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
          FFFFFFFFFFFF}
        ParentFont = False
        Transparent = False
        FlatAlwaysEdge = True
        OnClick = ebClearListClick
      end
    end
  end
  object fsStorage: TFormStorage
    OnSavePlacement = fsStorageSavePlacement
    OnRestorePlacement = fsStorageRestorePlacement
    StoredProps.Strings = (
      'paObject.Height')
    StoredValues = <
      item
        Name = 'EmitterDirX'
        Value = 0
      end
      item
        Name = 'EmitterDirY'
        Value = 0
      end
      item
        Name = 'EmitterDirZ'
        Value = 0
      end>
  end
end
