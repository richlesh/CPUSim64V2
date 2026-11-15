Class A
End Class
Class B
End Class
Class H(Of T As A)
End Class
Module M
    Sub Main()
        Dim h As H(Of B)
    End Sub
End Module
