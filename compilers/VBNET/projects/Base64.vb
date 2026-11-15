' Base64.vb
'
' Encodes a file to Base64 text (57 input bytes, 76 characters, per line) or
' decodes Base64 text back to the original.
' VB.NET version of documentation/projects/Base64.asm.
'
' Usage:
'     Base64 <e|d> <input_file> <output_file>
'
' Author:   Richard Lesh
Imports System.IO
Imports System.Text

Module Base64
    Const PadChar As Char = "="c
    Const Alphabet As String = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

    ' the value of a Base64 character: 0-63, 64 for the padding, -1 for anything else
    Function DecodeValue(c As Integer) As Integer
        If c >= 65 AndAlso c <= 90 Then Return c - 65
        If c >= 97 AndAlso c <= 122 Then Return c - 97 + 26
        If c >= 48 AndAlso c <= 57 Then Return c - 48 + 52
        If c = 43 Then Return 62
        If c = 47 Then Return 63
        If c = AscW(PadChar) Then Return 64
        Return -1
    End Function

    Function Encode(data As Byte()) As String
        Dim sb As New StringBuilder()
        Dim index As Long = 0
        Dim buffer As Integer = 0
        Dim lineRemainder As Integer = 0
        For Each b As Byte In data
            Dim c As Integer = b
            Dim which As Integer = CInt(index Mod 3)
            If which = 0 Then
                sb.Append(Alphabet(c >> 2))
                buffer = (c And 3) << 8
            ElseIf which = 1 Then
                buffer = buffer Or c
                sb.Append(Alphabet(buffer >> 4))
                buffer = (buffer And 15) << 8
            Else
                buffer = buffer Or c
                sb.Append(Alphabet(buffer >> 6))
                sb.Append(Alphabet(buffer And 63))
            End If
            index += 1
            lineRemainder = CInt(index Mod 57)
            If lineRemainder = 0 Then sb.Append(vbLf)
        Next
        If index Mod 3 = 1 Then
            sb.Append(Alphabet(buffer >> 4))
            sb.Append(PadChar)
            sb.Append(PadChar)
        ElseIf index Mod 3 = 2 Then
            sb.Append(Alphabet(buffer >> 6))
            sb.Append(PadChar)
        End If
        If lineRemainder <> 0 Then sb.Append(vbLf)
        Return sb.ToString()
    End Function

    Function Decode(text As String) As Byte()
        Dim bytes As New List(Of Byte)
        Dim index As Long = 0
        Dim buffer As Integer = 0
        For Each ch As Char In text
            Dim v As Integer = DecodeValue(AscW(ch))
            If v = 64 Then Exit For             ' the padding ends the data
            If v < 0 Then Continue For          ' a newline or another character
            Dim which As Integer = CInt(index Mod 4)
            If which = 0 Then
                buffer = v
            ElseIf which = 1 Then
                buffer = (buffer << 6) Or v
                bytes.Add(CByte(buffer >> 4))
                buffer = buffer And 15
            ElseIf which = 2 Then
                buffer = (buffer << 6) Or v
                bytes.Add(CByte(buffer >> 2))
                buffer = buffer And 3
            Else
                buffer = (buffer << 6) Or v
                bytes.Add(CByte(buffer))
            End If
            index += 1
        Next
        Return bytes.ToArray()
    End Function

    Function Main(args As String()) As Integer
        If args.Length < 3 Then
            Console.WriteLine("Syntax: Base64 <e|d> <input_file> <output_file>")
            Return 1
        End If
        Dim mode As String = args(0).ToLower()
        If mode <> "e" AndAlso mode <> "d" Then
            Console.WriteLine("Mode can only be E or D!")
            Return 1
        End If
        If Not File.Exists(args(1)) Then
            Console.WriteLine("Input file open failed!")
            Return 2
        End If
        ' the binary side is read or written as bytes, the Base64 side as text
        If mode = "e" Then
            File.WriteAllText(args(2), Encode(File.ReadAllBytes(args(1))))
        Else
            File.WriteAllBytes(args(2), Decode(File.ReadAllText(args(1))))
        End If
        Return 0
    End Function
End Module
