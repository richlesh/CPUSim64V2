' progress_bar2.vb
'
' Animates an indeterminate progress bar: a three-cell "eye" with a fading
' trail (▓▒░) bounces back and forth in a 40-cell bar, six times with
' shorter delays and different colors.
' VB.NET version of documentation/projects/progress_bar2.asm (with the
' progress_bar function of system/console.asm).
'
' Author:   Richard Lesh
Imports System.Threading

Module progress_bar2
    Const Size As Integer = 40
    Const EyeWidth As Integer = 3

    ReadOnly Esc As String = ChrW(27)
    ReadOnly Reset As String = Esc & "[0m"
    ReadOnly BrightBlack As String = Esc & "[90m"
    ReadOnly BrightRed As String = Esc & "[91m"
    ReadOnly BrightGreen As String = Esc & "[92m"
    ReadOnly BrightYellow As String = Esc & "[93m"
    ReadOnly BrightBlue As String = Esc & "[94m"
    ReadOnly BrightMagenta As String = Esc & "[95m"
    ReadOnly BrightCyan As String = Esc & "[96m"
    ReadOnly HideCursor As String = Esc & "[?25l"
    ReadOnly ShowCursor As String = Esc & "[?25h"

    ' waits ms milliseconds; with the environment variable NO_DELAY set it returns at once (for the tests)
    Sub Delay(ms As Integer)
        If Environment.GetEnvironmentVariable("NO_DELAY") Is Nothing Then Thread.Sleep(ms)
    End Sub

    ' the position of the eye at step x: 0, 1, ... m-1, m-2, ... 1, 0, 1, ...
    Function Triangle(x As Integer, m As Integer) As Integer
        Dim period As Integer = (m - 1) * 2, xm As Integer = x Mod period
        Return If(xm < m, xm, period - xm)
    End Function

    ' 1 while the eye moves right, -1 while it moves left
    Function Direction(x As Integer, m As Integer) As Integer
        Dim period As Integer = (m - 1) * 2, xm As Integer = x Mod period
        Return If(xm < m - 1, 1, -1)
    End Function

    ' writes the color of the cell at col and returns its character (color Nothing: the empty bar)
    Function EyeChar(col As Integer, pos As Integer, dir As Integer, color As String) As Char
        Dim eyeEnd As Integer = pos + EyeWidth - 1
        If color Is Nothing Then
            Console.Write(BrightBlack)
            Return ChrW(&H2591)
        End If
        If col >= pos AndAlso col <= eyeEnd Then
            Console.Write(color)
            Return ChrW(&H2588)                                 ' the eye
        End If
        Dim t As Integer = If(dir > 0, pos - col - 1, col - eyeEnd - 1)     ' the distance behind the eye
        If t < 0 OrElse t > 2 Then
            Console.Write(BrightBlack)
            Return ChrW(&H2591)
        End If
        Console.Write(color)
        Return If(t = 0, ChrW(&H2593), If(t = 1, ChrW(&H2592), ChrW(&H2591)))    ' the trail fades
    End Function

    Sub ProgressBar(total As Integer, count As Integer, color As String)
        Dim span As Integer = total - EyeWidth + 1
        Dim pos As Integer = Triangle(count, span), dir As Integer = Direction(count, span)
        For i As Integer = 0 To total - 1
            Console.Write(EyeChar(i, pos, dir, color))
        Next
        Console.Write(Reset)
    End Sub

    Sub BarDemo(delayMs As Integer, color As String)
        For p As Integer = 0 To 150
            Console.Write("Copying: ")
            ProgressBar(Size, p, color)
            Delay(delayMs)
            Console.Write(vbCr)
        Next
        Console.Write("Copying: ")
        ProgressBar(Size, 150, Nothing)
        Console.WriteLine(" Done")
    End Sub

    Function Main() As Integer
        Console.WriteLine("Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890")
        Console.Write(HideCursor)
        BarDemo(100, BrightCyan)
        BarDemo(80, BrightMagenta)
        BarDemo(60, BrightYellow)
        BarDemo(40, BrightRed)
        BarDemo(20, BrightGreen)
        BarDemo(10, BrightBlue)
        Console.Write(ShowCursor)
        Return 0
    End Function
End Module
