' CAS_printf_stress.vb
'
' Stress test of the atomic operations and of Console output with several
' threads (one per processor core).
'
' Test 1: every thread adds CasIters to a counter with Interlocked.Increment
'         and CasIters to a second counter with its own compare-and-swap loop
'         (Interlocked.CompareExchange).  No increment may be lost, so each
'         counter must end at CasIters * threads.
' Test 2: after a pause every thread prints PrintIters lines.  Console is
'         thread-safe, as in .NET: the lines of the threads are interleaved
'         but never mixed within a line.
' VB.NET version of documentation/projects/CAS_printf_stress.asm.
'
' Usage:
'     CAS_printf_stress
'
' Output:
'     "PASS ..." or "FAIL ..." for each counter, then the printed lines.
'     The result is the number of failed counters.
'
' Author:   Richard Lesh
Imports System.Threading

Module CAS_printf_stress
    Const CasIters As Integer = 100000
    Const PrintIters As Integer = 1000
    Const PauseMs As Integer = 10000

    Dim libCounter As Long              ' incremented with Interlocked.Increment
    Dim rawCounter As Long              ' incremented with a CAS loop

    ' waits ms milliseconds; with the environment variable NO_DELAY set it returns at once (for the tests)
    Sub Delay(ms As Integer)
        If Environment.GetEnvironmentVariable("NO_DELAY") Is Nothing Then Thread.Sleep(ms)
    End Sub

    Sub CasWorker(arg As Object)
        For i As Integer = 1 To CasIters
            Interlocked.Increment(libCounter)
        Next
        For i As Integer = 1 To CasIters
            Dim old As Long
            Do
                old = rawCounter
            Loop While Interlocked.CompareExchange(rawCounter, old + 1, old) <> old
        Next
    End Sub

    Sub PrintWorker(arg As Object)
        Dim t As Integer = CInt(arg)
        For i As Integer = 0 To PrintIters - 1
            Dim key As Integer = t * 10000 + i
            Console.WriteLine("T{0} L{1} K{2} abcdefghijklmnopqrstuvwxyz END", t, i, key)
        Next
    End Sub

    ' runs count threads of fn, with the thread number as their argument; returns how many started
    Function RunThreads(fn As ParameterizedThreadStart, count As Integer) As Integer
        Dim threads(count - 1) As Thread
        For i As Integer = 0 To count - 1
            threads(i) = New Thread(fn)
            threads(i).Start(i)
        Next
        For i As Integer = 0 To count - 1
            threads(i).Join()
        Next
        Return count
    End Function

    Function Main() As Integer
        Dim cores As Integer = Environment.ProcessorCount
        Dim failures As Integer = 0
        Console.WriteLine("Threads: {0}", cores)
        Console.WriteLine("=== CAS TEST BEGIN ===")
        Dim started As Integer = RunThreads(AddressOf CasWorker, cores)
        Dim expected As Long = CLng(CasIters) * started
        Dim actual As Long = libCounter
        If actual = expected Then
            Console.WriteLine("PASS Interlocked.Increment: {0}", actual)
        Else
            Console.WriteLine("FAIL Interlocked.Increment: expected {0}, got {1} (lost {2})", expected, actual, expected - actual)
            failures += 1
        End If
        actual = rawCounter
        If actual = expected Then
            Console.WriteLine("PASS raw CAS loop: {0}", actual)
        Else
            Console.WriteLine("FAIL raw CAS loop: expected {0}, got {1} (lost {2})", expected, actual, expected - actual)
            failures += 1
        End If
        Console.WriteLine("=== CAS TEST END ===")
        Console.WriteLine("Pausing 10 seconds...")
        Delay(PauseMs)
        Console.WriteLine("=== PRINTF TEST BEGIN (expect {0} lines) ===", PrintIters * cores)
        started = RunThreads(AddressOf PrintWorker, cores)
        Console.WriteLine("=== PRINTF TEST END ({0} threads started) ===", started)
        Return failures
    End Function
End Module
