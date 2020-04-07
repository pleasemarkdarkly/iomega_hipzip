VERSION 5.00
Object = "{648A5603-2C6E-101B-82B6-000000000014}#1.1#0"; "MSCOMM32.OCX"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3090
   ClientLeft      =   60
   ClientTop       =   450
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   ScaleHeight     =   3090
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
   Begin MSCommLib.MSComm MSComm1 
      Left            =   120
      Top             =   0
      _ExtentX        =   1005
      _ExtentY        =   1005
      _Version        =   393216
      BaudRate        =   57600
   End
   Begin VB.CommandButton ConnectButton 
      Caption         =   "Connect"
      Height          =   375
      Left            =   1440
      TabIndex        =   2
      Top             =   1080
      Width           =   1335
   End
   Begin VB.TextBox Text1 
      Height          =   375
      Left            =   960
      TabIndex        =   1
      Top             =   480
      Width           =   2295
   End
   Begin VB.CommandButton RunButton 
      Caption         =   "Run Test"
      Height          =   495
      Left            =   1440
      TabIndex        =   0
      Top             =   1800
      Width           =   1335
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' DJ VB Test Framework
' Teman Clark-Lindh <temancl@fullplaymedia.com>
' (c) 2002 Fullplay Media - All Rights Reserved
' Not for Redistribution
'
' Visual Basic framework for DJ automated testing
'
' May god have mercy on your soul.

Option Explicit

Private Declare Function testnet Lib "test" Alias "test_dll@12" (ByVal ipstr As String, ByRef isend As Long, ByRef irecv As Long) As Long

Dim ipstring As String


Private Sub ConnectButton_Click()
    Dim DialString$, FromModem$, dummy
    MSComm1.CommPort = 1
    MSComm1.Settings = "57600,N,8,1"
    
    On Error Resume Next
    MSComm1.PortOpen = True
    If Err Then
       MsgBox "COM1: not available."
       Exit Sub
    End If
    
     ' Flush the input buffer.
    MSComm1.InBufferCount = 0
    DialString$ = "slave" & vbCrLf
    Do
       dummy = DoEvents()
       ' If there is data in the buffer, then read it.
       If MSComm1.InBufferCount Then
          FromModem$ = FromModem$ + MSComm1.Input
          ' Check for "OK".
          If InStr(FromModem$, ">") Then
             ' Notify the user to pick up the phone.
             
             MsgBox "Connect OK"
             
            ' start net slave
            MSComm1.Output = DialString$
            
             Exit Do
          End If
       End If
        
       ' Did the user choose Cancel?
       'If CancelFlag Then
       '   CancelFlag = False
       '   Exit Do
       'End If
    Loop
    
End Sub

Private Sub RunButton_Click()
  Dim sendres, recvres, ret As Long
    sendres = 0
    recvres = 0
    
    MsgBox "Starting DLL Call"
    MsgBox ipstring
    
    ' you must call DLLs with a return for this to work right
    ret = testnet(Text1.Text, sendres, recvres)
    MsgBox sendres
    MsgBox recvres
    MsgBox "Ending DLL Call"
End Sub

