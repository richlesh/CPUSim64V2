Module Control
    Dim counter As Integer = 10
    Const Limit = 3

    Sub Swap(ByRef a As Integer, ByRef b As Integer)
        Dim t = a
        a = b
        b = t
    End Sub

    Sub Bump(ByRef x As Long)
        x += 100
    End Sub

    Function Sum(ParamArray xs() As Integer) As Integer
        Dim total = 0
        For Each x In xs
            total += x
        Next
        Return total
    End Function

    Function Greet(Optional name As String = "world", Optional times As Integer = 1) As String
        Dim s = ""
        For i = 1 To times
            s &= "hi " & name & ";"
        Next
        Return s
    End Function

    Function Fib(n As Integer) As Integer
        If n < 2 Then
            Fib = n
        Else
            Fib = Fib(n - 1) + Fib(n - 2)
        End If
    End Function

    Function NextId() As Integer
        Static id As Integer = 100
        id += 1
        Return id
    End Function

    Sub Main()
        Dim a = 1, b = 2
        Swap(a, b)
        Console.WriteLine(a & " " & b)
        Dim n As Integer = 5
        Bump(n)
        Console.WriteLine(n)
        Console.WriteLine(Sum() & " " & Sum(1) & " " & Sum(1, 2, 3))
        Console.WriteLine(Greet())
        Console.WriteLine(Greet("vb", 2))
        Console.WriteLine(Greet(times:=1, name:="named"))
        Console.WriteLine(Fib(15))
        Console.WriteLine(NextId() & " " & NextId() & " " & NextId())
        counter += Limit
        Console.WriteLine(counter)

        For i = 10 To 1 Step -3
            Console.Write(i & " ")
        Next
        Console.WriteLine()
        For d As Double = 0 To 1 Step 0.25
            Console.Write(d & " ")
        Next
        Console.WriteLine()
        Dim stp = 2
        For i = 0 To 6 Step stp
            If i = 4 Then Continue For
            Console.Write(i & " ")
        Next
        Console.WriteLine()
        For i = 1 To 3
            For j = 1 To 3
                If j = 2 Then Exit For
                Console.Write(i * 10 + j & " ")
            Next
        Next
        Console.WriteLine()

        Dim k = 0
        Do While k < 3
            k += 1
        Loop
        Do
            k += 10
        Loop Until k > 30
        Console.WriteLine(k)
        While k > 0
            k -= 7
            If k < 10 Then Exit While
        End While
        Console.WriteLine(k)
        Do
            k += 1
            If k = 12 Then Exit Do
        Loop
        Console.WriteLine(k)

        For Each v In {5, 15, 25, 105, -3}
            Select Case v
                Case Is < 0
                    Console.WriteLine(v & ": negative")
                Case 0 To 9
                    Console.WriteLine(v & ": digit")
                Case 10, 15, 20
                    Console.WriteLine(v & ": listed")
                Case Else
                    Console.WriteLine(v & ": other")
            End Select
        Next
        Select Case "b"
            Case "a" : Console.WriteLine("A")
            Case "b" : Console.WriteLine("B")
        End Select

        Dim t = 0
again:
        t += 1
        If t < 5 Then GoTo again
        Console.WriteLine("t=" & t)

        Dim word = "hey"
        For Each c In word
            Console.Write(c & ".")
        Next
        Console.WriteLine()
        If a > 1 Then Console.WriteLine("a>1") Else Console.WriteLine("a<=1")
        Console.WriteLine(If(b > a, "b", "a"))
    End Sub
End Module
