Module Expressions
    Sub Main()
        Dim v = 1 + 2 * 3 - 4 / 5 \ 6 Mod 7 ^ 8
        v = -1 + +2 - -3
        v = &HFF + &O17 + &B101 + 1_000 + 2UL + 3S + 1.5E3 + .5 + 2.0R + 3!
        v = "a" & "b" & 1 & "c"c
        v = a << 2 >> 1
        v = a = b And c <> d Or e < f Xor g >= h AndAlso i <= j OrElse k > l
        v = Not a And Not b
        v = x Is Nothing OrElse x IsNot Nothing
        v = s Like "a*"
        v = TypeOf o Is String
        v = TypeOf o IsNot List(Of Integer)
        v = If(a, b)
        v = If(a > 0, "pos", "neg")
        v = CInt(x) + CLng(x) + CDbl(x) + CBool(x) + CStr(x) + CChar(x) + CObj(x) + CSng(x)
        v = CType(x, Integer) + DirectCast(o, String).Length + TryCast(o, Object)
        v = GetType(Integer) Is GetType(Dictionary(Of ,))
        v = NameOf(v) & NameOf(Console.WriteLine)
        v = Integer.MaxValue + Double.Parse("1") + String.Empty.Length + Object.ReferenceEquals(a, b)
        v = Me.ToString() & MyBase.ToString() & MyClass.GetHashCode()
        v = Global.System.Math.Max(1, 2)
        v = a?.b?.c?(0)
        v = x.y!z.w(1)(2)
        v = New Integer() {1, 2, 3}
        v = New Integer(2) {}
        v = New Integer(0 To 2) {1, 2, 3}
        v = New Integer(,) {{1, 2}, {3, 4}}
        v = New String(2)() {}
        v = New List(Of Integer) From {1, 2, 3}
        v = New Dictionary(Of String, Integer) From {{"a", 1}, {"b", 2}}
        v = New Point With {.X = 1, .Y = 2}
        v = New Point() With {.X = 1}
        v = New With {.Name = "a", Key .Id = 1, x, y.z}
        v = New Nullable(Of Integer)
        v = New Integer?
        v = {1, 2, 3}
        v = {}
        v = {{1, 2}, {3}}
        v = (a + b) * c
        v = Function(x) x * 2
        v = Function(x As Integer, y As Integer) x + y
        v = Function() 42
        v = Sub(x) Console.WriteLine(x)
        v = Sub() Return
        v = Function(x As Integer) As Integer
                Dim y = x + 1
                Return y
            End Function
        v = Sub(x)
                Console.WriteLine(x)
            End Sub
        Run(Sub()
                Console.WriteLine("callback")
            End Sub)
        v = AddressOf Foo
        v = AddressOf obj.Method
        v = $"plain"
        v = $"{a}"
        v = $"a{b}c{d + 1}e{{literal}}f"
        v = $"{x,10}|{y,-5:N2}|{z:yyyy-MM-dd}"
        v = $"{If(a, "yes", "no")}"
        v = $"{list.Count(Function(q) q > 1)}"
        v = "multi
line"
        v = 1 +
            2 *
            3
        v = Foo(1,
                2,
                3)
        v = obj.
            Method()
        v = {
              1,
              2
            }
        v = a AndAlso
            b
        v = 1 _
            + 2
        v = 2 _ ' comment after continuation
            * 3
    End Sub
End Module
