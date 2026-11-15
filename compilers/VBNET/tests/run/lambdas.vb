Delegate Function BinOp(a As Integer, b As Integer) As Integer
Delegate Sub Notify(msg As String)

Class Counter
    Private count As Integer
    Public Function MakeIncrementer(stepBy As Integer) As Func(Of Integer)
        Return Function()
                   count += stepBy
                   Return count
               End Function
    End Function
    Public Sub Shout(msg As String)
        Console.WriteLine("Counter says " & msg & " at " & count)
    End Sub
End Class

Class Shape
    Public Overridable Function Name() As String
        Return "shape"
    End Function
End Class

Class Circle
    Inherits Shape
    Public Overrides Function Name() As String
        Return "circle"
    End Function
End Class

Module Program
    Function Add(a As Integer, b As Integer) As Integer
        Return a + b
    End Function

    Sub Log(msg As String)
        Console.WriteLine("log: " & msg)
    End Sub

    Function Apply(f As Func(Of Integer, Integer), x As Integer) As Integer
        Return f(x)
    End Function

    Function Compose(Of A, B, C)(f As Func(Of A, B), g As Func(Of B, C)) As Func(Of A, C)
        Return Function(x) g(f(x))
    End Function

    Function Map(Of T, R)(items As IEnumerable(Of T), f As Func(Of T, R)) As List(Of R)
        Dim result As New List(Of R)
        For Each x In items
            result.Add(f(x))
        Next
        Return result
    End Function

    Sub Main()
        Dim op As BinOp = AddressOf Add
        Console.WriteLine(op(2, 3) & " " & op.Invoke(10, 20))
        op = Function(a, b) a * b
        Console.WriteLine(op(6, 7))
        Dim sq = Function(x As Integer) x * x
        Console.WriteLine(sq(9) & " " & Apply(sq, 4) & " " & Apply(Function(x) x + 100, 1))
        Dim factor = 3
        Dim scale As Func(Of Integer, Integer) = Function(x) x * factor
        factor = 5
        Console.WriteLine(scale(2))
        Dim c As New Counter()
        Dim inc = c.MakeIncrementer(10)
        inc()
        inc()
        Console.WriteLine(inc())
        Dim n As Notify = AddressOf Log
        n = CType([Delegate].Combine(n, New Notify(AddressOf c.Shout)), Notify)
        n("hi")
        Dim fs As New List(Of Func(Of Integer))
        For i = 1 To 3
            Dim j = i
            fs.Add(Function() j * 10)
        Next
        For Each f In fs
            Console.Write(f() & " ")
        Next
        Console.WriteLine()
        Dim greet As Action(Of String) = Sub(s) Console.WriteLine("Hello, " & s)
        greet("VB")
        Dim twice = Compose(Function(x As Integer) x + 1, Function(y As Integer) y * 2)
        Console.WriteLine(twice(5))
        Dim toText = Compose(Function(x As Integer) x * 3, Function(y As Integer) "<" & y & ">")
        Console.WriteLine(toText(7))
        Dim adder = Function(x As Integer) Function(y As Integer) x + y
        Console.WriteLine(adder(3)(4))
        Dim total = 0
        Dim acc As Action(Of Integer) = Sub(v)
                                            total += v
                                        End Sub
        acc(5)
        acc(6)
        Console.WriteLine(total)

        Dim nums As New List(Of Integer) From {5, 12, 7, 20, 3, 8}
        Console.WriteLine(nums.Find(Function(x) x > 10) & " " & nums.FindIndex(Function(x) x > 10) & " " & nums.Exists(Function(x) x < 0))
        Console.WriteLine(String.Join(",", nums.FindAll(Function(x) x Mod 2 = 0)))
        nums.Sort(Function(a, b) b.CompareTo(a))
        Console.WriteLine(String.Join(",", nums))
        Console.WriteLine(nums.RemoveAll(Function(x) x < 6) & " " & String.Join(",", nums))
        nums.ForEach(Sub(x) Console.Write("[" & x & "]"))
        Console.WriteLine()
        Dim words = Map(nums, Function(x) StrDup(x \ 4, "*"c))
        Console.WriteLine(String.Join(" ", words))
        Console.WriteLine(String.Join(",", nums.ConvertAll(Function(x) x / 4)))
        Dim names() As String = {"pear", "fig", "banana", "kiwi"}
        Array.Sort(names, Function(a, b) a.Length - b.Length)
        Console.WriteLine(String.Join(",", names) & " " & Array.Find(names, Function(s) s.StartsWith("b")))
        Dim shapes As New List(Of Shape) From {New Shape(), New Circle()}
        Dim namers = shapes.ConvertAll(Function(s) New Func(Of String)(AddressOf s.Name))
        Console.WriteLine(namers(0)() & " " & namers(1)())
        Dim ops As New Dictionary(Of String, Func(Of Double, Double, Double)) From {
            {"+", Function(a, b) a + b}, {"-", Function(a, b) a - b}, {"^", AddressOf Math.Pow}}
        For Each kv In ops
            Console.Write(kv.Key & "=" & kv.Value(2, 5) & " ")
        Next
        Console.WriteLine()
    End Sub
End Module
