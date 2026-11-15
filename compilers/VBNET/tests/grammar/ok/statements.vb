Module Statements
    Sub Main()
        Dim a As Integer = 1, b, c As Long
        Dim arr(9) As Integer, grid(2, 3) As Double, jag()() As Integer
        Dim n? As Integer = Nothing
        Dim list As New List(Of Integer)
        Dim s As String = "x" : Dim t = s
        Static calls As Integer
        Const Max = 10

        If a > 0 Then
            a = 1
        ElseIf a < 0 Then
            a = -1
        Else If a = 0
            a = 0
        Else
            a = 2
        End If
        If a = 1 Then a = 2 Else a = 3
        If a = 1 Then a = 2 : b = 3 Else a = 4
        If a = 1 Then Return
        If a Then
        End If

        Select Case a
            Case 1
            Case 2, 3, 5 To 7, Is > 100, Is <= -1, < 0
                b = 2
            Case Else
                b = 3
        End Select
        Select a
        End Select

        While a < 10
            a += 1
            If a = 5 Then Exit While
            Continue While
        End While

        Do
            a -= 1
        Loop While a > 0
        Do Until a >= 10
            a += 2
        Loop
        Do While True
            Exit Do
        Loop
        Do : a = 0 : Loop

        For i = 0 To 10 Step 2
            Continue For
        Next i
        For i As Integer = 10 To 0 Step -1 : Next
        For Each x As Integer In arr
            Exit For
        Next
        For Each ch In s : Next ch
        For i = 1 To 3
            For j = 1 To 3
                For k = 1 To 3
                Next k, j, i

        Try
            Throw New Exception("x")
        Catch ex As ArgumentException When ex.Message = "y"
            Throw
        Catch ex As Exception
        Catch
        Finally
            a = 0
        End Try
        Try
        Finally
        End Try

        With list
            .Add(1)
            .Capacity = 10
        End With
        SyncLock list
        End SyncLock
        Using r As New IO.StringReader("a"), w = New IO.StringWriter()
        End Using
        Using list
        End Using

        ReDim arr(19)
        ReDim Preserve arr(0 To 29), grid(4, 5)
        Erase arr, grid

        AddHandler obj.Changed, AddressOf OnChanged
        RemoveHandler obj.Changed, AddressOf Me.OnChanged
        RaiseEvent Changed(Me, EventArgs.Empty)
        RaiseEvent Done

        Call Foo()
        Call Foo
        Foo()
        Foo
        obj.Method(1, , 3)
        obj!key = 4
        x = obj("a")(0).Field
        dict("a") = 1
        a += 1 : a -= 1 : a *= 2 : a /= 2 : a \= 2 : a ^= 2 : s &= "x" : a <<= 1 : a >>= 1
        Mid(s, 1, 1) = "y"
        GoTo 10
10:     a = 1
done:
        GoTo done
        Stop
        End
    End Sub

    Function F() As Integer
        Exit Function
    End Function

    Property P As Integer
        Get
            Exit Property
        End Get
        Set
        End Set
    End Property
End Module
