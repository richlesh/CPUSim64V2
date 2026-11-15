Module M
    Sub Main()
        Dim x As Integer = Foo(1,
                               2
                              )
        x = x +
            3 ' comment after an operator
        Console.WriteLine(x) _

        Dim s = "a" &

                "b"
        x += 1 : x -= 1
        Dim y = x _ ' VB 16 comment after _
            * 2
        Dim z = x.
            ToString()
    End Sub
End Module
