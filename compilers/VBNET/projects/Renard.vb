' Renard.vb
'
' Computes and prints a Renard series of preferred numbers.  The R5, R10,
' R20 and R40 series divide a decade into 5, 10, 20 or 40 equal logarithmic
' steps; term n of the series S is (10 ^ (1/S)) ^ n.
' VB.NET version of documentation/projects/Renard.asm.
'
' Usage:
'     Renard <series>
'
' Example:
'     > Renard 10
'     1.00 1.26 1.58 2.00 2.51 3.16 3.98 5.01 6.31 7.94
'
' Author:   Richard Lesh
' Original: 2022/11/28
Module Renard
    Function Main(args As String()) As Integer
        If args.Length <> 1 Then
            Console.WriteLine("You must supply a series value 5, 10, 20 or 40")
            Return 1
        End If
        Dim series As Long = Long.Parse(args(0))
        If series < 2 Then
            Console.WriteLine("You must supply a series value > 2")
            Return 2
        End If
        Dim factor As Double = Math.Pow(10.0, 1.0 / series)     ' the common ratio
        For i As Long = 0 To series - 1
            Console.Write("{0:F2} ", Math.Pow(factor, CDbl(i)))
        Next
        Console.WriteLine()
        Return 0
    End Function
End Module
