' Classes, properties, exceptions and a Dictionary: a small bank.
Class InsufficientFundsException
    Inherits Exception
    Public Sub New(needed As Double)
        MyBase.New($"Insufficient funds: {needed:F2} more needed.")
    End Sub
End Class

Class Account
    Private Shared nextNumber As Integer = 1001
    Public ReadOnly Property Number As Integer
    Public Property Owner As String
    Public ReadOnly Property Balance As Double

    Public Sub New(owner As String, Optional initial As Double = 0)
        Number = nextNumber
        nextNumber += 1
        Me.Owner = owner
        _Balance = initial
    End Sub

    Public Sub Deposit(amount As Double)
        If amount <= 0 Then Throw New ArgumentOutOfRangeException(NameOf(amount), "The amount must be positive.")
        _Balance += amount
    End Sub

    Public Sub Withdraw(amount As Double)
        If amount > _Balance Then Throw New InsufficientFundsException(amount - _Balance)
        _Balance -= amount
    End Sub

    Public Overrides Function ToString() As String
        Return $"#{Number} {Owner,-6} {Balance,10:N2}"
    End Function
End Class

Module Bank
    Sub Main()
        Dim accounts As New Dictionary(Of Integer, Account)
        For Each a In {New Account("Ann", 500), New Account("Bob"), New Account("Cy", 1250.75)}
            accounts.Add(a.Number, a)
        Next
        accounts(1002).Deposit(80)
        Try
            accounts(1002).Withdraw(100)
        Catch ex As InsufficientFundsException
            Console.WriteLine("Bob: " & ex.Message)
        End Try
        Try
            accounts(1001).Deposit(-5)
        Catch ex As ArgumentOutOfRangeException
            Console.WriteLine("Ann: " & ex.Message)
        End Try
        accounts(1003).Withdraw(250.75)
        For Each kv In accounts
            Console.WriteLine(kv.Value)
        Next
        Dim total = 0.0
        For Each a In accounts.Values
            total += a.Balance
        Next
        Console.WriteLine($"Total {total,17:N2}")
    End Sub
End Module
