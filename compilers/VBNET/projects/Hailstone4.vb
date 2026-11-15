' Hailstone4.vb
'
' Multi-threaded search for the starting number up to a limit that has the
' longest Hailstone (Collatz) sequence.  The range is split into one equal
' work unit per processor core; the main thread does the first unit and a
' thread does each of the others.  The sequence lengths of the values below
' CacheSize are memoized in an array that all the threads share (two
' threads may compute the same entry, but they store the same value).  Each
' thread reports its best value under a lock.
' VB.NET version of documentation/projects/Hailstone4.asm.
'
' Usage:
'     Hailstone4 <limit>
'
' Output:
'     the number of cores, the work size, a line for each thread starting,
'     finishing and finding a new high, then "Max <length> found at <i>".
'
' Author:   Richard Lesh
Imports System.Threading

Module Hailstone4
    Const CacheSize As Integer = 3000000        ' number of memoized entries

    Class BestValue                                  ' the best length so far, and its starting value
        Public Max As Long
        Public IMax As Long
    End Class

    Dim best As New BestValue()
    Dim worksize As Long = 10000                ' starting values per thread
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

    ' handles the starting values first .. first + worksize - 1
    Sub Worker(data As Object)
        Dim first As Long = CLng(data)
        Dim last As Long = Math.Min(first + worksize, limit)
        Dim pid As Integer = Thread.CurrentThread.ManagedThreadId
        Dim wmax As Long = 0, wimax As Long = 0
        Console.WriteLine("Thread work unit {0} executing with PID {1}...", first, pid)
        For i As Long = first To last - 1
            Dim hs As Long = ComputeHailstone(i)
            If hs > wmax Then
                wmax = hs
                wimax = i
            End If
        Next
        SyncLock best
            ' a longer sequence wins; of two equal ones the smaller starting value
            If wmax > best.Max OrElse (wmax = best.Max AndAlso wimax < best.IMax) Then
                Console.WriteLine("New high: {0}:{1} ({2})", wimax, wmax, pid)
                best.Max = wmax
                best.IMax = wimax
            End If
        End SyncLock
        Console.WriteLine("Thread {0} finishing...", pid)
    End Sub

    Function Main(args As String()) As Integer
        Dim cores As Integer = Environment.ProcessorCount
        Console.WriteLine("Number of cores: {0}", cores)
        If args.Length < 1 Then
            Console.WriteLine("You must supply a positive integer argument.")
            Return 1
        End If
        limit = Long.Parse(args(0))
        If limit < 2 Then
            Console.WriteLine("Argument must be > 2!")
            Return 1
        End If
        worksize = (limit + cores - 1) \ cores      ' ceil(limit / cores)
        Console.WriteLine("Worksize: {0}", worksize)
        ReDim cache(CacheSize - 1)
        cache(0) = 1
        cache(1) = 1
        cache(2) = 2
        cache(3) = 8
        cache(4) = 3
        cache(5) = 6
        Worker(0L)                                  ' the main thread does the first unit
        Dim threads(cores - 1) As Thread
        Dim j As Long = worksize
        For i As Integer = 1 To cores - 1
            threads(i) = New Thread(AddressOf Worker)
            threads(i).Start(j)
            j += worksize
        Next
        For i As Integer = 1 To cores - 1
            Console.WriteLine("Main is joining {0}...", threads(i).ManagedThreadId)
            threads(i).Join()
        Next
        Console.WriteLine("Max {0} found at {1}", best.Max, best.IMax)
        Return 0
    End Function
End Module
