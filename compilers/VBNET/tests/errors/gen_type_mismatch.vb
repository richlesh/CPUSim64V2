Class Box(Of T)
    Public V As T
End Class
Module M
    Sub Main()
        Dim b As New Box(Of Integer)
        b.V = "x"c
    End Sub
End Module
