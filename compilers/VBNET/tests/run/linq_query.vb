Class Person
    Public Property Name As String
    Public Property Age As Integer
    Public Property City As String
    Public Sub New(n As String, a As Integer, c As String)
        Name = n
        Age = a
        City = c
    End Sub
End Class

Class Order
    Public Property Customer As String
    Public Property Total As Double
    Public Sub New(c As String, t As Double)
        Customer = c
        Total = t
    End Sub
End Class

Module Program
    Sub Main()
        Dim numbers = {5, 3, 8, 1, 9, 2, 6}
        Dim result = From n In numbers Where n Mod 2 = 0 Order By n Descending Select n * 2
        Console.WriteLine(String.Join(",", result))
        Dim people As New List(Of Person) From {New Person("Ann", 31, "Oslo"), New Person("Bob", 25, "Rome"),
                                                New Person("Cy", 40, "Oslo"), New Person("Di", 25, "Rome")}
        Dim young = From p In people Where p.Age < 35 Order By p.Age, p.Name Select p.Name
        Console.WriteLine(String.Join(",", young))
        For Each x In From p In people Let decade = p.Age \ 10 Select p.Name, decade
            Console.Write(x.ToString() & " ")
        Next
        Console.WriteLine()
        For Each g In From p In people Group p By p.City Into Count(), Total = Sum(p.Age), Oldest = Max(p.Age)
            Console.WriteLine(g.City & " " & g.Count & " " & g.Total & " " & g.Oldest)
        Next
        For Each g In From p In people Group p.Name By p.Age Into Names = Group Order By Age
            Console.WriteLine(g.Age & ": " & String.Join("/", g.Names))
        Next
        Dim orders As New List(Of Order) From {New Order("Ann", 10.5), New Order("Cy", 7), New Order("Ann", 2)}
        For Each j In From p In people Join o In orders On p.Name Equals o.Customer Select p.Name, o.Total
            Console.Write(j.Name & "=" & j.Total & " ")
        Next
        Console.WriteLine()
        For Each j In From p In people Group Join o In orders On p.Name Equals o.Customer Into Bought = Sum(o.Total), Count()
            Console.Write(j.p.Name & ":" & j.Bought & "/" & j.Count & " ")
        Next
        Console.WriteLine()
        Dim pairs = From a In {1, 2, 3}, b In {10, 20} Where a + b > 12 Select a * b
        Console.WriteLine(String.Join(",", pairs))
        Dim total = Aggregate n In numbers Where n > 4 Into Sum(n)
        Dim stats = Aggregate n In numbers Into Low = Min(n), High = Max(n), Avg = Average(n)
        Console.WriteLine(total & " " & stats.Low & " " & stats.High & " " & stats.Avg.ToString("F3"))
        Dim firstTwo = From n In numbers Order By n Take 2
        Dim distinctAges = From p In people Select p.Age Distinct
        Console.WriteLine(String.Join(",", firstTwo) & " " & String.Join(",", distinctAges))
        Console.WriteLine((From n In numbers Where n > 100).Any())
        Dim words = {"apple", "Banana", "cherry", "avocado"}
        Dim byLetter = From w In words Group By First = Char.ToLower(w(0)) Into Group, Count()
        For Each g In byLetter
            Console.Write(g.First & "=" & g.Count & " ")
        Next
        Console.WriteLine()
    End Sub
End Module
