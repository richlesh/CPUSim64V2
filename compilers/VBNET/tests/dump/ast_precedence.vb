Module P
    Sub Main()
        a = 1 + 2 * 3 ^ 2 ^ 2
        a = -x ^ 2 * -y
        a = b & c + d << 2 = e
        a = Not b = c And d Or e Xor f
        a = x \ y * z Mod w
        a = p OrElse q AndAlso r
        a = s Like t Is u IsNot v
        a = TypeOf o Is String AndAlso TypeOf o IsNot Integer
        a = x?.y?(0)!z.ToString()
        a = CInt("1") + CType(o, Long) + DirectCast(o, String).Length
        a = Integer.MaxValue + String.Empty.Length + GetType(List(Of )).Name
        a = Foo(1, , z:=3) + NameOf(Main)
    End Sub
End Module
