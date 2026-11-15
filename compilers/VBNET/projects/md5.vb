' md5.vb
'
' Prints the MD5 digest of a file (RFC 1321).  The 32-bit words are kept in
' Longs and masked after every operation, because VB.NET checks integer
' overflow.
' VB.NET version of documentation/projects/md5.asm.
'
' Usage:
'     md5 [<file>]
'
' Without a file name the lines of standard input are hashed, each followed
' by a newline.
'
' Author:   Richard Lesh
Imports System.IO
Imports System.Text

Module md5
    Const M32 As Long = &HFFFFFFFFL

    ReadOnly Shift As Integer() = {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21}

    ' KTable(i) = floor(2^32 * |sin(i + 1)|)
    ReadOnly KTable As Long() = {
        &HD76AA478L, &HE8C7B756L, &H242070DBL, &HC1BDCEEEL, &HF57C0FAFL, &H4787C62AL, &HA8304613L, &HFD469501L,
        &H698098D8L, &H8B44F7AFL, &HFFFF5BB1L, &H895CD7BEL, &H6B901122L, &HFD987193L, &HA679438EL, &H49B40821L,
        &HF61E2562L, &HC040B340L, &H265E5A51L, &HE9B6C7AAL, &HD62F105DL, &H2441453L, &HD8A1E681L, &HE7D3FBC8L,
        &H21E1CDE6L, &HC33707D6L, &HF4D50D87L, &H455A14EDL, &HA9E3E905L, &HFCEFA3F8L, &H676F02D9L, &H8D2A4C8AL,
        &HFFFA3942L, &H8771F681L, &H6D9D6122L, &HFDE5380CL, &HA4BEEA44L, &H4BDECFA9L, &HF6BB4B60L, &HBEBFBC70L,
        &H289B7EC6L, &HEAA127FAL, &HD4EF3085L, &H4881D05L, &HD9D4D039L, &HE6DB99E5L, &H1FA27CF8L, &HC4AC5665L,
        &HF4292244L, &H432AFF97L, &HAB9423A7L, &HFC93A039L, &H655B59C3L, &H8F0CCC92L, &HFFEFF47DL, &H85845DD1L,
        &H6FA87E4FL, &HFE2CE6E0L, &HA3014314L, &H4E0811A1L, &HF7537E82L, &HBD3AF235L, &H2AD7D2BBL, &HEB86D391L}

    Dim h(3) As Long

    ' processes the 64 bytes of data starting at offset
    Sub Md5Block(data As Integer(), offset As Integer)
        Dim m(15) As Long
        Dim a As Long = h(0), b As Long = h(1), c As Long = h(2), d As Long = h(3)
        For i As Integer = 0 To 15
            Dim p As Integer = offset + 4 * i
            m(i) = CLng(data(p)) Or (CLng(data(p + 1)) << 8) Or (CLng(data(p + 2)) << 16) Or (CLng(data(p + 3)) << 24)
        Next
        For i As Integer = 0 To 63
            Dim f As Long, g As Integer
            If i < 16 Then
                f = ((c Xor d) And b) Xor d
                g = i
            ElseIf i < 32 Then
                f = ((b Xor c) And d) Xor c
                g = (5 * i + 1) Mod 16
            ElseIf i < 48 Then
                f = b Xor c Xor d
                g = (3 * i + 5) Mod 16
            Else
                f = c Xor (b Or ((Not d) And M32))
                g = (7 * i) Mod 16
            End If
            f = (f + a + KTable(i) + m(g)) And M32
            a = d
            d = c
            c = b
            b = (b + (((f << Shift(i)) Or (f >> (32 - Shift(i)))) And M32)) And M32
        Next
        h(0) = (h(0) + a) And M32
        h(1) = (h(1) + b) And M32
        h(2) = (h(2) + c) And M32
        h(3) = (h(3) + d) And M32
    End Sub

    Sub ComputeMd5(bytes As Byte())
        h(0) = &H67452301L
        h(1) = &HEFCDAB89L
        h(2) = &H98BADCFEL
        h(3) = &H10325476L
        ' the data, then the padding: 0x80, zeros up to 56 bytes mod 64, the length in bits (little endian)
        Dim len As Integer = bytes.Length
        Dim total As Integer = ((len + 8) \ 64 + 1) * 64
        Dim data(total - 1) As Integer
        For i As Integer = 0 To len - 1
            data(i) = bytes(i)
        Next
        data(len) = &H80
        Dim bits As Long = CLng(len) * 8
        For i As Integer = 0 To 7
            data(total - 8 + i) = CInt((bits >> (8 * i)) And 255L)
        Next
        For offset As Integer = 0 To total - 64 Step 64
            Md5Block(data, offset)
        Next
    End Sub

    ' the digest: the bytes of each word, lowest first
    Function Digest() As String
        Dim sb As New StringBuilder()
        For i As Integer = 0 To 3
            For j As Integer = 0 To 3
                sb.Append(((h(i) >> (8 * j)) And 255L).ToString("X2"))
            Next
        Next
        Return sb.ToString()
    End Function

    Function Main(args As String()) As Integer
        If args.Length < 1 Then
            ' the lines of standard input, each followed by a newline (UTF-8 is not decoded)
            Dim input As New List(Of Byte)
            Dim line As String = Console.ReadLine()
            While line IsNot Nothing
                For Each ch As Char In line
                    input.Add(CByte(AscW(ch) And 255))
                Next
                input.Add(10)
                line = Console.ReadLine()
            End While
            ComputeMd5(input.ToArray())
            Console.WriteLine()
            Console.WriteLine(Digest())
            Return 0
        End If
        If Not File.Exists(args(0)) Then
            Console.WriteLine("File open failed!")
            Return 1
        End If
        ComputeMd5(File.ReadAllBytes(args(0)))
        Console.WriteLine()
        Console.WriteLine("{0} {1}", Digest(), args(0))
        Return 0
    End Function
End Module
