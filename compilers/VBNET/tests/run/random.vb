Module M
    Sub Main()
        For Each s In {2000000000, 161803399, 1000000000, 42, 0, -7}
            Dim r As New Random(s)
            Console.WriteLine(s & " " & r.Next() & " " & r.Next(100))
        Next
    End Sub
End Module
