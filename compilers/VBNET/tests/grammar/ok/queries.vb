Module Queries
    Sub Main()
        Dim q1 = From x In xs
        Dim q2 = From x In xs Where x > 1 Select x
        Dim q3 = From x As Integer In xs, y In ys
                 Where x < y
                 Order By x Descending, y Ascending, x + y
                 Select Sum = x + y, x
        Dim q4 = From c In customers
                 Join o In orders On c.Id Equals o.CustomerId And c.Region Equals o.Region
                 Select c.Name, o.Total
        Dim q5 = From c In customers
                 Group Join o In orders On c.Id Equals o.CustomerId Into Orders = Group, Count()
        Dim q6 = From p In products
                 Group p By p.Category Into g = Group, Total = Sum(p.Price)
                 Order By Total
        Dim q7 = From p In products
                 Group By key = p.Category
                 Into Count()
        Dim q8 = Aggregate x In xs Into Sum(x)
        Dim q9 = Aggregate x In xs Where x > 0 Into Total = Sum(x), Max(x)
        Dim q10 = From x In xs Let y = x * 2, z = y + 1 Where z > 3 Distinct Take 5 Skip 1
        Dim q11 = From x In xs Take While x < 10 Skip While x < 2 Select x
        Dim q12 = From x In xs
                  From y In x.Children
                  Select x, y
        Dim q13 = From a In xs
                  Join b In ys
                  Join c In zs On b.Id Equals c.Id
                  On a.Id Equals b.Id
                  Select a
        Dim q14 = From x In xs Select x Distinct
        Dim from = 1, where = 2, take = 3
        from = where + take
    End Sub
End Module
