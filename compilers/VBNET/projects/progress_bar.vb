' progress_bar.vb
'
' Animates a percentage progress bar ("Copying: ████░░░░  42%") six times,
' with steps of 1% to 6% and a different color each time.  The bar is
' redrawn on the same line with a carriage return; the cursor is hidden
' while it runs.
' VB.NET version of documentation/projects/progress_bar.asm (with the
' progress_bar_percent function of system/console.asm).
'
' Author:   Richard Lesh
Imports System.Threading

Module progress_bar
    Const Size As Integer = 74

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

    ' rounds half away from zero, as the assembly version does (Math.Round rounds half to even)
    Function RoundHalfUp(x As Double) As Integer
        Return CInt(Math.Floor(x + 0.5))
    End Function

    ' a bar of total cells, the fraction pct of them filled, then the percentage (right aligned, omitted at 100%)
    Sub ProgressBarPercent(total As Integer, pct As Double, color As String)
        If pct < 0.0 Then pct = 0.0
        If pct > 1.0 Then pct = 1.0
        Dim filled As Integer = RoundHalfUp(total * pct)
        Dim pctInt As Integer = RoundHalfUp(pct * 100)
        Console.Write(color)
        Console.Write(StrDup(filled, ChrW(&H2588)))         ' full block
        Console.Write(BrightBlack)
        Console.Write(StrDup(total - filled, ChrW(&H2591)))  ' light shade
        Console.Write(Reset)
        If pctInt < 100 Then Console.Write(" {0,3}%", pctInt)
    End Sub

    Sub BarDemo(incr As Integer, color As String)
        For p As Integer = 0 To 100 Step incr
            Console.Write("Copying: ")
            ProgressBarPercent(Size, p / 100.0, color)
            Delay(100)
            Console.Write(vbCr)
        Next
        Console.Write("Copying: ")
        ProgressBarPercent(Size, 1.0, Reset)
        Console.WriteLine(" Done")
    End Sub

    Function Main() As Integer
        Console.WriteLine("Copying: 12345678901234567890123456789012345678901234567890123456789012345678901234567890")
        Console.Write(HideCursor)
        BarDemo(1, BrightCyan)
        BarDemo(2, BrightMagenta)
        BarDemo(3, BrightYellow)
        BarDemo(4, BrightRed)
        BarDemo(5, BrightGreen)
        BarDemo(6, BrightBlue)
        Console.Write(ShowCursor)
        Return 0
    End Function
End Module
