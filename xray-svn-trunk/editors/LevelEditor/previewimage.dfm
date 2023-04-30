object frmPreviewImage: TfrmPreviewImage
  Left = 440
  Top = 343
  BorderStyle = bsToolWindow
  Caption = 'Image Viewer'
  ClientHeight = 258
  ClientWidth = 256
  Color = 2302755
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  Position = poScreenCenter
  Scaled = False
  OnClose = FormClose
  OnKeyDown = FormKeyDown
  PixelsPerInch = 96
  TextHeight = 13
  object paImage: TPanel
    Left = 0
    Top = 0
    Width = 256
    Height = 258
    Align = alClient
    BevelOuter = bvLowered
    Caption = '<no image>'
    Color = 5131854
    TabOrder = 0
    object pbImage: TPaintBox
      Left = 1
      Top = 1
      Width = 254
      Height = 256
      Cursor = crSizeAll
      Align = alClient
      Color = 9276813
      ParentColor = False
      OnMouseDown = pbImageMouseDown
      OnPaint = pbImagePaint
    end
  end
end
