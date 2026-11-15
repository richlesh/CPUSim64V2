' XORCipher.vb
'
' Encrypts or decrypts a file by XORing each byte with the next character of
' a keyword, repeated as often as needed.  Running it again with the same
' keyword restores the file.
' VB.NET version of documentation/projects/XORCipher.asm.
'
' Usage:
'     XORCipher keyword input_file output_file
'
' Author:   Richard Lesh
Imports System.IO

Module XORCipher
    Function Cipher(key As String, data As Byte()) As Byte()
        Dim result(data.Length - 1) As Byte
        Dim index As Integer = 0
        For i As Integer = 0 To data.Length - 1
            result(i) = CByte((data(i) Xor AscW(key(index))) And 255)
            index += 1
            If index >= key.Length Then index = 0
        Next
        Return result
    End Function

    Function Main(args As String()) As Integer
        If args.Length < 3 Then
            Console.WriteLine("Syntax: XORCipher keyword input_file output_file")
            Return 0
        End If
        If Not File.Exists(args(1)) Then
            Console.WriteLine("Input file open failed!")
            Return 0
        End If
        File.WriteAllBytes(args(2), Cipher(args(0), File.ReadAllBytes(args(1))))
        Return 0
    End Function
End Module
