' Hailstone3.vb
'
' Finds the starting number up to a limit that has the longest Hailstone
' (Collatz) sequence.  The lengths of starting values below CacheSize are
' memoized in an array, so each value is computed once.  A progress line is
' printed every 1000 candidates.
' VB.NET version of documentation/projects/Hailstone3.asm.
'
' Usage:
'     Hailstone3 <limit>
'
' Output:
'     "<i>...<imax>:<max>" every 1000 candidates, then "<imax>: <max>".
'
' Author:   Richard Lesh
' Original: 2009/03/20
Module Hailstone3
    Const CacheSize As Integer = 3000000        ' number of memoized entries

    Dim cache As Long()                         ' cache(n) = length for n, 0 = not computed yet

    Function ComputeHailstone(n As Long) As Long
        Dim length As Long
        If cache Is Nothing Then
            ReDim cache(CacheSize - 1)
            cache(1) = 1                        ' the sequence of 1 has length 1
        End If
        If n < CacheSize AndAlso cache(CInt(n)) <> 0 Then Return cache(CInt(n))
        If (n And 1) = 0 Then
            length = ComputeHailstone(n \ 2) + 1
        Else
            length = ComputeHailstone(3 * n + 1) + 1
        End If
        If n < CacheSize Then cache(CInt(n)) = length
        Return length
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
            If i Mod 1000 = 0 Then Console.WriteLine("{0}...{1}:{2}", i, imax, max)
        Next
        Console.WriteLine("{0}: {1}", imax, max)
        Return 0
    End Function
End Module
