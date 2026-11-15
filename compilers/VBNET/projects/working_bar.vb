' working_bar.vb
'
' Animates a "working" indicator: a dot (●) swings back and forth inside
' brackets, easing in and out with a cosine, seven times with shorter
' delays and different colors.
' VB.NET version of documentation/projects/working_bar.asm (with the
' working_bar function of system/console.asm).
'
' Author:   Richard Lesh
Imports System.Threading

Module working_bar
    Const ProgressSize As Integer = 40

    ReadOnly Esc As String = ChrW(27)
    ReadOnly Reset As String = Esc & "[0m"
    ReadOnly BrightRed As String = Esc & "[91m"
    ReadOnly BrightGreen As String = Esc & "[92m"
    ReadOnly BrightYellow As String = Esc & "[93m"
    ReadOnly BrightBlue As String = Esc & "[94m"
    ReadOnly BrightMagenta As String = Esc & "[95m"
    ReadOnly BrightCyan As String = Esc & "[96m"
    ReadOnly BrightWhite As String = Esc & "[97m"
    ReadOnly HideCursor As String = Esc & "[?25l"
    ReadOnly ShowCursor As String = Esc & "[?25h"

    ' waits ms milliseconds; with the environment variable NO_DELAY set it returns at once (for the tests)
    Sub Delay(ms As Integer)
        If Environment.GetEnvironmentVariable("NO_DELAY") Is Nothing Then Thread.Sleep(ms)
    End Sub

    ' "[   ●      ]" with len cells; the dot is at (1 - cos(2 pi pct)) / 2 of the way (color Nothing: no dot)
    Sub WorkingBar(len As Integer, pct As Double, color As String)
        Dim f As Double = -(((Math.Cos(Math.PI * 2 * pct) + 1) / 2) - 1)
        ' rounds half away from zero, as the assembly version does (Math.Round rounds half to even)
        Dim barLen As Integer = CInt(Math.Floor(f * (len - 1) + 0.5)) + 1
        Console.Write(Reset)
        Console.Write("["c)
        Console.Write(StrDup(barLen - 1, " "c))
        If color IsNot Nothing Then
            Console.Write(color)
            Console.Write(ChrW(&H25CF))
            Console.Write(Reset)
        Else
            Console.Write(" "c)
        End If
        Console.Write(StrDup(len - barLen, " "c))
        Console.Write("]"c)
    End Sub

    Sub BarDemo(delayMs As Integer, color As String)
        Dim p As Double = 0
        For i As Integer = 0 To 300
            Console.Write("Processing: ")
            p = i / 100.0
            WorkingBar(ProgressSize, p, color)
            Delay(delayMs)
            Console.Write(vbCr)
        Next
        Console.Write("Processing: ")
        WorkingBar(ProgressSize, p, Nothing)
        Console.WriteLine(" Done")
    End Sub

    Function Main() As Integer
        Console.Write(HideCursor)
        BarDemo(60, BrightCyan)
        BarDemo(50, BrightMagenta)
        BarDemo(40, BrightYellow)
        BarDemo(30, BrightRed)
        BarDemo(20, BrightGreen)
        BarDemo(10, BrightBlue)
        BarDemo(5, BrightWhite)
        Console.Write(ShowCursor)
        Return 0
    End Function
End Module
