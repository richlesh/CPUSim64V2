' hashcode2.vb
'
' Prints a 64-bit hash code of a file: hash = hash * 31 + byte for every
' byte, as Java's String.hashCode does but in 64 bits.  The file is read
' in blocks of 8 bytes; the bytes of the last, short block are printed as
' they are hashed.  VB.NET checks integer overflow, so the multiplication
' wraps around through WrapAdd.
' VB.NET version of documentation/projects/hashcode2.asm.
'
' Usage:
'     hashcode [<file>]
'
' Without a file name the lines of standard input are hashed, each followed
' by a newline.
'
' Author:   Richard Lesh
Imports System.IO

Module hashcode2
    ' a + b modulo 2^64
    Function WrapAdd(a As ULong, b As ULong) As ULong
        Dim lo As ULong = (a And &HFFFFFFFFUL) + (b And &HFFFFFFFFUL)
        Dim hi As ULong = (a >> 32) + (b >> 32) + (lo >> 32)
        Return ((hi And &HFFFFFFFFUL) << 32) Or (lo And &HFFFFFFFFUL)
    End Function

    ' hash * 31 + c modulo 2^64: 31 * hash is (hash << 5) - hash
    Function Step31(hash As ULong, c As ULong) As ULong
        Return WrapAdd(WrapAdd(hash << 5, Not hash), c + 1UL)
    End Function

    Function Hash(data As Byte()) As ULong
        Dim h As ULong = 0
        Dim whole As Integer = data.Length - data.Length Mod 8      ' the bytes in complete blocks
        For i As Integer = 0 To data.Length - 1
            Dim c As Integer = data(i)
            If i >= whole Then Console.WriteLine("tail byte: {0:x2}", c)
            h = Step31(h, CULng(c))
        Next
        Return h
    End Function

    ' the lines of standard input, each followed by a newline, as bytes (UTF-8 is not decoded)
    Function ReadStdin() As Byte()
        Dim bytes As New List(Of Byte)
        Dim line As String = Console.ReadLine()
        While line IsNot Nothing
            For Each ch As Char In line
                bytes.Add(CByte(AscW(ch) And 255))
            Next
            bytes.Add(10)
            line = Console.ReadLine()
        End While
        Return bytes.ToArray()
    End Function

    Function Main(args As String()) As Integer
        If args.Length < 1 Then
            Console.WriteLine("{0:x16}", Hash(ReadStdin()))
            Return 0
        End If
        If Not File.Exists(args(0)) Then
            Console.WriteLine("Failed to open: {0}", args(0))
            Return 2
        End If
        Console.WriteLine("{0:x16} {1}", Hash(File.ReadAllBytes(args(0))), args(0))
        Return 0
    End Function
End Module
