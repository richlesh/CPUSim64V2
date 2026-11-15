' rainbow.vb
'
' Shows the ANSI colors: a full-width bar in each of the 16 background
' colors, then a line of text in each of the 16 foreground colors, plain,
' bold and dim.
' VB.NET version of documentation/projects/rainbow.asm.
'
' Author:   Richard Lesh
Module rainbow
    ReadOnly Esc As String = ChrW(27)
    ReadOnly Reset As String = Esc & "[0m"
    ReadOnly Bold As String = Esc & "[1m"
    ReadOnly Dimmed As String = Esc & "[2m"
    ReadOnly ClearToEol As String = Esc & "[0K"

    ' the SGR sequence of a color code
    Function Sgr(code As Integer) As String
        Return Esc & "[" & code & "m"
    End Function

    ' a line filled with the background color
    Sub DrawBar(bg As String)
        Console.Write(bg)
        Console.Write(ClearToEol)
        Console.Write(Reset)
        Console.WriteLine()
    End Sub

    Sub DrawText(fg As String, modifier As String)
        Console.Write(fg)
        Console.Write(modifier)
        Console.Write("Lorem ipsum dolor sit amet, consectetur adipiscing elit")
        Console.Write(Reset)
        Console.WriteLine()
    End Sub

    ' a heading and the eight colors from first with the modifier
    Sub DrawGroup(title As String, first As Integer, modifier As String)
        Console.WriteLine()
        Console.WriteLine(title)
        For i As Integer = 0 To 7
            DrawText(Sgr(first + i), modifier)
        Next
    End Sub

    Function Main() As Integer
        For i As Integer = 0 To 7                   ' black, red, green, yellow, blue, magenta, cyan, white
            DrawBar(Sgr(40 + i))
        Next
        For i As Integer = 0 To 7                   ' and the bright ones
            DrawBar(Sgr(100 + i))
        Next
        DrawGroup("Regular", 30, "")
        DrawGroup("Bright", 90, "")
        DrawGroup("Regular Bold", 30, Bold)
        DrawGroup("Bright Bold", 90, Bold)
        DrawGroup("Regular Dim", 30, Dimmed)
        DrawGroup("Bright Dim", 90, Dimmed)
        Return 0
    End Function
End Module
