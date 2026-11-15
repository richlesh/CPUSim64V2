Imports System.Text

Class Person
    Implements IComparable(Of Person)
    Public Property Name As String
    Public Property Age As Integer
    Public Sub New(name As String, age As Integer)
        Me.Name = name
        Me.Age = age
    End Sub
    Public Function CompareTo(other As Person) As Integer Implements IComparable(Of Person).CompareTo
        Return Age.CompareTo(other.Age)
    End Function
    Public Overrides Function ToString() As String
        Return Name & "(" & Age & ")"
    End Function
End Class

Module Program
    Function Show(Of T)(items As IEnumerable(Of T)) As String
        Dim sb As New StringBuilder()
        For Each x In items
            If sb.Length > 0 Then sb.Append(", ")
            sb.Append(x)
        Next
        Return "[" & sb.ToString() & "]"
    End Function

    Sub Main()
        Dim nums As New List(Of Integer) From {5, 3, 8, 1}
        nums.Add(9)
        nums.Insert(0, 7)
        Console.WriteLine(Show(nums) & " count=" & nums.Count)
        nums.Sort()
        Console.WriteLine(Show(nums) & " " & nums(0) & " " & nums.IndexOf(8) & " " & nums.Contains(4))
        nums.Remove(8)
        nums.RemoveAt(0)
        nums(0) = 42
        nums.Reverse()
        Console.WriteLine(Show(nums) & " " & nums.BinarySearch(100))
        Dim arr = nums.ToArray()
        Console.WriteLine(arr.Length & " " & arr(arr.Length - 1))

        Dim people As New List(Of Person) From {New Person("Ann", 31), New Person("Bob", 25), New Person("Cy", 40)}
        people.Sort()
        Console.WriteLine(Show(people))

        Dim ages As New Dictionary(Of String, Integer)
        ages("ann") = 31
        ages.Add("bob", 25)
        ages("cy") = 40
        ages("ann") += 1
        Console.WriteLine(ages.Count & " " & ages("ann") & " " & ages.ContainsKey("bob") & " " & ages.ContainsKey("zed"))
        For Each kv In ages
            Console.Write(kv.Key & "=" & kv.Value & " ")
        Next
        Console.WriteLine()
        ages.Remove("bob")
        ages("dan") = 19
        Console.WriteLine(Show(ages.Keys) & " " & Show(ages.Values))
        Dim v As Integer
        Console.WriteLine(ages.TryGetValue("cy", v) & " " & v & " " & ages.TryGetValue("x", v) & " " & v)
        Try
            Dim x = ages("nobody")
        Catch ex As KeyNotFoundException
            Console.WriteLine(ex.Message)
        End Try
        Try
            ages.Add("cy", 1)
        Catch ex As ArgumentException
            Console.WriteLine(ex.Message)
        End Try
        Dim squares As New Dictionary(Of Integer, String) From {{1, "one"}, {2, "four"}}
        Console.WriteLine(squares(2) & " " & Show(squares))

        Dim words As New HashSet(Of String)
        For Each w In "the cat and the hat and the bat".Split(" "c)
            words.Add(w)
        Next
        Console.WriteLine(words.Count & " " & Show(words) & " " & words.Contains("hat") & " " & words.Add("cat"))

        Dim st As New Stack(Of String)
        st.Push("a")
        st.Push("b")
        st.Push("c")
        Console.WriteLine(Show(st) & " " & st.Pop() & " " & st.Peek() & " " & st.Count)
        Dim q As New Queue(Of Integer)
        For i = 1 To 6
            q.Enqueue(i * 10)
        Next
        Console.WriteLine(q.Dequeue() & " " & q.Dequeue() & " " & q.Peek() & " " & Show(q))

        Dim sb As New StringBuilder("x")
        sb.Append(1).Append("-"c).Append(2.5).Append(True).AppendLine()
        sb.Insert(0, ">").Replace("True", "T")
        sb.Remove(1, 1)
        Console.Write(sb.ToString())
        Console.WriteLine(sb.Length & " " & sb(0))

        Dim letters() As String = {"pear", "Apple", "fig", "apple"}
        Array.Sort(letters)
        Console.WriteLine(String.Join(",", letters) & " " & Array.IndexOf(letters, "fig"))
        Dim small() As Integer = {3, 1, 2}
        Array.Resize(small, 5)
        Array.Reverse(small)
        Console.WriteLine(String.Join(",", small))
        Dim t = Tuple.Create("pi", 3.14)
        Console.WriteLine(t.Item1 & " " & t.Item2 & " " & t.ToString())

        Dim r As New Random(42)
        Console.WriteLine(r.Next() & " " & r.Next(100) & " " & r.Next(5, 10) & " " & r.NextDouble().ToString("F6"))

        Try
            For Each n In nums
                If n = 42 Then nums.Add(0)
            Next
        Catch ex As InvalidOperationException
            Console.WriteLine(ex.Message)
        End Try
        Try
            Console.WriteLine(nums(99))
        Catch ex As ArgumentOutOfRangeException
            Console.WriteLine(ex.Message)
        End Try
    End Sub
End Module
