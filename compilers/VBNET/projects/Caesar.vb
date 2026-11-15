' Caesar.vb
'
' Encrypts or decrypts a text file with a Caesar cipher on the printable
' ASCII characters (space to ~, 95 of them): each is moved by the key,
' wrapping around; other characters are copied unchanged.
' VB.NET version of documentation/projects/Caesar.asm.
'
' Usage:
'     Caesar <key> <input_file> <output_file>
'
'     key in [1,94] encrypts, key in [-94,-1] decrypts, 0 is a no-op
'
' Author:   Richard Lesh
Imports System.IO
Imports System.Text

Module Caesar
    Function Shift(text As String, key As Long) As String
        Dim sb As New StringBuilder()
        For Each ch As Char In text
            Dim c As Integer = AscW(ch)
            If c >= 32 AndAlso c <= 126 Then
                Dim v As Long = c - 32 + key
                While v < 0
                    v += 95
                End While
                While v >= 95
                    v -= 95
                End While
                c = CInt(v) + 32
            End If
            sb.Append(ChrW(c))
        Next
        Return sb.ToString()
    End Function

    Function Main(args As String()) As Integer
        If args.Length < 3 Then
            Console.WriteLine("Syntax: Caesar <key> <input_file> <output_file>")
            Console.WriteLine("    key can be in the range [1,94] to encrypt.")
            Console.WriteLine("    key can be in the range [-94,-1] to decrypt.")
            Console.WriteLine("    key == 0 is a no-op")
            Return 1
        End If
        If Not File.Exists(args(1)) Then
            Console.WriteLine("Input file open failed!")
            Return 3
        End If
        Dim key As Long = Long.Parse(args(0)) Mod 95
        File.WriteAllText(args(2), Shift(File.ReadAllText(args(1)), key))
        Return 0
    End Function
End Module
