' A file with only modules and comments, regions and odd spacing.
REM a REM comment
#Region "Main"
Module   Misc   ' trailing comment


    Sub    Main  (  )
        Dim [Integer] As Integer = 1 : Dim [End] = [Integer] + 1
        Dim x% = 1, s$ = "", d# = 2.0, l& = 3, f! = 1.5
        Dim café = 1, ÉCOLE = café
        Dim r = dict!key
        x = l& + 1
        DIM Y AS INTEGER = 2 : y = Y
        dim z as string = "lower case keywords"
    End Sub

    Sub Empty() : End Sub

    Sub WithParams(ByVal a As Integer, ByRef b As String,
                   Optional c As Boolean = False)
    End Sub
End Module
#End Region
