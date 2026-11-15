' Lambdas and LINQ: queries over a list of products.
Class Product
    Public Property Name As String
    Public Property Category As String
    Public Property Price As Double
    Public Property Stock As Integer
End Class

Module LinqDemo
    Sub Main()
        Dim products As New List(Of Product) From {
            New Product With {.Name = "Apple", .Category = "Fruit", .Price = 0.5, .Stock = 120},
            New Product With {.Name = "Banana", .Category = "Fruit", .Price = 0.25, .Stock = 0},
            New Product With {.Name = "Carrot", .Category = "Vegetable", .Price = 0.3, .Stock = 75},
            New Product With {.Name = "Bread", .Category = "Bakery", .Price = 2.4, .Stock = 12},
            New Product With {.Name = "Cherry", .Category = "Fruit", .Price = 4.0, .Stock = 8}}

        Dim numbers = Enumerable.Range(1, 10)
        Dim result = From n In numbers Where n Mod 2 = 0 Order By n Descending Select n * 2
        Console.WriteLine("even numbers doubled: " & String.Join(", ", result))

        Dim inStock = From p In products Where p.Stock > 0 Order By p.Price Select p.Name, p.Price
        For Each p In inStock
            Console.WriteLine($"  {p.Name,-8}{p.Price,6:F2}")
        Next

        Dim byCategory = From p In products
                         Group p By p.Category Into Count(), Value = Sum(p.Price * p.Stock)
                         Order By Value Descending
        For Each g In byCategory
            Console.WriteLine($"{g.Category,-10}{g.Count,3} products, stock worth {g.Value,8:F2}")
        Next

        Dim cheapest = products.OrderBy(Function(p) p.Price).First()
        Dim expensive = products.Where(Function(p) p.Price > 1).Select(Function(p) p.Name.ToUpper())
        Console.WriteLine("cheapest: " & cheapest.Name & ", expensive: " & String.Join("/", expensive))
        Dim stats = Aggregate p In products Into Total = Sum(p.Stock), Avg = Average(p.Price)
        Console.WriteLine($"items in stock: {stats.Total}, average price {stats.Avg:F3}")
    End Sub
End Module
