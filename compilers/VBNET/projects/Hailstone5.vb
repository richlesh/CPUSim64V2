' Hailstone5.vb
'
' Multi-threaded search for the starting number up to a limit that has the
' longest Hailstone (Collatz) sequence.  Unlike Hailstone4 (one fixed share
' per thread), the range is cut into work units of Worksize values and every
' thread, one per processor core, takes the next unit with
' Interlocked.Increment until none are left, so the threads stay busy to the
' end.  The memo cache (CacheSize entries) is shared by all of them.
' VB.NET version of documentation/projects/Hailstone5.asm.
'
' Usage:
'     Hailstone5 <limit>
'
' Output:
'     the number of cores, a line for each work unit and new high, then
'     "Max <length> found at <i>".
'
' Author:   Richard Lesh
Imports System.Threading

Module Hailstone5
    Const CacheSize As Integer = 8000000        ' number of memoized entries
    Const Worksize As Long = 100000             ' starting values per work unit

    Class BestValue                                  ' the best length so far, and its starting value
        Public Max As Long
        Public IMax As Long
    End Class

    Dim best As New BestValue()
    Dim nextUnit As Long = -1                   ' the last work unit taken
    Dim limit As Long
    Dim cache As Long()

    Function ComputeHailstone(n As Long) As Long
        If n < CacheSize AndAlso cache(CInt(n)) <> 0 Then Return cache(CInt(n))
        Dim length As Long
        If (n And 1) = 0 Then
            length = ComputeHailstone(n \ 2) + 1
        Else
            length = ComputeHailstone(3 * n + 1) + 1
        End If
        If n < CacheSize Then cache(CInt(n)) = length
        Return length
    End Function

    Sub Worker()
        Dim pid As Integer = Thread.CurrentThread.ManagedThreadId
        Do
            Dim d As Long = Interlocked.Increment(nextUnit) * Worksize     ' the next work unit
            If d >= limit Then Exit Do
            Dim last As Long = Math.Min(d + Worksize, limit)
            Dim wmax As Long = 0, wimax As Long = 0
            Console.WriteLine("Thread work unit {0} executing with PID {1}...", d, pid)
            For i As Long = d To last - 1
                Dim hs As Long = ComputeHailstone(i)
                If hs > wmax Then
                    wmax = hs
                    wimax = i
                End If
            Next
            SyncLock best
                If wmax > best.Max OrElse (wmax = best.Max AndAlso wimax < best.IMax) Then
                    Console.WriteLine("New high: {0}:{1} ({2})", wimax, wmax, pid)
                    best.Max = wmax
                    best.IMax = wimax
                End If
            End SyncLock
        Loop
        Console.WriteLine("Thread {0} finishing...", pid)
    End Sub

    Function Main(args As String()) As Integer
        If args.Length < 1 Then
            Console.WriteLine("You must supply a positive integer argument.")
            Return 1
        End If
        limit = Long.Parse(args(0))
        If limit < 2 Then
            Console.WriteLine("Argument must be >= 2!")
            Return 1
        End If
        ReDim cache(CacheSize - 1)
        cache(0) = 1
        cache(1) = 1
        cache(2) = 2
        cache(3) = 8
        cache(4) = 3
        cache(5) = 6
        Dim cores As Integer = Environment.ProcessorCount
        Console.WriteLine("Number of cores: {0}", cores)
        Dim threads(cores - 1) As Thread
        For i As Integer = 0 To cores - 1
            threads(i) = New Thread(AddressOf Worker)
            threads(i).Start()
        Next
        For i As Integer = 0 To cores - 1
            Console.WriteLine("Main is joining {0}...", threads(i).ManagedThreadId)
            threads(i).Join()
        Next
        Console.WriteLine("Max {0} found at {1}", best.Max, best.IMax)
        Return 0
    End Function
End Module
