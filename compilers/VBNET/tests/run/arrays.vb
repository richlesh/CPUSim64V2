Module Arrays
    Dim primes() As Integer = {2, 3, 5, 7}

    Function Total(xs() As Integer) As Integer
        Dim s = 0
        For i = 0 To xs.Length - 1
            s += xs(i)
        Next
        Return s
    End Function

    Sub Fill(a() As Integer, v As Integer)
        For i = 0 To a.Length - 1
            a(i) = v
        Next
    End Sub

    Function Main(args As String()) As Integer
        Dim sieve(30) As Boolean
        For i = 2 To 30
            If Not sieve(i) Then
                Console.Write(i & " ")
                For j = i * i To 30 Step i
                    sieve(j) = True
                Next
            End If
        Next
        Console.WriteLine()
        Console.WriteLine(Total(primes))
        Dim grid(2, 3) As Integer
        For r = 0 To 2
            For c = 0 To 3
                grid(r, c) = r * 10 + c
            Next
        Next
        Console.WriteLine(grid(2, 3) & " " & grid.Length)
        Dim m = {{1, 2, 3}, {4, 5, 6}}
        Dim sum = 0
        For Each x In m
            sum += x
        Next
        Console.WriteLine(sum & " " & m(1, 2))
        Dim names() As String = {"ann", "bob"}
        ReDim Preserve names(3)
        names(3) = "dan"
        For Each nm In names
            Console.Write("[" & nm & "]")
        Next
        Console.WriteLine()
        Dim buf(4) As Integer
        Fill(buf, 7)
        Console.WriteLine(Total(buf))
        ReDim buf(1)
        Console.WriteLine(buf.Length & " " & buf(1))
        Dim jag(2)() As Integer
        jag(0) = {1}
        jag(1) = {1, 2}
        jag(2) = New Integer() {1, 2, 3}
        Console.WriteLine(jag(2)(2) + jag(1).Length)
        Dim ds() As Double = {0.5, 1.5}
        ds(1) *= 3
        Console.WriteLine(ds(0) + ds(1))
        Dim e() As Integer = {}
        Console.WriteLine(e.Length)
        Erase buf
        Console.WriteLine(buf Is Nothing)
        Console.WriteLine(args.Length & " " & args(0) & " " & args(1))
        Dim line = Console.ReadLine()
        While line IsNot Nothing
            Console.WriteLine("read: " & line & " (" & line.Length & ")")
            line = Console.ReadLine()
        End While
        Console.WriteLine("abc"(1))
        Return 3
    End Function
End Module
