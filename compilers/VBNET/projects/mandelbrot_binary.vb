' mandelbrot_binary.vb
'
' Multi-process Mandelbrot set image generator.  It renders the set as a
' grayscale PGM image in the binary (P5) format, one byte for each pixel.
' The image is cut into bands of rows, one per processor core (at most
' MaxChildren); each band is computed by a child process made with
' CPUSim64.Process.Fork() that writes its rows to a temporary file
' filename_<n>.tmp.  The parent waits for the
' children and merges the files into filename.pgm.
' VB.NET version of documentation/projects/mandelbrot_binary.asm.
'
' Usage:
'     mandelbrot_binary x y radius escape_limit image_size filename
'
'     x, y         - the center of the view (floats)
'     radius       - half the width and height of the view (float)
'     escape_limit - the iterations after which a point counts as in the set
'     image_size   - the width and height of the image in pixels
'     filename     - the image is written to filename.pgm
'
' Example (the image of the documentation):
'     > mandelbrot_binary -0.748127681290498 0.09408613781852182 1e-6 10000 128 mandelbrot_binary
'
' Author:   Richard Lesh
Imports System.IO
Imports System.Text

Module mandelbrot_binary
    Const MaxChildren As Integer = 100

    Dim xMin As Double = -2.5, xMax As Double = 0.5, yMin As Double = -1.5, yMax As Double = 1.5
    Dim maxIteration As Integer = 50

    ' the escape time of the point (x0, y0): iterations / maxIteration, 0 if it never escapes
    Function ComputeEscape(x0 As Double, y0 As Double) As Double
        Dim x As Double = 0, y As Double = 0, x2 As Double = 0, y2 As Double = 0
        Dim iteration As Integer = 0
        Do
            y = y * x * 2 + y0
            x = x2 - y2 + x0
            x2 = x * x
            y2 = y * y
            iteration += 1
        Loop While x2 + y2 <= 4.0 AndAlso iteration < maxIteration
        If iteration = maxIteration Then Return 0
        Return iteration / maxIteration
    End Function

    ' the rows first .. last-1 of a w x w image as bytes
    Function ComputeMandelbrot(first As Integer, last As Integer, w As Integer) As Byte()
        Dim pixels((last - first) * w - 1) As Byte
        Dim xWidth As Double = xMax - xMin, yHeight As Double = yMax - yMin
        Dim k As Integer = 0
        For j As Integer = first To last - 1
            For i As Integer = 0 To w - 1
                Dim x0 As Double = (i + 0.5) * xWidth / w + xMin
                Dim y0 As Double = yMax - (j + 0.5) * yHeight / w
                pixels(k) = CByte(Math.Floor(ComputeEscape(x0, y0) * 256))
                k += 1
            Next
        Next
        Return pixels
    End Function

    ' starts a child process that writes the rows first .. last-1 to the file name; returns its pid
    Function SpawnChild(name As String, first As Integer, last As Integer, width As Integer) As Integer
        Console.WriteLine("spawnChild({0}, {1}, {2}, {3})", name, first, last, width)
        Dim childPid As Integer = CPUSim64.Process.Fork()
        Console.WriteLine("spawnChild result: {0}", childPid)
        If childPid <> 0 Then
            Console.WriteLine("Child {0} forked: {1}", name, childPid)
            Return childPid
        End If
        Console.WriteLine("Child {0} executing...{1} {2} {3}", name, first, last, width)
        File.WriteAllBytes(name, ComputeMandelbrot(first, last, width))
        Console.WriteLine("Child {0} done!", name)
        Environment.Exit(0)
        Return 0
    End Function

    ' merges the band files into filename.pgm and deletes them
    Sub CombineOutput(filename As String, size As Integer, numChildren As Integer)
        Dim parts As New List(Of Byte())
        Dim header As String = "P5" & vbLf & size & " " & size & vbLf & "255" & vbLf
        Dim total As Integer = header.Length
        Console.WriteLine("NUM FILES: {0}", numChildren)
        For i As Integer = 1 To numChildren
            Dim name As String = filename & "_" & i & ".tmp"
            Console.WriteLine("Merging {0}", name)
            If File.Exists(name) Then
                Dim part As Byte() = File.ReadAllBytes(name)
                parts.Add(part)
                total += part.Length
                File.Delete(name)
            End If
        Next
        Dim image(total - 1) As Byte
        Dim k As Integer = 0
        For Each c As Char In header
            image(k) = CByte(AscW(c))
            k += 1
        Next
        For Each part As Byte() In parts
            Array.Copy(part, 0, image, k, part.Length)
            k += part.Length
        Next
        File.WriteAllBytes(filename & ".pgm", image)
    End Sub

    Function Main(args As String()) As Integer
        If args.Length <> 6 Then
            Console.WriteLine("Syntax: mandelbrot_binary x y radius escape_limit image_size filename")
            Return 1
        End If
        Dim x As Double = Double.Parse(args(0))
        Dim y As Double = Double.Parse(args(1))
        Dim radius As Double = Double.Parse(args(2))
        maxIteration = Integer.Parse(args(3))
        Dim imageSize As Integer = Integer.Parse(args(4))
        Dim filename As String = args(5)
        Console.WriteLine("Image: {0:F6}, {1:F6}, {2:F6}", x, y, radius)
        xMin = x - radius
        xMax = x + radius
        yMin = y - radius
        yMax = y + radius
        Console.WriteLine("Image Bounds: {0:F6}, {1:F6}, {2:F6}, {3:F6}", xMin, xMax, yMin, yMax)
        Dim numChildren As Integer = Math.Min(Math.Min(MaxChildren, Environment.ProcessorCount), imageSize)
        Console.WriteLine("Number of children: {0}", numChildren)
        Dim pids(numChildren) As Integer
        Dim firstRow As Integer = 0
        For i As Integer = 1 To numChildren
            ' the band ends at round(imageSize / numChildren * i), rounding half up as the assembly version does
            Dim lastRow As Integer = CInt(Math.Floor(imageSize / numChildren * i + 0.5))
            Dim child As String = filename & "_" & i & ".tmp"
            pids(i) = SpawnChild(child, firstRow, lastRow, imageSize)
            Console.WriteLine("Spawn child for {0}...into {1}", pids(i), child)
            firstRow = lastRow
        Next
        For i As Integer = 1 To numChildren
            Console.WriteLine("Waiting for {0}...", pids(i))
            CPUSim64.Process.Wait(pids(i))
        Next
        CombineOutput(filename, imageSize, numChildren)
        Return 0
    End Function
End Module
