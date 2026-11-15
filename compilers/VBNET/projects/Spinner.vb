' Spinner.vb
'
' Animates a text spinner ("Working: |", "/", "-", "\") for 48 frames,
' changing the color every 8 frames.
' VB.NET version of documentation/projects/Spinner.asm.
'
' Author:   Richard Lesh
Imports System.Threading

Module Spinner
    Const FrameCount As Integer = 48
    Const FrameDelay As Integer = 300           ' milliseconds
    Const FramesPerColor As Integer = 8

    ReadOnly Esc As String = ChrW(27)
    ReadOnly Reset As String = Esc & "[0m"
    ReadOnly HideCursor As String = Esc & "[?25l"
    ReadOnly ShowCursor As String = Esc & "[?25h"
    ' bright cyan, magenta, yellow, red, green, blue
    ReadOnly Colors As String() = {Esc & "[96m", Esc & "[95m", Esc & "[93m", Esc & "[91m", Esc & "[92m", Esc & "[94m"}
    ReadOnly Frames As Char() = {"|"c, "/"c, "-"c, "\"c}

    ' waits ms milliseconds; with the environment variable NO_DELAY set it returns at once (for the tests)
    Sub Delay(ms As Integer)
        If Environment.GetEnvironmentVariable("NO_DELAY") Is Nothing Then Thread.Sleep(ms)
    End Sub

    Function Main() As Integer
        Console.Write(HideCursor)
        For i As Integer = 0 To FrameCount - 1
            Console.Write(Colors((i \ FramesPerColor) Mod Colors.Length))
            Console.Write("Working: ")
            Console.Write(Frames(i Mod 4))
            Delay(FrameDelay)
            Console.Write(vbCr)
        Next
        Console.Write(Reset)
        Console.Write(ShowCursor)
        Console.WriteLine("Working: Done!        ")
        Return 0
    End Function
End Module
