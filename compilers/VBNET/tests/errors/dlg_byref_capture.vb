Module M
    Sub F(ByRef x As Integer)
        Dim g As Func(Of Integer) = Function() x
    End Sub
    Sub Main()
    End Sub
End Module
