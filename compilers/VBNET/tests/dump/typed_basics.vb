Module Program
    Const Greeting As String = "Hello, " & "World"
    Const Big = Limit * 2
    Const Limit = 100
    Dim counter As Integer = 5
    Private names() As String = {"a", "b"}

    Function Square(x As Integer) As Integer
        Return x * x
    End Function

    Function Sum(ParamArray xs() As Integer) As Long
        Dim total As Long
        For Each x In xs
            total += x
        Next
        Sum = total
    End Function

    Sub Show(Optional label As String = "n", Optional ByRef n As Long = 0)
        Console.WriteLine(label & "=" & n)
    End Sub

    Sub Main()
        Dim a = 1 + 2 * 3
        Dim b As Byte = 200
        Dim c = b + b
        Dim d = 7 / 2
        Dim e As Double = a
        Dim s = Greeting & "!"
        Dim arr(9) As Integer
        Dim grid = {{1, 2}, {3, 4}}
        For i = 0 To arr.Length - 1
            arr(i) = Square(i)
        Next
        If a > 3 AndAlso s.Length > 0 Then Console.WriteLine(s)
        Select Case a
            Case 1, 2 To 5
                Console.WriteLine("small")
            Case Is > 100
            Case Else
                Console.WriteLine(Big)
        End Select
        Show()
        Show("total", Sum(1, 2, 3))
        Console.WriteLine(Integer.MaxValue)
        Console.WriteLine("{0} {1}", a, c)
        Console.WriteLine(CInt(2.5) + CInt(3.5))
        Console.WriteLine(&HFFFFFFFF)
        Console.WriteLine(-2 ^ 2)
        Console.WriteLine($"a={a}")
    End Sub
End Module
