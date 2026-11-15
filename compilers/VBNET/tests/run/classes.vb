Class Account
    Private Shared nextId As Integer = 1000
    Public ReadOnly Id As Integer
    Public Property Owner As String
    Private _balance As Double
    Public Shared Count As Integer

    Public Sub New(owner As String)
        Me.Owner = owner
        Id = nextId
        nextId += 1
        Count += 1
    End Sub

    Public Sub New(owner As String, initial As Double)
        Me.New_(owner)
        Deposit(initial)
    End Sub

    Private Sub New_(owner As String)
        Me.Owner = owner
    End Sub

    Public ReadOnly Property Balance As Double
        Get
            Return _balance
        End Get
    End Property

    Public Property Limit As Double
        Get
            Limit = 500
            If _balance > 1000 Then Limit = 2000
        End Get
        Set(value As Double)
            Console.WriteLine("limit set to " & value)
        End Set
    End Property

    Public Sub Deposit(amount As Double)
        _balance += amount
    End Sub

    Public Function Withdraw(amount As Double) As Boolean
        If amount > _balance Then Return False
        _balance -= amount
        Return True
    End Function

    Public Function Describe() As String
        Return $"{Owner} #{Id}: {Balance:F2}"
    End Function
End Class

Class Point
    Public X As Integer
    Public Y As Integer
    Public Function Dist2() As Integer
        Return X * X + Y * Y
    End Function
End Class

Module Program
    Sub Main()
        Dim a As New Account("Ann")
        Dim b = New Account("Bob")
        a.Deposit(100)
        Console.WriteLine(a.Withdraw(30) & " " & a.Withdraw(500))
        Console.WriteLine(a.Describe())
        Console.WriteLine(b.Describe())
        Console.WriteLine(Account.Count)
        Dim c As New Account("Cy", 2500)
        Console.WriteLine(c.Describe() & " limit " & c.Limit)
        c.Limit = 10
        a.Owner = "Anne"
        Console.WriteLine(a.Owner & " " & a.Limit)
        Dim p As New Point With {.X = 3, .Y = 4}
        Console.WriteLine(p.Dist2())
        Dim q As Point = p
        q.X = 6
        Console.WriteLine(p.X & " " & (p Is q))
        Dim o As Object = p
        Console.WriteLine(o.ToString() & " " & p.ToString())
        Dim back As Point = CType(o, Point)
        Console.WriteLine(back.Y)
        Dim n As Point = Nothing
        Console.WriteLine(n Is Nothing)
        Console.WriteLine(n.X)
    End Sub
End Module
