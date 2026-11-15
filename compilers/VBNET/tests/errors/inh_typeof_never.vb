Class A
End Class
NotInheritable Class B
End Class
Module M
    Sub Main()
        Dim a As New A()
        Dim b = TypeOf a Is B
    End Sub
End Module
