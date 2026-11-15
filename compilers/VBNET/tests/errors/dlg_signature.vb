Delegate Sub D(s As String)
Module M
    Sub F(x As Integer)
    End Sub
    Sub Main()
        Dim d As D = AddressOf F
    End Sub
End Module
