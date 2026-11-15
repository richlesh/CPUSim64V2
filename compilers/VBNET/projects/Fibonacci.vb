' Fibonacci.vb
'
' Prints the first 45 numbers of the Fibonacci sequence.
' VB.NET version of documentation/projects/Fibonacci.asm.
'
' Author:   Richard Lesh
' Original: 2009/03/20
Module Fibonacci
    Sub Main()
        Dim a As Long = 1
        Dim b As Long = 1
        Console.WriteLine(a)
        Console.WriteLine(b)
        For i As Integer = 0 To 42
            Dim c As Long = a + b
            Console.WriteLine(c)
            b = a
            a = c
        Next
    End Sub
End Module
