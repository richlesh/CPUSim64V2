Class InsufficientFundsException
    Inherits Exception
    Public ReadOnly Amount As Double
    Public Sub New(amount As Double)
        MyBase.New("Insufficient funds: need " & amount.ToString("F2"))
        Me.Amount = amount
    End Sub
End Class

Class PlainError
    Inherits ApplicationException
End Class

Class Custom
    Inherits Exception
End Class

Module Program
    Dim log As String = ""

    Function Divide(a As Integer, b As Integer) As Integer
        Return a \ b
    End Function

    Sub Withdraw(balance As Double, amount As Double)
        If amount > balance Then Throw New InsufficientFundsException(amount - balance)
        Console.WriteLine("withdrawn " & amount)
    End Sub

    Function WithFinally(n As Integer) As String
        Try
            If n = 0 Then Return "zero"
            If n = 1 Then Throw New InvalidOperationException("one")
            Return "other"
        Finally
            log &= "[finally " & n & "]"
        End Try
    End Function

    Function Nested() As Integer
        For i = 1 To 5
            Try
                Try
                    If i = 2 Then Continue For
                    If i = 4 Then Exit For
                    log &= "<" & i & ">"
                Finally
                    log &= "f" & i
                End Try
            Finally
                log &= "F" & i
            End Try
        Next
        Return 7
    End Function

    Sub Main()
        Try
            Console.WriteLine(Divide(10, 0))
        Catch ex As DivideByZeroException
            Console.WriteLine("caught: " & ex.Message)
        End Try

        Dim arr(2) As Integer
        Try
            arr(5) = 1
        Catch ex As NullReferenceException
            Console.WriteLine("wrong")
        Catch ex As IndexOutOfRangeException When arr.Length > 10
            Console.WriteLine("wrong too")
        Catch ex As SystemException
            Console.WriteLine("system: " & ex.Message)
        End Try

        Try
            Withdraw(100, 30)
            Withdraw(100, 250.5)
            Console.WriteLine("not here")
        Catch ex As InsufficientFundsException
            Console.WriteLine(ex.Message & " / " & ex.Amount)
        Finally
            Console.WriteLine("finally 1")
        End Try

        Console.WriteLine(WithFinally(0) & " " & WithFinally(2))
        Try
            WithFinally(1)
        Catch ex As Exception
            Console.WriteLine("caught " & ex.Message)
        End Try
        Console.WriteLine(log)
        log = ""
        Console.WriteLine(Nested() & " " & log)

        Try
            Try
                Dim s As String = Nothing
                Console.WriteLine(s.Length)
            Catch ex As NullReferenceException
                Console.WriteLine("inner caught")
                Throw
            Finally
                Console.WriteLine("inner finally")
            End Try
        Catch ex As Exception
            Console.WriteLine("outer: " & ex.Message)
        End Try

        Try
            Try
                Throw New FormatException("bad format")
            Catch ex As FormatException
                Throw New InvalidOperationException("wrapped", ex)
            End Try
        Catch ex As InvalidOperationException
            Console.WriteLine(ex.Message & " <- " & ex.InnerException.Message)
            Console.WriteLine(ex.ToString())
        End Try

        Dim e1 As New PlainError()
        Console.WriteLine(e1.Message)
        Console.WriteLine(New Custom().Message)
        Console.WriteLine(New ArgumentNullException("name").Message)
        Console.WriteLine(New Exception("plain").ToString())

        Try
            Dim o As Object = "text"
            Dim n As Integer = CInt(o)
        Catch ex As InvalidCastException
            Console.WriteLine(ex.Message)
        End Try

        Try
            Dim big As Integer = Integer.MaxValue
            big += 1
        Catch ex As OverflowException
            Console.WriteLine(TypeOf ex Is ArithmeticException)
        End Try

        For i = 1 To 3
            Try
                If i = 2 Then GoTo skip
                Console.Write("i" & i & " ")
            Finally
                Console.Write("fin" & i & " ")
            End Try
skip:
        Next
        Console.WriteLine()
        Throw New Custom()
    End Sub
End Module
