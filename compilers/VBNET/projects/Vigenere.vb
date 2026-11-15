' Vigenere.vb
'
' Encrypts or decrypts a text file with a Vigenere cipher on the visible
' ASCII characters (! to ~, 94 of them).  Each such character is moved by the
' next character of the keyword (! is 0), wrapping around; spaces, newlines
' and other characters are copied unchanged and do not use up a key
' character.
' VB.NET version of documentation/projects/Vigenere.asm.
'
' Usage:
'     Vigenere [-]keyword <input_file> <output_file>
'
'     a keyword starting with '-' decrypts
'
' Author:   Richard Lesh
Imports System.IO
Imports System.Text

Module Vigenere
    Function Cipher(key As String, text As String) As String
        Dim decrypt As Boolean = False
        If key.Length > 0 AndAlso key(0) = "-"c Then
            decrypt = True
            key = key.Substring(1)
        End If
        If key.Length = 0 Then Return ""
        Dim sb As New StringBuilder()
        Dim index As Integer = 0
        For Each ch As Char In text
            Dim c As Integer = AscW(ch)
            If c > 32 AndAlso c <= 126 Then
                Dim offset As Integer = AscW(key(index)) - 33
                Dim v As Integer = c - 33 + If(decrypt, -offset, offset)
                While v < 0
                    v += 94
                End While
                While v >= 94
                    v -= 94
                End While
                c = v + 33
                index += 1
                If index >= key.Length Then index = 0
            End If
            sb.Append(ChrW(c))
        Next
        Return sb.ToString()
    End Function

    Function Main(args As String()) As Integer
        If args.Length < 3 Then
            Console.WriteLine("Syntax: Vigenere [-]keyword input_file output_file")
            Console.WriteLine("    a keyword starting with '-' decrypts")
            Return 1
        End If
        If Not File.Exists(args(1)) Then
            Console.WriteLine("Input file open failed!")
            Return 2
        End If
        File.WriteAllText(args(2), Cipher(args(0), File.ReadAllText(args(1))))
        Return 0
    End Function
End Module
