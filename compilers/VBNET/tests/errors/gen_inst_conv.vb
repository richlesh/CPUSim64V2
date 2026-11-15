Class Box(Of T)
    Public V As T
End Class
Module M
    Sub Main()
        Dim a As Box(Of Integer) = New Box(Of Long)()
    End Sub
End Module
