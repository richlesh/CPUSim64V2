Structure S
    Public X As Integer
End Structure
Module M
    Function F() As S
        Return Nothing
    End Function
    Sub Main()
        F().X = 1
    End Sub
End Module
