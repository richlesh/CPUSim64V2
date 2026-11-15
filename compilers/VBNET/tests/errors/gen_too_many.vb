Class Box(Of T)
    Public V As T
End Class
Module M
    Sub Main()
        Dim b As Box(Of Integer, String)
    End Sub
End Module
