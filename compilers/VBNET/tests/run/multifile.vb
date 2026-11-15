Imports V = MathLib.Vectors

Module Program
    Sub Main()
        Dim a As New V.Vec(1, 2)
        Dim b As New V.Vec(3, 5)
        Dim c = a + b
        Console.WriteLine(c)
        Console.WriteLine(-a)
        Console.WriteLine(a * 3)
        Console.WriteLine((a = New V.Vec(1, 2)) & " " & (a <> b))
        Dim len As Double = CType(b, Double)
        Console.WriteLine(len.ToString("F3"))
        Dim d As Double = a
        Console.WriteLine(d.ToString("F3"))
        Dim w = CType(7, V.Vec)
        Console.WriteLine(w)
        c += a
        Console.WriteLine(c.ToString() & " " & c.Describe())
        Dim log As New MathLib.Logger
        log.Add("one")
        log.Add("two", 3)
        log.Add(1.5)
        Console.WriteLine(log.Count)
    End Sub
End Module
