' System.Threading: Thread, Interlocked, Monitor and SyncLock, exceptions in
' threads, allocation while threads run; CPUSim64.Process.Fork; File bytes
Imports System.Threading
Imports System.IO

Module Threads
    Class LockBox
        Public Count As Long
    End Class

    Dim counter As Integer
    Dim total As Long
    Dim box As New LockBox()
    Dim words As New List(Of String)

    Sub Work()
        For i As Integer = 1 To 500
            Interlocked.Increment(counter)
            Interlocked.Add(total, 3)
            SyncLock box
                box.Count += 1
                SyncLock box                ' recursive
                    box.Count += 1
                End SyncLock
            End SyncLock
            Dim s As String = "w" & i.ToString()        ' allocates while other threads run
            If i Mod 100 = 0 Then
                SyncLock words
                    words.Add(s)
                End SyncLock
            End If
        Next
    End Sub

    Sub WithArg(o As Object)
        Interlocked.Add(total, CLng(o))
    End Sub

    Function Main() As Integer
        Dim ts As New List(Of Thread)
        For i As Integer = 1 To 4
            Dim t As New Thread(AddressOf Work)
            t.Start()
            ts.Add(t)
        Next
        For Each t As Thread In ts
            t.Join()
        Next
        Console.WriteLine("counter {0} total {1} box {2} words {3}", counter, total, box.Count, words.Count)

        Dim p As New Thread(AddressOf WithArg)
        p.Start(1000L)
        p.Join()
        Console.WriteLine("total {0}", total)

        ' an exception caught in a thread, and one that leaves a SyncLock
        Dim results As New List(Of String)
        Dim lam As New Thread(Sub()
                                  Try
                                      SyncLock box
                                          Throw New InvalidOperationException("inside")
                                      End SyncLock
                                  Catch ex As InvalidOperationException
                                      SyncLock results
                                          results.Add("caught " & ex.Message & " in a thread: " & (Thread.CurrentThread.ManagedThreadId <> 1).ToString())
                                      End SyncLock
                                  End Try
                              End Sub)
        lam.Start()
        lam.Join()
        Console.WriteLine(results(0))
        Console.WriteLine("lock free again: {0}", Monitor.TryEnter(box))
        Monitor.Exit(box)
        Try
            Monitor.Exit(box)
        Catch ex As InvalidOperationException
            Console.WriteLine("Exit without Enter: " & ex.Message)
        End Try
        Try
            lam.Start()
        Catch ex As ThreadStateException
            Console.WriteLine("restart: " & ex.Message)
        End Try

        ' the main thread's handlers still work
        Try
            Throw New ArgumentException("main")
        Catch ex As ArgumentException
            Console.WriteLine("main caught " & ex.Message)
        End Try

        Dim x As Integer = 5
        Dim old As Integer = Interlocked.CompareExchange(x, 7, 5)
        Console.WriteLine("{0} {1}", old, x)
        old = Interlocked.CompareExchange(x, 9, 5)
        Console.WriteLine("{0} {1}", old, x)
        old = Interlocked.Exchange(x, 11)
        Console.WriteLine("{0} {1} {2}", old, x, Interlocked.Decrement(x))
        Dim big As Integer = Integer.MaxValue
        Console.WriteLine(Interlocked.Increment(big))           ' wraps, as in .NET

        Thread.Sleep(5)
        Console.WriteLine("cores > 0: {0}, main id {1}", Environment.ProcessorCount > 0, Thread.CurrentThread.ManagedThreadId)
        Console.WriteLine("unset variable: {0}", Environment.GetEnvironmentVariable("CPUSIM64_NO_SUCH_VARIABLE") Is Nothing)
        GC.Collect()                                            ' the collector runs again

        Dim pid As Integer = CPUSim64.Process.Fork()
        If pid = 0 Then
            Console.WriteLine("child: counter {0}", counter)
            counter = 0
            Environment.Exit(0)
        End If
        CPUSim64.Process.Wait(pid)
        Console.WriteLine("parent: counter {0}", counter)

        Dim path As String = "threads_test.tmp"
        File.WriteAllBytes(path, New Byte() {0, 1, 128, 255, 10, 13})
        Dim b As Byte() = File.ReadAllBytes(path)
        Console.WriteLine("{0} bytes: {1} {2} {3} {4}", b.Length, b(0), b(2), b(3), b(5))
        File.Delete(path)
        Return 0
    End Function
End Module
