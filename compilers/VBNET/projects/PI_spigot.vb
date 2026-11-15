' PI_spigot.vb
'
' Prints n digits of pi after the decimal point with the Rabinowitz-Wagon
' spigot algorithm, which produces the digits one after the other with
' integer arithmetic only.  A few guard digits are computed beyond n, and
' runs of nines are held back until the carry that decides them is known.
' VB.NET version of documentation/projects/PI_spigot.asm.
'
' Usage:
'     PI_spigot <n>      (n = number of digits after the decimal point)
'
' Example:
'     > PI_spigot 10
'     3.1415926535
'
' Author:   Richard Lesh
Module PI_spigot
    Const GuardDigits As Integer = 10

    Dim emitIndex As Long = -2          ' the index of the next digit: -1 is the predigit 0, 0 is the 3
    Dim emitEnd As Long                 ' the digits with index < emitEnd are printed

    Sub EmitDigit(d As Long)
        emitIndex += 1
        If emitIndex >= 0 AndAlso emitIndex < emitEnd Then
            Console.Write(ChrW(48 + CInt(d)))
            If emitIndex = 0 Then Console.Write("."c)
        End If
    End Sub

    Sub EmitRun(d As Long, count As Long)
        For i As Long = 1 To count
            EmitDigit(d)
        Next
    End Sub

    Function Main(args As String()) As Integer
        If args.Length <> 1 Then
            Console.WriteLine("Usage: PI_spigot <n>   (n = number of digits after the decimal point)")
            Return 1
        End If
        Dim n As Long = Long.Parse(args(0))
        If n < 1 Then
            Console.WriteLine("n must be >= 1")
            Return 1
        End If
        emitEnd = n + 1
        Dim total As Long = n + 1 + GuardDigits
        Dim length As Long = total * 10 \ 3 + 1
        Dim a(CInt(length)) As Long
        Dim predigit As Long = 0, nines As Long = 0
        For i As Long = 1 To length
            a(CInt(i)) = 2
        Next
        For j As Long = 0 To total - 1
            Dim q As Long = 0
            For i As Long = length To 1 Step -1
                Dim den As Long = 2 * i - 1
                Dim x As Long = a(CInt(i)) * 10 + q * i
                q = x \ den
                a(CInt(i)) = x Mod den
            Next
            a(1) = q Mod 10
            q = q \ 10
            If q = 9 Then
                nines += 1
            ElseIf q = 10 Then
                EmitDigit(predigit + 1)
                EmitRun(0, nines)
                predigit = 0
                nines = 0
            Else
                EmitDigit(predigit)
                predigit = q
                EmitRun(9, nines)
                nines = 0
            End If
            If emitIndex + 1 >= emitEnd Then Exit For
        Next
        Console.WriteLine()
        Return 0
    End Function
End Module
