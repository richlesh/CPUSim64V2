' Hailstone2.vb
'
' Finds the starting number up to a limit that has the longest Hailstone
' (Collatz) sequence: halve a number when it is even, otherwise triple it and
' add 1, until it reaches 1.
' VB.NET version of documentation/projects/Hailstone2.asm.
'
' Usage:
'     Hailstone2 <limit>
'
' Output:
'     "<number>: <length>"
'
' Author:   Richard Lesh
' Original: 2009/03/20
Module Hailstone2
    Function ComputeHailstone(n As Long) As Long
        If n <= 1 Then Return 1
        If (n And 1) = 0 Then Return ComputeHailstone(n \ 2) + 1
        Return ComputeHailstone(3 * n + 1) + 1
    End Function

    Function Main(args As String()) As Integer
        If args.Length < 1 Then
            Console.WriteLine("You must supply a positive integer argument.")
            Return 1
        End If
        Dim limit As Long = Long.Parse(args(0))
        Dim imax As Long = 1, max As Long = 1
        For i As Long = 2 To limit
            Dim length As Long = ComputeHailstone(i)
            If length > max Then
                imax = i
                max = length
            End If
        Next
        Console.WriteLine("{0}: {1}", imax, max)
        Return 0
    End Function
End Module
