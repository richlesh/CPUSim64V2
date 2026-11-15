' rot13.vb
'
' Applies the ROT13 substitution (each letter is replaced by the letter 13
' places along the alphabet) to a text file, or to standard input.
' VB.NET version of documentation/projects/rot13.asm.
'
' Usage:
'     rot13 [<input_file> [<output_file>]]
'
' Without file names the text is read from standard input and written to
' standard output, line by line.
'
' Author:   Richard Lesh
Imports System.IO
Imports System.Text

Module rot13
    Function Rotate(text As String) As String
        Dim sb As New StringBuilder()
        For Each ch As Char In text
            Dim c As Integer = AscW(ch)
            If c >= 65 AndAlso c <= 90 Then
                c = 65 + (c - 65 + 13) Mod 26
            ElseIf c >= 97 AndAlso c <= 122 Then
                c = 97 + (c - 97 + 13) Mod 26
            End If
            sb.Append(ChrW(c))
        Next
        Return sb.ToString()
    End Function

    Function Main(args As String()) As Integer
        If args.Length >= 1 AndAlso Not File.Exists(args(0)) Then
            Console.WriteLine("Can't open {0} for reading.", args(0))
            Return 1
        End If
        If args.Length >= 1 Then
            Dim result As String = Rotate(File.ReadAllText(args(0)))
            If args.Length >= 2 Then
                File.WriteAllText(args(1), result)
            Else
                Console.Write(result)
            End If
        Else
            Dim line As String = Console.ReadLine()
            While line IsNot Nothing
                Console.WriteLine(Rotate(line))
                line = Console.ReadLine()
            End While
        End If
        Return 0
    End Function
End Module
