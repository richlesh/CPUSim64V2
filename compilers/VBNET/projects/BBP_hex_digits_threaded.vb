' BBP_hex_digits_threaded.vb
'
' Computes 100 blocks of 32 hexadecimal digits of pi, starting at position n,
' n + 32, n + 64, ..., with the Bailey-Borwein-Plouffe digit extraction
' formula (see BBP_hex_digits.vb).  One thread per processor core takes the
' next block with Interlocked.Increment until all 100 are done; the results
' are written to a shared array and printed in order by the main thread.
' VB.NET version of documentation/projects/BBP_hex_digits_threaded.asm.
'
' Usage:
'     BBP_hex_digits_threaded <n>      (n = 1 is the first hex digit after the point)
'
' Example:
'     > BBP_hex_digits_threaded 1
'     Number of threads: 8
'     Starting 0
'     ...
'     Position 1: 243F6A8885A308D313198A2E03707344
'     Position 33: A4093822299F31D0082EFA98EC4E6C89
'     ...
'
' Author:   Richard Lesh
Imports System.Threading

Module BBP_hex_digits_threaded
    Const MaxPosition As Long = 10000000
    Const NumSlots As Integer = 100             ' the positions n, n+32, ..., n+32*99
    Const SlotStride As Long = 32               ' the 32 digits of a slot are followed by those of the next
    Const MaxStart As Long = MaxPosition - SlotStride * (NumSlots - 1)
    Const Low32 As ULong = &HFFFFFFFFUL

    ' a fraction in [0, 1): Hi holds the first 64 bits
    Class U192
        Public Hi As ULong
        Public Mid As ULong
        Public Lo As ULong
    End Class

    ' a + b + cin (cin is 0 or 1) modulo 2^64; carry is the carry out
    Function AddC(a As ULong, b As ULong, cin As ULong, ByRef carry As ULong) As ULong
        Dim lo As ULong = (a And Low32) + (b And Low32) + cin
        Dim hi As ULong = (a >> 32) + (b >> 32) + (lo >> 32)
        carry = hi >> 32
        Return ((hi And Low32) << 32) Or (lo And Low32)
    End Function

    ' num / den as a 192-bit fraction: long division 32 bits at a time
    Sub Frac192(num As ULong, den As ULong, f As U192)
        Dim r As ULong = num
        f.Hi = Next64(r, den)
        f.Mid = Next64(r, den)
        f.Lo = Next64(r, den)
    End Sub

    ' the next 64 bits of the fraction r / den (r < den), and the remainder in r
    Function Next64(ByRef r As ULong, den As ULong) As ULong
        Dim a As ULong = (r << 32) \ den
        r = (r << 32) Mod den
        Dim b As ULong = (r << 32) \ den
        r = (r << 32) Mod den
        Return (a << 32) Or b
    End Function

    Sub Add192(a As U192, b As U192)
        Dim c1 As ULong, c2 As ULong, c3 As ULong
        a.Lo = AddC(a.Lo, b.Lo, 0UL, c1)
        a.Mid = AddC(a.Mid, b.Mid, c1, c2)
        a.Hi = AddC(a.Hi, b.Hi, c2, c3)
    End Sub

    ' a - b: a + (not b) + 1
    Sub Sub192(a As U192, b As U192)
        Dim c1 As ULong, c2 As ULong, c3 As ULong
        a.Lo = AddC(a.Lo, Not b.Lo, 1UL, c1)
        a.Mid = AddC(a.Mid, Not b.Mid, c1, c2)
        a.Hi = AddC(a.Hi, Not b.Hi, c2, c3)
    End Sub

    ' shifts left by n (0 < n < 64)
    Sub Shl192(a As U192, n As Integer)
        a.Hi = (a.Hi << n) Or (a.Mid >> (64 - n))
        a.Mid = (a.Mid << n) Or (a.Lo >> (64 - n))
        a.Lo = a.Lo << n
    End Sub

    ' 16^e mod m
    Function PowMod16(e As ULong, m As ULong) As ULong
        Dim r As ULong = 1
        If m = 1UL Then Return 0
        Dim b As ULong = 16UL Mod m
        While e > 0UL
            If (e And 1UL) <> 0UL Then r = r * b Mod m
            b = b * b Mod m
            e >>= 1
        End While
        Return r
    End Function

    ' the fractional part of 16^d * S(j)
    Function Series(j As Long, d As Long) As U192
        Dim s As New U192()
        Dim t As New U192()
        For k As Long = 0 To d
            Dim m As Long = 8 * k + j
            Frac192(PowMod16(CULng(d - k), CULng(m)), CULng(m), t)
            Add192(s, t)
        Next
        ' the tail: 16^-t / m for the next terms, shifted right by 4t bits
        For n As Long = 1 To 47
            Dim m As Long = 8 * (d + n) + j
            Frac192(1UL, CULng(m), t)
            Dim sh As Integer = CInt(4 * n)
            While sh >= 64
                t.Lo = t.Mid
                t.Mid = t.Hi
                t.Hi = 0
                sh -= 64
            End While
            If sh <> 0 Then
                t.Lo = (t.Lo >> sh) Or (t.Mid << (64 - sh))
                t.Mid = (t.Mid >> sh) Or (t.Hi << (64 - sh))
                t.Hi >>= sh
            End If
            Add192(s, t)
        Next
        Return s
    End Function

    Dim slot As Integer = -1                    ' the last block taken
    Dim firstPos As Long                        ' the first position
    Dim results(2 * NumSlots - 1) As ULong      ' 2 words (the 32 digits) for each block

    ' a thread: computes blocks until there are none left
    Sub Worker()
        Do
            Dim s As Integer = Interlocked.Increment(slot)      ' the next block
            If s >= NumSlots Then Exit Do
            Console.WriteLine("Starting {0}", s)
            Dim pos As Long = firstPos + s * SlotStride
            Dim d As Long = pos - 1
            Dim acc As U192 = Series(1, d)
            Shl192(acc, 2)                  ' 4 * S(1)
            Dim t As U192 = Series(4, d)
            Shl192(t, 1)                    ' 2 * S(4)
            Sub192(acc, t)
            Sub192(acc, Series(5, d))
            Sub192(acc, Series(6, d))
            results(2 * s) = acc.Hi
            results(2 * s + 1) = acc.Mid
            ' the top 32 guard bits: all 0s or all 1s mean the last digits may be off by one
            Dim guard As ULong = acc.Lo >> 32
            If guard = 0UL Then
                Console.WriteLine("Warning: position {0} guard bits are all 0s; the last digit(s) may be 1 too high.", pos)
            ElseIf guard = Low32 Then
                Console.WriteLine("Warning: position {0} guard bits are all 1s; the last digit(s) may be 1 too low.", pos)
            End If
        Loop
    End Sub

    Function Main(args As String()) As Integer
        If args.Length <> 1 Then
            Console.WriteLine("Usage: BBP_hex_digits <n>   (n = 1 is the first hex digit after the point)")
            Return 1
        End If
        firstPos = Long.Parse(args(0))
        If firstPos < 1 Then
            Console.WriteLine("n must be >= 1")
            Return 1
        End If
        If firstPos > MaxStart Then
            Console.WriteLine("n must be <= {0}", MaxStart)
            Return 1
        End If
        Dim cores As Integer = Environment.ProcessorCount
        Console.WriteLine("Number of threads: {0}", cores)
        Dim threads(cores - 1) As Thread
        For i As Integer = 0 To cores - 1
            threads(i) = New Thread(AddressOf Worker)
            threads(i).Start()
        Next
        For i As Integer = 0 To cores - 1
            threads(i).Join()
        Next
        For i As Integer = 0 To NumSlots - 1
            Console.WriteLine("Position {0}: {1:X16}{2:X16}", firstPos + i * SlotStride, results(2 * i), results(2 * i + 1))
        Next
        Return 0
    End Function
End Module
