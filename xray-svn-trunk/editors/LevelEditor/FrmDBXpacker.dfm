object DB_packer: TDB_packer
  Left = 1001
  Top = 271
  Width = 687
  Height = 604
  Caption = 'DB_packer'
  Color = 2960685
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 671
    Height = 466
    Align = alClient
    BevelOuter = bvNone
    Color = 10528425
    TabOrder = 0
    object Panel5: TPanel
      Left = 404
      Top = 0
      Width = 267
      Height = 466
      Align = alRight
      BevelOuter = bvNone
      Color = 2960685
      TabOrder = 0
      object shellTree: TElShellTree
        Left = 0
        Top = 0
        Width = 267
        Height = 466
        Cursor = crDefault
        LeftPosition = 0
        RootFolder = sfoDesktop
        UseSystemMenus = False
        CheckForChildren = False
        ShowFiles = True
        FileSystemOnly = False
        SortModifiers = [ssmFoldersFirst, ssmExecutablesFirst]
        SizeFormat = ssfAsIs
        DefaultColumns = False
        Align = alClient
        DefaultSectionWidth = 120
        BorderSides = [ebsLeft, ebsRight, ebsTop, ebsBottom]
        DragCursor = crDrag
        FocusedSelectColor = 12171705
        FocusedSelectTextColor = clSilver
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        HeaderHeight = 19
        HeaderHotTrack = False
        HeaderInvertSortArrows = True
        HeaderSections.Data = {F4FFFFFF00000000}
        HeaderFont.Charset = DEFAULT_CHARSET
        HeaderFont.Color = clWindowText
        HeaderFont.Height = -11
        HeaderFont.Name = 'MS Sans Serif'
        HeaderFont.Style = []
        HorzScrollBarStyles.ShowTrackHint = False
        HorzScrollBarStyles.Width = 17
        HorzScrollBarStyles.ButtonSize = 17
        IncrementalSearch = False
        LineBorderActiveColor = clBlack
        LineBorderInactiveColor = clBlack
        LineHeight = 17
        LinesColor = 1513239
        LinesStyle = psSolid
        LineHintColor = clAqua
        MultiSelect = False
        OwnerDrawMask = '~~@~~'
        ScrollbarOpposite = False
        ShowLeafButton = False
        SortMode = smAddClick
        StoragePath = '\Tree'
        VertScrollBarStyles.ShowTrackHint = True
        VertScrollBarStyles.Width = 17
        VertScrollBarStyles.ButtonSize = 17
        TextColor = clBtnText
        BkColor = 6710886
        DockOrientation = doNoOrient
        BorderStyle = bsNone
        ParentFont = False
        TabOrder = 0
        TabStop = True
      end
    end
    object Panel6: TPanel
      Left = 0
      Top = 0
      Width = 404
      Height = 466
      Align = alClient
      BevelOuter = bvNone
      Color = 2960685
      TabOrder = 1
      object Splitter1: TSplitter
        Left = 0
        Top = 231
        Width = 404
        Height = 1
        Cursor = crVSplit
        Align = alTop
        Color = 6381921
        ParentColor = False
      end
      object Panel3: TPanel
        Left = 0
        Top = 232
        Width = 404
        Height = 234
        Align = alClient
        BevelOuter = bvNone
        Color = 5131854
        TabOrder = 0
        DesignSize = (
          404
          234)
        object MxLabel2: TMxLabel
          Left = 8
          Top = 4
          Width = 58
          Height = 13
          Caption = 'Include files'
          ShadowColor = 8158332
        end
        object ExtBtn3: TExtBtn
          Left = 367
          Top = 26
          Width = 33
          Height = 17
          Align = alNone
          Anchors = [akTop, akRight]
          BevelShow = False
          Caption = '<<'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          Kind = knNone
          ParentFont = False
          FlatAlwaysEdge = True
          OnClick = ExtBtn3Click
        end
        object ExtBtn4: TExtBtn
          Left = 367
          Top = 50
          Width = 33
          Height = 17
          Align = alNone
          Anchors = [akTop, akRight]
          BevelShow = False
          Caption = '>>'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          Kind = knNone
          ParentFont = False
          FlatAlwaysEdge = True
          OnClick = ExtBtn4Click
        end
        object lbIncludeFiles: TElListBox
          Left = 8
          Top = 24
          Width = 355
          Height = 206
          AllowGrayed = False
          BorderStyle = bsNone
          ItemHeight = 13
          MultiSelect = True
          ItemIndex = 0
          TopIndex = 0
          BorderSides = [ebsLeft, ebsRight, ebsTop, ebsBottom]
          HorizontalScroll = False
          LineBorderActiveColor = clBlack
          LineBorderInactiveColor = clBlack
          SelectedFont.Charset = DEFAULT_CHARSET
          SelectedFont.Color = clHighlightText
          SelectedFont.Height = -11
          SelectedFont.Name = 'MS Sans Serif'
          SelectedFont.Style = []
          Anchors = [akLeft, akTop, akRight, akBottom]
          Color = 6710886
          TabOrder = 0
        end
      end
      object Panel4: TPanel
        Left = 0
        Top = 0
        Width = 404
        Height = 231
        Align = alTop
        BevelOuter = bvNone
        Color = 5131854
        TabOrder = 1
        DesignSize = (
          404
          231)
        object MxLabel1: TMxLabel
          Left = 8
          Top = 4
          Width = 71
          Height = 13
          Caption = 'Include folders'
          ShadowColor = 8158332
        end
        object ExtBtn1: TExtBtn
          Left = 367
          Top = 26
          Width = 33
          Height = 17
          Align = alNone
          Anchors = [akTop, akRight]
          BevelShow = False
          Caption = '<<'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          Kind = knNone
          ParentFont = False
          FlatAlwaysEdge = True
          OnClick = ExtBtn1Click
        end
        object ExtBtn2: TExtBtn
          Left = 367
          Top = 51
          Width = 33
          Height = 17
          Align = alNone
          Anchors = [akTop, akRight]
          BevelShow = False
          Caption = '>>'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'MS Sans Serif'
          Font.Style = []
          Kind = knNone
          ParentFont = False
          FlatAlwaysEdge = True
          OnClick = ExtBtn2Click
        end
        object lbIncludeFolders: TElListBox
          Left = 8
          Top = 24
          Width = 355
          Height = 199
          AllowGrayed = False
          BorderStyle = bsNone
          ItemHeight = 13
          MultiSelect = True
          ItemIndex = 0
          TopIndex = 0
          BorderSides = [ebsLeft, ebsRight, ebsTop, ebsBottom]
          HorizontalScroll = False
          LineBorderActiveColor = clBlack
          LineBorderInactiveColor = clBlack
          SelectedFont.Charset = DEFAULT_CHARSET
          SelectedFont.Color = clHighlightText
          SelectedFont.Height = -11
          SelectedFont.Name = 'MS Sans Serif'
          SelectedFont.Style = []
          Anchors = [akLeft, akTop, akRight, akBottom]
          Color = 6710886
          TabOrder = 0
        end
      end
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 466
    Width = 671
    Height = 100
    Align = alBottom
    BevelOuter = bvNone
    Color = 5131854
    TabOrder = 1
    object btnLoad: TExtBtn
      Left = 80
      Top = 18
      Width = 57
      Height = 17
      Align = alNone
      BevelShow = False
      Caption = 'Load'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Kind = knNone
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = btnLoadClick
    end
    object btnSave: TExtBtn
      Left = 16
      Top = 18
      Width = 57
      Height = 17
      Align = alNone
      BevelShow = False
      Caption = 'Save'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Kind = knNone
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = btnSaveClick
    end
    object ExtBtn5: TExtBtn
      Left = 304
      Top = 18
      Width = 57
      Height = 17
      Align = alNone
      BevelShow = False
      Caption = 'Start'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      Kind = knNone
      ParentFont = False
      FlatAlwaysEdge = True
      OnClick = ExtBtn5Click
    end
    object Splitter2: TSplitter
      Left = 0
      Top = 0
      Width = 671
      Height = 1
      Cursor = crVSplit
      Align = alTop
      Color = 6381921
      ParentColor = False
    end
  end
end
