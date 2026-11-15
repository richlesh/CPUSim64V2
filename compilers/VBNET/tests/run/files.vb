Imports System.IO

Module Program
    Sub Main()
        Dim path = "files_test.tmp"
        File.WriteAllText(path, "first line" & vbLf & "ünïcödé" & vbLf)
        File.AppendAllText(path, "third")
        Console.WriteLine(File.Exists(path) & " " & File.ReadAllText(path).Length)
        For Each line In File.ReadAllLines(path)
            Console.WriteLine("[" & line & "]")
        Next
        Using w As New StreamWriter(path)
            w.WriteLine("a,1")
            w.WriteLine("b,22")
            w.Write(3.5)
            w.WriteLine()
        End Using
        Dim total As Double = 0
        Using r As New StreamReader(path)
            While Not r.EndOfStream
                Dim parts = r.ReadLine().Split(","c)
                total += CDbl(parts(parts.Length - 1))
            End While
        End Using
        Console.WriteLine(total)
        File.WriteAllLines(path, New List(Of String) From {"x", "y"})
        Console.WriteLine(File.ReadAllText(path).Replace(vbLf, "|"))
        File.Delete(path)
        Console.WriteLine(File.Exists(path))
        Try
            Dim s = File.ReadAllText("no_such_file.txt")
        Catch ex As FileNotFoundException
            Console.WriteLine(ex.Message)
        End Try
        Dim rd As New StreamReader("files.vb")
        rd.Close()
        Try
            rd.ReadLine()
        Catch ex As ObjectDisposedException
            Console.WriteLine(ex.Message)
        End Try
    End Sub
End Module
