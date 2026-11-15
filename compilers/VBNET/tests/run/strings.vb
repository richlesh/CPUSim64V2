Module Strs
    Sub Main()
        Dim s As String = "  Hello, World  "
        Dim t = s.Trim()
        Console.WriteLine("[" & t & "] " & t.Length)
        Console.WriteLine(t.ToUpper() & " " & t.ToLower())
        Console.WriteLine(t.Substring(7) & "|" & t.Substring(0, 5))
        Console.WriteLine(t.IndexOf("o") & " " & t.LastIndexOf("o") & " " & t.IndexOf("o"c, 5) & " " & t.IndexOf("xyz"))
        Console.WriteLine(t.Contains("World") & " " & t.StartsWith("Hell") & " " & t.EndsWith("x"))
        Console.WriteLine(t.Replace("l", "L") & " " & t.Replace("o"c, "0"c))
        Console.WriteLine(t.Insert(5, "!!!") & " " & t.Remove(5) & " " & t.Remove(0, 7))
        Console.WriteLine("[" & "ab".PadLeft(5) & "][" & "ab".PadRight(5, "."c) & "]")
        Dim parts = "a,b,,c".Split(","c)
        Console.WriteLine(parts.Length & ": " & String.Join("|", parts))
        Dim words = "the quick  brown fox".Split(" "c)
        Console.WriteLine(words.Length & " " & words(3))
        Console.WriteLine(String.Join("-", {"x", "y", "z"}))
        Console.WriteLine(String.IsNullOrEmpty("") & " " & String.IsNullOrEmpty(Nothing) & " " & String.IsNullOrEmpty("a"))
        Console.WriteLine(String.Concat("ab", "cd") & " " & String.Format("{0}-{1}", 1, "two"))
        Console.WriteLine(String.Compare("apple", "Banana") & " " & String.Compare("b", "a") & " " & "abc".CompareTo("abc"))
        Dim chars = "hello".ToCharArray()
        Array_Reverse(chars)
        Console.WriteLine(CStr(chars))
        Console.WriteLine(Len("abcd") & " " & Left("abcdef", 2) & " " & Right("abcdef", 3) & " " & Mid("abcdef", 2, 3) & " " & Mid("abcdef", 4))
        Console.WriteLine(InStr("hello world", "o") & " " & InStr(6, "hello world", "o") & " " & InStr("abc", "z") & " " & InStrRev("hello world", "o"))
        Console.WriteLine(UCase("MiXeD") & LCase("MiXeD") & "[" & Trim("  x  ") & "][" & LTrim("  x  ") & "][" & RTrim("  x  ") & "]")
        Console.WriteLine(Chr(65) & ChrW(97) & " " & Asc("A") & " " & AscW("a"c))
        Console.WriteLine("[" & Space(3) & "]" & StrDup(4, "*"c) & StrReverse("abc"))
        Console.WriteLine(Replace("a-b-c", "-", "+") & " " & Split("1 2 3")(2) & " " & Join({"p", "q"}, ","))
        Console.WriteLine(Val("  12.5abc") & " " & Val("&HFF") & " " & IsNumeric("3.14") & " " & IsNumeric("pi"))
        Console.WriteLine(Hex(255) & " " & Hex(-1) & " " & Oct(8))
        Console.WriteLine(Int(-2.5) & " " & Fix(-2.5) & " " & Int(7))
        Console.WriteLine(Char.IsDigit("7"c) & " " & Char.IsLetter("x"c) & " " & Char.IsUpper("x"c) & " " & Char.ToUpper("q"c))
        Console.WriteLine(("ab" Like "a*") & " " & ("a1" Like "a#") & " " & ("x" Like "[a-f]") & " " & ("Cat" Like "C?t"))
        Dim n As Integer
        Console.WriteLine(Integer.TryParse("123", n) & " " & n)
        Console.WriteLine(Integer.TryParse("12x", n) & " " & n)
        Dim d As Double
        Console.WriteLine(Double.TryParse("1,234.5", d) & " " & d)
        Console.WriteLine(Integer.Parse(" -42 ") + 1)
        Console.WriteLine(CInt("  7 ") + CDbl("2.5") + Convert.ToInt32("10"))
        Console.WriteLine(Boolean.Parse("true") & " " & CBool("False"))
        Dim c As Char = "z"c
        Console.WriteLine(c & "!" & c.ToString())
        Console.WriteLine("tab:" & vbTab & "|" & ControlChars.Quote & "q" & ControlChars.Quote)
        Dim arr() As Char = {"h"c, "i"c}
        Dim fromChars As String = arr
        Console.WriteLine(fromChars)
        Dim back() As Char = "xy"
        Console.WriteLine(back.Length & back(1))
    End Sub

    Sub Array_Reverse(a() As Char)
        For i = 0 To a.Length \ 2 - 1
            Dim tmp = a(i)
            a(i) = a(a.Length - 1 - i)
            a(a.Length - 1 - i) = tmp
        Next
    End Sub
End Module
