Imports System.Collections.Generic

Class Stack(Of T)
    Private items(3) As T
    Private n As Integer
    Public Sub Push(x As T)
        If n = items.Length Then ReDim Preserve items(2 * n - 1)
        items(n) = x
        n += 1
    End Sub
    Public Function Pop() As T
        If n = 0 Then Throw New InvalidOperationException("Stack empty")
        n -= 1
        Return items(n)
    End Function
    Public ReadOnly Property Count As Integer
        Get
            Return n
        End Get
    End Property
    Public Function Peek() As T
        Return items(n - 1)
    End Function
End Class

Class Pair(Of A, B)
    Public First As A
    Public Second As B
    Public Sub New(f As A, s As B)
        First = f
        Second = s
    End Sub
    Public Function Swap() As Pair(Of B, A)
        Return New Pair(Of B, A)(Second, First)
    End Function
    Public Overrides Function ToString() As String
        Return "(" & First.ToString() & ", " & Second.ToString() & ")"
    End Function
End Class

Class Node(Of T)
    Public Value As T
    Public NextNode As Node(Of T)
End Class

' a generic collection that For Each can use through IEnumerable(Of T)
Class Range
    Implements IEnumerable(Of Integer)
    Private ReadOnly lo As Integer, hi As Integer
    Public Sub New(lo As Integer, hi As Integer)
        Me.lo = lo
        Me.hi = hi
    End Sub
    Public Function GetEnumerator() As IEnumerator(Of Integer) Implements IEnumerable(Of Integer).GetEnumerator
        Return New RangeEnumerator(lo, hi)
    End Function
    Private Function GetEnumerator1() As System.Collections.IEnumerator Implements System.Collections.IEnumerable.GetEnumerator
        Return GetEnumerator()
    End Function
End Class

Class RangeEnumerator
    Implements IEnumerator(Of Integer)
    Private cur As Integer, hi As Integer
    Public Sub New(lo As Integer, hi As Integer)
        cur = lo - 1
        Me.hi = hi
    End Sub
    Public ReadOnly Property Current As Integer Implements IEnumerator(Of Integer).Current
        Get
            Return cur
        End Get
    End Property
    Private ReadOnly Property Current1 As Object Implements System.Collections.IEnumerator.Current
        Get
            Return cur
        End Get
    End Property
    Public Function MoveNext() As Boolean Implements System.Collections.IEnumerator.MoveNext
        cur += 1
        Return cur <= hi
    End Function
    Public Sub Reset() Implements System.Collections.IEnumerator.Reset
    End Sub
    Public Sub Dispose() Implements IDisposable.Dispose
    End Sub
End Class

Class Version
    Implements IComparable(Of Version)
    Public Major As Integer, Minor As Integer
    Public Sub New(major As Integer, minor As Integer)
        Me.Major = major
        Me.Minor = minor
    End Sub
    Public Function CompareTo(other As Version) As Integer Implements IComparable(Of Version).CompareTo
        If Major <> other.Major Then Return Major.CompareTo(other.Major)
        Return Minor.CompareTo(other.Minor)
    End Function
    Public Overrides Function ToString() As String
        Return Major & "." & Minor
    End Function
End Class

Structure Box(Of T)
    Public Value As T
    Public Function Map(Of R)(f As R) As Box(Of R)
        Dim b As Box(Of R)
        b.Value = f
        Return b
    End Function
End Structure

Module Program
    Function Max(Of T As IComparable(Of T))(a As T, b As T) As T
        If a.CompareTo(b) >= 0 Then Return a
        Return b
    End Function

    Function Largest(Of T As IComparable(Of T))(xs() As T) As T
        Dim best = xs(0)
        For Each x In xs
            best = Max(best, x)
        Next
        Return best
    End Function

    Function Twice(Of T)(x As T) As T()
        Return New T() {x, x}
    End Function

    Function Count(Of T)(xs() As T) As Integer
        Return xs.Length
    End Function

    Function Make(Of T As New)() As T
        Return New T()
    End Function

    Function Sum(xs As IEnumerable(Of Integer)) As Integer
        Dim s = 0
        For Each x In xs
            s += x
        Next
        Return s
    End Function

    Sub Main()
        Dim s As New Stack(Of Integer)
        For i = 1 To 10
            s.Push(i * i)
        Next
        Console.WriteLine(s.Count & " " & s.Pop() & " " & s.Peek())
        Dim names As New Stack(Of String)()
        names.Push("a")
        names.Push("b")
        Console.WriteLine(names.Pop() & names.Pop())
        Dim p As New Pair(Of String, Integer)("x", 1)
        Console.WriteLine(p.ToString() & " " & p.Swap().ToString())
        Dim head As Node(Of Double) = Nothing
        For k = 1 To 3
            head = New Node(Of Double) With {.Value = k / 2, .NextNode = head}
        Next
        Console.WriteLine(head.Value + head.NextNode.Value)
        Console.WriteLine(Max(3, 7) & " " & Max("pear", "apple") & " " & Max(2.5, 1))
        Console.WriteLine(Max(Of Long)(5, 9))
        Console.WriteLine(Twice("hi").Length & " " & Count({1, 2, 3}) & " " & Count(Twice(1.5)))
        Console.WriteLine(Largest({New Version(1, 2), New Version(2, 0), New Version(1, 9)}))
        Console.WriteLine(Largest({"b", "c", "a"}))
        For Each i In New Range(1, 4)
            Console.Write(i & " ")
        Next
        Console.WriteLine()
        Console.WriteLine(Sum(New Range(1, 100)))
        Dim bx As Box(Of Integer)
        bx.Value = 5
        Dim bs = bx.Map("five")
        Console.WriteLine(bs.Value & " " & bx.Value)
        Console.WriteLine(Make(Of Stack(Of Integer))() Is Nothing)
        Console.WriteLine(Make(Of Integer)())
        Try
            Dim e As New Stack(Of Boolean)
            e.Pop()
        Catch ex As InvalidOperationException
            Console.WriteLine(ex.Message)
        End Try
        Console.WriteLine(New Stack(Of Integer)())
        Console.WriteLine(CObj(New Node(Of String())()))
    End Sub
End Module
