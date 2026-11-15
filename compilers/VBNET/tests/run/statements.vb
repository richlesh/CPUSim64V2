Imports System.Collections

' A collection with the For Each pattern (GetEnumerator, MoveNext, Current)
Class Countdown
    Private ReadOnly from As Integer
    Public Sub New(from As Integer)
        Me.from = from
    End Sub
    Public Function GetEnumerator() As CountdownEnumerator
        Return New CountdownEnumerator(from)
    End Function
End Class

Class CountdownEnumerator
    Implements IDisposable
    Private n As Integer
    Public Sub New(n As Integer)
        Me.n = n + 1
    End Sub
    Public Function MoveNext() As Boolean
        n -= 1
        Return n > 0
    End Function
    Public ReadOnly Property Current As Integer
        Get
            Return n
        End Get
    End Property
    Public Sub Dispose() Implements IDisposable.Dispose
        Console.WriteLine("(enumerator disposed)")
    End Sub
End Class

' A non-generic IEnumerable
Class Words
    Implements IEnumerable
    Private ReadOnly items() As String
    Public Sub New(text As String)
        items = text.Split(" "c)
    End Sub
    Public Function GetEnumerator() As IEnumerator Implements IEnumerable.GetEnumerator
        Return New WordEnumerator(items)
    End Function
End Class

Class WordEnumerator
    Implements IEnumerator
    Private ReadOnly items() As String
    Private pos As Integer = -1
    Public Sub New(items() As String)
        Me.items = items
    End Sub
    Public Function MoveNext() As Boolean Implements IEnumerator.MoveNext
        pos += 1
        Return pos < items.Length
    End Function
    Public ReadOnly Property Current As Object Implements IEnumerator.Current
        Get
            Return items(pos)
        End Get
    End Property
    Public Sub Reset() Implements IEnumerator.Reset
        pos = -1
    End Sub
End Class

Class Resource
    Implements IDisposable
    Public ReadOnly Name As String
    Public Sub New(name As String)
        Me.Name = name
        Console.WriteLine("open " & name)
    End Sub
    Public Sub Dispose() Implements IDisposable.Dispose
        Console.WriteLine("close " & Name)
    End Sub
End Class

Structure Settings
    Public Width As Integer
    Public Height As Integer
End Structure

Class Box
    Public Size As Settings
    Public Label As String
End Class

Module Program
    Function FirstOver(limit As Integer) As Integer
        For Each n In New Countdown(5)
            If n < limit Then Return n
        Next
        Return -1
    End Function

    Sub Main()
        For Each n In New Countdown(3)
            Console.Write(n & " ")
        Next
        Console.WriteLine()
        Console.WriteLine(FirstOver(3))
        For Each w As String In New Words("alpha beta gamma")
            If w = "beta" Then Continue For
            Console.Write(w.ToUpper() & " ")
        Next
        Console.WriteLine()
        Dim en As IEnumerable = New Words("x y")
        For Each o In en
            Console.Write("[" & o.ToString() & "]")
        Next
        Console.WriteLine()

        Using r As New Resource("A"), s As New Resource("B")
            Console.WriteLine("using " & r.Name & s.Name)
        End Using
        Try
            Using t As New Resource("C")
                Throw New InvalidOperationException("boom")
            End Using
        Catch ex As InvalidOperationException
            Console.WriteLine("caught " & ex.Message)
        End Try
        Dim res As New Resource("D")
        Using res
            Console.WriteLine("inside")
        End Using

        Dim b As New Box()
        With b
            .Label = "box"
            .Size.Width = 3
            With .Size
                .Height = 4
            End With
        End With
        Console.WriteLine(b.Label & " " & b.Size.Width & "x" & b.Size.Height)
        Dim st As Settings
        With st
            .Width = 10
        End With
        Console.WriteLine(st.Width)
        With "hello"
            Console.WriteLine(.ToUpper() & .Length)
        End With

        SyncLock b
            Console.WriteLine("locked")
        End SyncLock
    End Sub
End Module
