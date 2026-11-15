' Renard2.vb
'
' Prints the Renard series of preferred numbers for every series size given
' on the command line.  Term n of the series S is (10 ^ (1/S)) ^ n.
' VB.NET version of documentation/projects/Renard2.asm.
'
' Usage:
'     Renard2 <series> [<series> ...]
'
' Example:
'     > Renard2 5 10
'     5: 1.00 1.58 2.51 3.98 6.31
'     10: 1.00 1.26 1.58 2.00 2.51 3.16 3.98 5.01 6.31 7.94
'
' Author:   Richard Lesh
Module Renard2
    ' prints "S: t0 t1 ..." for the series of size s
    Sub PrintRenard(s As Long)
        Dim factor As Double = Math.Pow(10.0, 1.0 / s)
        Console.Write("{0}:", s)
        For i As Long = 0 To s - 1
            Console.Write(" {0:F2}", Math.Pow(factor, CDbl(i)))
        Next
        Console.WriteLine()
    End Sub

    Function Main(args As String()) As Integer
        If args.Length < 1 Then
            Console.WriteLine("You must supply a series value 5, 10, 20 or 40")
            Return 1
        End If
        For Each a As String In args
            PrintRenard(Long.Parse(a))
        Next
        Return 0
    End Function
End Module
