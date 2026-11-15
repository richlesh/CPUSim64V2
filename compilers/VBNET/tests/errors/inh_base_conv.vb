Option Strict On
Class A
End Class
Class B
    Inherits A
End Class
Module M
    Sub Main()
        Dim a As A = New B()
        Dim b As B = a
    End Sub
End Module
