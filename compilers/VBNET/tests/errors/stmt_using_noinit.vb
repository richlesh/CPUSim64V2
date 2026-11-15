Class C
    Implements IDisposable
    Sub Dispose() Implements IDisposable.Dispose
    End Sub
End Class
Module M
    Sub Main()
        Using c As C
        End Using
    End Sub
End Module
