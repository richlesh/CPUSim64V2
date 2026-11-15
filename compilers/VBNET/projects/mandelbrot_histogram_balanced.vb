' mandelbrot_histogram_balanced.vb
'
' Multi-threaded Mandelbrot set image generator with histogram coloring.  It
' renders the set as a binary (P5) PGM image.  One thread per processor core
' (at most MaxThreads) takes the next row of the image with
' Interlocked.Increment until none is left, so the threads stay busy to the
' end although some rows take much longer than others.  Each thread stores
' the iteration count of every pixel of its rows in a shared image buffer.
' The main thread then counts how many pixels have each iteration count and
' gives a pixel the gray level 256 * (the fraction of the pixels with an
' iteration count up to its own), which spreads the gray levels evenly over
' the image.  Pixels in the set (count 0) stay black.
' VB.NET version of documentation/projects/mandelbrot_histogram_balanced.asm.
'
' Usage:
'     mandelbrot_histogram_balanced x y radius escape_limit image_size filename
'
'     x, y         - the center of the view (floats)
'     radius       - half the width and height of the view (float)
'     escape_limit - the iterations after which a point counts as in the set
'     image_size   - the width and height of the image in pixels
'     filename     - the image is written to filename.pgm
'
' Example (the image of the documentation):
'     > mandelbrot_histogram_balanced -0.748127681290498 0.09408613781852182 1e-6 10000 128 mandelbrot_histogram_balanced
'
' Author:   Richard Lesh
Imports System.IO
Imports System.Threading

Module mandelbrot_histogram_balanced
    Const MaxThreads As Integer = 100

    Dim xMin As Double = -2.5, xMax As Double = 0.5, yMin As Double = -1.5, yMax As Double = 1.5
    Dim maxIteration As Integer = 50
    Dim image As Integer()                      ' the iteration count of every pixel, row by row

    Dim counter As Integer = -1                 ' the last row taken

    ' the number of iterations of the point (x0, y0) before it escapes, 0 if it never does
    Function ComputeEscape(x0 As Double, y0 As Double) As Integer
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
        Return iteration
    End Function

    ' fills in the rows first .. last-1 of the image buffer
    Sub ComputeMandelbrot(first As Integer, last As Integer, w As Integer)
        Dim xWidth As Double = xMax - xMin, yHeight As Double = yMax - yMin
        Dim inverseWidth As Double = 1.0 / w
        For j As Integer = first To last - 1
            Dim y0 As Double = yMax - (j + 0.5) * yHeight * inverseWidth
            For i As Integer = 0 To w - 1
                Dim x0 As Double = (i + 0.5) * xWidth * inverseWidth + xMin
                image(j * w + i) = ComputeEscape(x0, y0)
            Next
        Next
    End Sub

    ' the thread function: computes rows of a width x width image until there are none left
    Sub Run(data As Object)
        Dim w As Integer = CInt(data)
        Dim pid As Integer = Thread.CurrentThread.ManagedThreadId
        Console.WriteLine("Thread {0} executing...{1}", pid, w)
        Do
            Dim row As Integer = Interlocked.Increment(counter)
            If row >= w Then Exit Do
            ComputeMandelbrot(row, row + 1, w)
        Loop
        Console.WriteLine("Thread {0} done!", pid)
    End Sub

    ' writes the image as filename.pgm with the gray levels from the histogram
    Sub CombineOutput(filename As String, size As Integer)
        Dim pixels As Integer = size * size
        Dim histogram(maxIteration - 1) As Integer
        Dim total As Integer = 0
        For i As Integer = 0 To pixels - 1          ' the number of pixels of each iteration count
            If image(i) > 0 Then
                histogram(image(i)) += 1
                total += 1
            End If
        Next
        Dim subtotal As Integer = 0
        For i As Integer = 0 To maxIteration - 1    ' the gray level of an iteration count
            subtotal += histogram(i)
            histogram(i) = CInt(Math.Floor(subtotal / total * 256))
        Next
        Dim header As String = "P5" & vbLf & size & " " & size & vbLf & "255" & vbLf
        Dim output(header.Length + pixels - 1) As Byte
        For i As Integer = 0 To header.Length - 1
            output(i) = CByte(AscW(header(i)))
        Next
        For i As Integer = 0 To pixels - 1
            ' the low byte, as the assembly version writes it (the last count has level 256, which becomes 0)
            output(header.Length + i) = CByte(histogram(image(i)) And 255)
        Next
        File.WriteAllBytes(filename & ".pgm", output)
    End Sub

    Function Main(args As String()) As Integer
        If args.Length <> 6 Then
            Console.WriteLine("Syntax: mandelbrot_histogram_balanced x y radius escape_limit image_size filename")
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
        ReDim image(imageSize * imageSize - 1)
        Dim numThreads As Integer = Math.Min(Math.Min(MaxThreads, Environment.ProcessorCount), imageSize)
        Console.WriteLine("Number of threads: {0}", numThreads)
        Dim threads(numThreads) As Thread
        For i As Integer = 1 To numThreads
            threads(i) = New Thread(AddressOf Run)
            threads(i).Start(imageSize)
            Console.WriteLine("Spawn thread for {0}", threads(i).ManagedThreadId)
        Next
        For i As Integer = 1 To numThreads
            Console.WriteLine("Waiting for {0}...", threads(i).ManagedThreadId)
            threads(i).Join()
        Next
        CombineOutput(filename, imageSize)
        Return 0
    End Function
End Module
