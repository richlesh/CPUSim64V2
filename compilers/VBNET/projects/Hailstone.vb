' Hailstone.vb
'
' Prints the length of the Hailstone (Collatz) sequence of a number: halve it
' when it is even, otherwise triple it and add 1, until it reaches 1.
' VB.NET version of documentation/projects/Hailstone.asm.
'
' Usage:
'     Hailstone [<n>]
'
' With no argument the lengths for 1 to 100 are printed and the result is 1.
'
' Author:   Richard Lesh
' Original: 2009/03/20
Module Hailstone
    Function ComputeHailstone(n As Long) As Long
        If n <= 1 Then Return 1
        If (n And 1) = 0 Then Return ComputeHailstone(n \ 2) + 1
        Return ComputeHailstone(3 * n + 1) + 1
    End Function

    Function Main(args As String()) As Integer
        If args.Length >= 1 Then
            Console.WriteLine(ComputeHailstone(Long.Parse(args(0))))
            Return 0
        End If
        For i As Long = 1 To 100
            Console.WriteLine("{0}: {1}", i, ComputeHailstone(i))
        Next
        Return 1
    End Function
End Module
