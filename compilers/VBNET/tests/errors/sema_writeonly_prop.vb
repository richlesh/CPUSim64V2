Class C
    WriteOnly Property P As Integer
        Set(v As Integer)
        End Set
    End Property
End Class
Module M
    Sub Main()
        Dim c As New C()
        Dim x = c.P
    End Sub
End Module
