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

Module Program
    Sub Main()
        Dim nums = {5, 3, 8, 1, 9, 2}
        Dim evens = nums.Where(Function(n) n Mod 2 = 0)
        Console.WriteLine(String.Join(",", evens))
        Console.WriteLine(String.Join(",", nums.Select(Function(n) n * n)))
        Console.WriteLine(String.Join(",", nums.OrderBy(Function(n) n)))
        Console.WriteLine(String.Join(",", nums.OrderByDescending(Function(n) n).Take(3)))
        Console.WriteLine(nums.Sum() & " " & nums.Max() & " " & nums.Min() & " " & nums.Average().ToString("F2") & " " & nums.Count())
        Console.WriteLine(nums.First() & " " & nums.Last() & " " & nums.First(Function(n) n > 5) & " " & nums.Any(Function(n) n > 8) & " " & nums.All(Function(n) n > 0))
        Dim people As New List(Of Person) From {New Person("Ann", 31, "Oslo"), New Person("Bob", 25, "Rome"),
                                                New Person("Cy", 40, "Oslo"), New Person("Di", 25, "Rome")}
        Console.WriteLine(people.Sum(Function(p) p.Age) & " " & people.Average(Function(p) p.Age) & " " & people.Max(Function(p) p.Age))
        Console.WriteLine(String.Join(",", people.OrderBy(Function(p) p.Age).ThenByDescending(Function(p) p.Name).Select(Function(p) p.Name)))
        For Each g In people.GroupBy(Function(p) p.City)
            Console.WriteLine(g.Key & ": " & g.Count() & " " & String.Join("/", g.Select(Function(p) p.Name)))
        Next
        Dim d = people.ToDictionary(Function(p) p.Name, Function(p) p.Age)
        Console.WriteLine(d("Cy"))
        Console.WriteLine(String.Join(",", Enumerable.Range(1, 5).Select(Function(i) i * 10).Reverse()))
        Console.WriteLine(String.Join(",", {1, 2, 2, 3, 1}.Distinct()) & " " & {1, 2, 3}.Aggregate(Function(a, b) a * b) & " " & nums.Skip(4).Count())
        Dim log As New List(Of String)
        Dim lazy = nums.Where(Function(n)
                                  log.Add("w" & n)
                                  Return n > 4
                              End Function).Select(Function(n) n * 100)
        Console.WriteLine("before: " & log.Count)
        Console.WriteLine(lazy.First() & " " & String.Join(",", log))
    End Sub
End Module
