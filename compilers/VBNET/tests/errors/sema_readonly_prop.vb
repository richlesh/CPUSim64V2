Class C
    ReadOnly Property P As Integer
        Get
            Return 1
        End Get
    End Property
End Class
Module M
    Sub Main()
        Dim c As New C()
        c.P = 2
    End Sub
End Module
