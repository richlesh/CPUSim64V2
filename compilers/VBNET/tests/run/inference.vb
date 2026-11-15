Imports System.Runtime.CompilerServices

' Task 16: extension methods, nullable values, ?. and ?(, anonymous types,
' and the inference and initializers done earlier.
Module StringExtensions
    <Extension()>
    Public Function Shout(s As String) As String
        Return s.ToUpper() & "!"
    End Function

    <Extension>
    Public Function Repeat(s As String, n As Integer) As String
        Return StrDup(n, "-"c) & s
    End Function

    <Extension()>
    Public Function Total(xs As IEnumerable(Of Integer)) As Integer
        Dim t = 0
        For Each x In xs
            t += x
        Next
        Return t
    End Function

    <Extension()>
    Public Function Second(Of T)(xs As List(Of T)) As T
        Return xs(1)
    End Function

    <Extension()>
    Public Sub Bump(ByRef n As Integer)
        n += 1
    End Sub
End Module

Class Node
    Public Value As Integer
    Public NextNode As Node
    Public Name As String
    Public Sub Show()
        Console.WriteLine("node " & Value)
    End Sub
End Class

Module Program
    Function Find(xs() As Integer, v As Integer) As Integer?
        For i = 0 To xs.Length - 1
            If xs(i) = v Then Return i
        Next
        Return Nothing
    End Function

    Sub Main()
        ' extension methods
        Console.WriteLine("hello".Shout() & " " & "x".Repeat(3))
        Dim l As New List(Of Integer) From {1, 2, 3}
        Console.WriteLine(l.Total() & " " & l.Second() & " " & {4, 5, 6}.Total())
        Dim k = 5
        k.Bump()
        Console.WriteLine(k & " " & New List(Of String) From {"a", "b"}.Second())

        ' nullable values
        Dim a As Integer? = 5
        Dim b As Integer? = Nothing
        Dim c? As Double
        Console.WriteLine(a.HasValue & " " & b.HasValue & " " & c.HasValue & " " & a.Value)
        Console.WriteLine((a Is Nothing) & " " & (b Is Nothing) & " " & (b IsNot Nothing))
        Console.WriteLine(If(b, -1) & " " & If(a, -1) & " " & b.GetValueOrDefault())
        Dim arr = {4, 8, 15}
        Dim p = Find(arr, 8), q = Find(arr, 99)
        Console.WriteLine(p.Value & " " & q.HasValue & " [" & q & "] [" & p & "]")
        Dim plain As Integer = a
        Dim boxed As Object = a, boxedNone As Object = b
        Console.WriteLine((plain + 1) & " " & boxed.ToString() & " " & (boxedNone Is Nothing))
        Try
            Dim z As Integer = b
        Catch ex As InvalidOperationException
            Console.WriteLine(ex.Message)
        End Try

        ' ?. and ?(
        Dim n1 As New Node With {.Value = 1, .Name = "a"}
        n1.NextNode = New Node With {.Value = 2}
        Dim none As Node = Nothing
        Console.WriteLine(n1?.Value & " " & (none?.Value Is Nothing) & " " & n1.NextNode?.Value)
        Console.WriteLine(If(n1.NextNode?.NextNode?.Value, -1))
        Console.WriteLine(n1?.Name?.Length & " [" & n1.NextNode?.Name?.ToUpper() & "]")
        Dim len As Integer? = none?.Name.Length
        Console.WriteLine(len.HasValue)
        n1?.Show()
        none?.Show()
        Dim nums() As Integer = Nothing
        Console.WriteLine(nums?(0) Is Nothing)
        nums = {7, 8}
        Console.WriteLine(nums?(1))
        Dim s As String = Nothing
        Console.WriteLine(If(s?.Length, 0))

        ' anonymous types
        Dim person = New With {.Name = "Ann", .Age = 31}
        Console.WriteLine(person.Name & " " & person.Age & " " & person.ToString())
        person.Age += 1
        Console.WriteLine(person.Age)
        Dim k1 = New With {Key .Id = 7, .Label = "x"}
        Dim k2 = New With {Key .Id = 7, .Label = "y"}
        Console.WriteLine(k1.Equals(k2) & " " & (k1.GetHashCode() = k2.GetHashCode()))
        Dim x = 3
        Dim mixed = New With {x, person.Name, .Sq = x * x}
        Console.WriteLine(mixed)
    End Sub
End Module
