' Linq.vb - System.Linq: the Enumerable extension methods and the
' iterators behind them, part of every program compiled by the CPUSim64
' VB.NET compiler (only the parts a program uses are compiled into it).
' Query expressions (From ... Where ... Select ...) are lowered by the
' compiler to calls of these methods.
'
' The lazy operators (Where, Select, Take, ...) return iterators: classes
' derived from Iterator(Of T), which start their work only when they are
' enumerated, and start again for each enumeration, as in .NET.  OrderBy,
' Reverse, GroupBy and Join read their whole source when the first element
' is asked for.

Imports System.Collections.Generic
Imports System.Runtime.CompilerServices

Namespace System.Linq
    ' ---------------------------------------------------------------- iterators

    Public MustInherit Class Iterator(Of T)
        Implements IEnumerable(Of T), IEnumerator(Of T)
        Protected _current As T

        ' a new iterator of the same sequence, in its initial state
        Protected MustOverride Function Fresh() As Iterator(Of T)
        ' the next element into _current
        Protected MustOverride Function Advance() As Boolean

        Public Function GetEnumerator() As IEnumerator(Of T) Implements IEnumerable(Of T).GetEnumerator
            Return Fresh()
        End Function

        Private Function GetEnumerator2() As System.Collections.IEnumerator Implements System.Collections.IEnumerable.GetEnumerator
            Return Fresh()
        End Function

        Public Function MoveNext() As Boolean Implements System.Collections.IEnumerator.MoveNext
            Return Advance()
        End Function

        Public ReadOnly Property Current As T Implements IEnumerator(Of T).Current
            Get
                Return _current
            End Get
        End Property

        Private ReadOnly Property Current1 As Object Implements System.Collections.IEnumerator.Current
            Get
                Return _current
            End Get
        End Property

        Public Sub Reset() Implements System.Collections.IEnumerator.Reset
            Throw New NotSupportedException()
        End Sub

        Public Sub Dispose() Implements IDisposable.Dispose
        End Sub
    End Class

    Friend NotInheritable Class WhereIterator(Of T)
        Inherits Iterator(Of T)
        Private ReadOnly _source As IEnumerable(Of T)
        Private ReadOnly _pred As Func(Of T, Boolean)
        Private _e As IEnumerator(Of T)
        Public Sub New(source As IEnumerable(Of T), pred As Func(Of T, Boolean))
            _source = source
            _pred = pred
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of T)
            Return New WhereIterator(Of T)(_source, _pred)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _e Is Nothing Then _e = _source.GetEnumerator()
            While _e.MoveNext()
                Dim x = _e.Current
                If _pred(x) Then
                    _current = x
                    Return True
                End If
            End While
            Return False
        End Function
    End Class

    Friend NotInheritable Class SelectIterator(Of T, R)
        Inherits Iterator(Of R)
        Private ReadOnly _source As IEnumerable(Of T)
        Private ReadOnly _f As Func(Of T, R)
        Private _e As IEnumerator(Of T)
        Public Sub New(source As IEnumerable(Of T), f As Func(Of T, R))
            _source = source
            _f = f
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of R)
            Return New SelectIterator(Of T, R)(_source, _f)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _e Is Nothing Then _e = _source.GetEnumerator()
            If Not _e.MoveNext() Then Return False
            _current = _f(_e.Current)
            Return True
        End Function
    End Class

    Friend NotInheritable Class SelectIndexIterator(Of T, R)
        Inherits Iterator(Of R)
        Private ReadOnly _source As IEnumerable(Of T)
        Private ReadOnly _f As Func(Of T, Integer, R)
        Private _e As IEnumerator(Of T)
        Private _i As Integer = -1
        Public Sub New(source As IEnumerable(Of T), f As Func(Of T, Integer, R))
            _source = source
            _f = f
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of R)
            Return New SelectIndexIterator(Of T, R)(_source, _f)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _e Is Nothing Then _e = _source.GetEnumerator()
            If Not _e.MoveNext() Then Return False
            _i += 1
            _current = _f(_e.Current, _i)
            Return True
        End Function
    End Class

    Friend NotInheritable Class SelectManyIterator(Of T, C, R)
        Inherits Iterator(Of R)
        Private ReadOnly _source As IEnumerable(Of T)
        Private ReadOnly _coll As Func(Of T, IEnumerable(Of C))
        Private ReadOnly _res As Func(Of T, C, R)
        Private _e As IEnumerator(Of T)
        Private _inner As IEnumerator(Of C)
        Public Sub New(source As IEnumerable(Of T), coll As Func(Of T, IEnumerable(Of C)), res As Func(Of T, C, R))
            _source = source
            _coll = coll
            _res = res
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of R)
            Return New SelectManyIterator(Of T, C, R)(_source, _coll, _res)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _e Is Nothing Then _e = _source.GetEnumerator()
            Do
                If _inner IsNot Nothing AndAlso _inner.MoveNext() Then
                    _current = _res(_e.Current, _inner.Current)
                    Return True
                End If
                If Not _e.MoveNext() Then Return False
                _inner = _coll(_e.Current).GetEnumerator()
            Loop
        End Function
    End Class

    Friend NotInheritable Class TakeIterator(Of T)
        Inherits Iterator(Of T)
        Private ReadOnly _source As IEnumerable(Of T)
        Private ReadOnly _count As Integer
        Private ReadOnly _skip As Boolean
        Private _e As IEnumerator(Of T)
        Private _n As Integer
        Public Sub New(source As IEnumerable(Of T), count As Integer, skip As Boolean)
            _source = source
            _count = count
            _skip = skip
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of T)
            Return New TakeIterator(Of T)(_source, _count, _skip)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _e Is Nothing Then
                _e = _source.GetEnumerator()
                If _skip Then
                    While _n < _count AndAlso _e.MoveNext()
                        _n += 1
                    End While
                End If
            End If
            If Not _skip Then
                If _n >= _count Then Return False
                _n += 1
            End If
            If Not _e.MoveNext() Then Return False
            _current = _e.Current
            Return True
        End Function
    End Class

    Friend NotInheritable Class WhileIterator(Of T)
        Inherits Iterator(Of T)
        Private ReadOnly _source As IEnumerable(Of T)
        Private ReadOnly _pred As Func(Of T, Boolean)
        Private ReadOnly _skip As Boolean
        Private _e As IEnumerator(Of T)
        Private _done As Boolean
        Private _skipped As Boolean
        Public Sub New(source As IEnumerable(Of T), pred As Func(Of T, Boolean), skip As Boolean)
            _source = source
            _pred = pred
            _skip = skip
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of T)
            Return New WhileIterator(Of T)(_source, _pred, _skip)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _done Then Return False
            If _e Is Nothing Then _e = _source.GetEnumerator()
            If _skip Then
                If Not _skipped Then
                    _skipped = True
                    While _e.MoveNext()
                        If Not _pred(_e.Current) Then
                            _current = _e.Current
                            Return True
                        End If
                    End While
                    _done = True
                    Return False
                End If
                If Not _e.MoveNext() Then Return False
                _current = _e.Current
                Return True
            End If
            If _e.MoveNext() AndAlso _pred(_e.Current) Then
                _current = _e.Current
                Return True
            End If
            _done = True
            Return False
        End Function
    End Class

    Friend NotInheritable Class DistinctIterator(Of T)
        Inherits Iterator(Of T)
        Private ReadOnly _source As IEnumerable(Of T)
        Private _e As IEnumerator(Of T)
        Private _seen As HashSet(Of T)
        Public Sub New(source As IEnumerable(Of T))
            _source = source
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of T)
            Return New DistinctIterator(Of T)(_source)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _e Is Nothing Then
                _e = _source.GetEnumerator()
                _seen = New HashSet(Of T)()
            End If
            While _e.MoveNext()
                If _seen.Add(_e.Current) Then
                    _current = _e.Current
                    Return True
                End If
            End While
            Return False
        End Function
    End Class

    Friend NotInheritable Class ConcatIterator(Of T)
        Inherits Iterator(Of T)
        Private ReadOnly _a As IEnumerable(Of T), _b As IEnumerable(Of T)
        Private _e As IEnumerator(Of T)
        Private _second As Boolean
        Public Sub New(a As IEnumerable(Of T), b As IEnumerable(Of T))
            _a = a
            _b = b
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of T)
            Return New ConcatIterator(Of T)(_a, _b)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _e Is Nothing Then _e = _a.GetEnumerator()
            Do
                If _e.MoveNext() Then
                    _current = _e.Current
                    Return True
                End If
                If _second Then Return False
                _second = True
                _e = _b.GetEnumerator()
            Loop
        End Function
    End Class

    ' a sequence computed on the first MoveNext (Reverse, ...)
    Friend NotInheritable Class BufferIterator(Of T)
        Inherits Iterator(Of T)
        Private ReadOnly _make As Func(Of List(Of T))
        Private _items As List(Of T)
        Private _i As Integer = -1
        Public Sub New(make As Func(Of List(Of T)))
            _make = make
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of T)
            Return New BufferIterator(Of T)(_make)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _items Is Nothing Then _items = _make()
            _i += 1
            If _i >= _items.Count Then Return False
            _current = _items(_i)
            Return True
        End Function
    End Class

    Friend NotInheritable Class RangeIterator
        Inherits Iterator(Of Integer)
        Private ReadOnly _start As Integer, _count As Integer
        Private _i As Integer = -1
        Public Sub New(start As Integer, count As Integer)
            _start = start
            _count = count
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of Integer)
            Return New RangeIterator(_start, _count)
        End Function
        Protected Overrides Function Advance() As Boolean
            _i += 1
            If _i >= _count Then Return False
            _current = _start + _i
            Return True
        End Function
    End Class

    ' OrderBy ... ThenBy: the source and the comparison of all the keys
    Public NotInheritable Class OrderedSequence(Of T)
        Inherits Iterator(Of T)
        Friend ReadOnly Source As IEnumerable(Of T)
        Friend ReadOnly Compare As Comparison(Of T)
        Private _items() As T
        Private _i As Integer = -1
        Friend Sub New(source As IEnumerable(Of T), compare As Comparison(Of T))
            Me.Source = source
            Me.Compare = compare
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of T)
            Return New OrderedSequence(Of T)(Source, Compare)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _items Is Nothing Then
                _items = Enumerable.ToArray(Source)
                CollectionHelpers.SortWith(_items, 0, _items.Length, Compare)
            End If
            _i += 1
            If _i >= _items.Length Then Return False
            _current = _items(_i)
            Return True
        End Function
    End Class

    ' ---------------------------------------------------------------- groups

    Public Interface IGrouping(Of TKey, TElement)
        Inherits IEnumerable(Of TElement)
        ReadOnly Property Key As TKey
    End Interface

    Public NotInheritable Class Grouping(Of TKey, TElement)
        Implements IGrouping(Of TKey, TElement)
        Private ReadOnly _key As TKey
        Friend ReadOnly Items As New List(Of TElement)

        Friend Sub New(key As TKey)
            _key = key
        End Sub

        Public ReadOnly Property Key As TKey Implements IGrouping(Of TKey, TElement).Key
            Get
                Return _key
            End Get
        End Property

        Public Function GetEnumerator() As IEnumerator(Of TElement) Implements IEnumerable(Of TElement).GetEnumerator
            Return Items.GetEnumerator()
        End Function

        Private Function GetEnumerator2() As System.Collections.IEnumerator Implements System.Collections.IEnumerable.GetEnumerator
            Return Items.GetEnumerator()
        End Function
    End Class

    ' ---------------------------------------------------------------- joins

    Friend NotInheritable Class JoinIterator(Of TOuter, TInner, TKey, TResult)
        Inherits Iterator(Of TResult)
        Private ReadOnly _outer As IEnumerable(Of TOuter), _inner As IEnumerable(Of TInner)
        Private ReadOnly _ok As Func(Of TOuter, TKey), _ik As Func(Of TInner, TKey)
        Private ReadOnly _res As Func(Of TOuter, TInner, TResult)
        Private _lookup As Dictionary(Of TKey, List(Of TInner))
        Private _e As IEnumerator(Of TOuter)
        Private _matches As List(Of TInner)
        Private _mi As Integer
        Public Sub New(outer As IEnumerable(Of TOuter), inner As IEnumerable(Of TInner), ok As Func(Of TOuter, TKey),
                       ik As Func(Of TInner, TKey), res As Func(Of TOuter, TInner, TResult))
            _outer = outer
            _inner = inner
            _ok = ok
            _ik = ik
            _res = res
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of TResult)
            Return New JoinIterator(Of TOuter, TInner, TKey, TResult)(_outer, _inner, _ok, _ik, _res)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _lookup Is Nothing Then
                _lookup = Enumerable.MakeLookup(_inner, _ik)
                _e = _outer.GetEnumerator()
            End If
            Do
                If _matches IsNot Nothing AndAlso _mi < _matches.Count Then
                    _current = _res(_e.Current, _matches(_mi))
                    _mi += 1
                    Return True
                End If
                If Not _e.MoveNext() Then Return False
                _matches = Nothing
                _mi = 0
                Dim k = _ok(_e.Current)
                If CObj(k) IsNot Nothing Then _lookup.TryGetValue(k, _matches)
            Loop
        End Function
    End Class

    Friend NotInheritable Class GroupJoinIterator(Of TOuter, TInner, TKey, TResult)
        Inherits Iterator(Of TResult)
        Private ReadOnly _outer As IEnumerable(Of TOuter), _inner As IEnumerable(Of TInner)
        Private ReadOnly _ok As Func(Of TOuter, TKey), _ik As Func(Of TInner, TKey)
        Private ReadOnly _res As Func(Of TOuter, IEnumerable(Of TInner), TResult)
        Private _lookup As Dictionary(Of TKey, List(Of TInner))
        Private _e As IEnumerator(Of TOuter)
        Public Sub New(outer As IEnumerable(Of TOuter), inner As IEnumerable(Of TInner), ok As Func(Of TOuter, TKey),
                       ik As Func(Of TInner, TKey), res As Func(Of TOuter, IEnumerable(Of TInner), TResult))
            _outer = outer
            _inner = inner
            _ok = ok
            _ik = ik
            _res = res
        End Sub
        Protected Overrides Function Fresh() As Iterator(Of TResult)
            Return New GroupJoinIterator(Of TOuter, TInner, TKey, TResult)(_outer, _inner, _ok, _ik, _res)
        End Function
        Protected Overrides Function Advance() As Boolean
            If _lookup Is Nothing Then
                _lookup = Enumerable.MakeLookup(_inner, _ik)
                _e = _outer.GetEnumerator()
            End If
            If Not _e.MoveNext() Then Return False
            Dim matches As List(Of TInner) = Nothing
            Dim k = _ok(_e.Current)
            If CObj(k) IsNot Nothing Then _lookup.TryGetValue(k, matches)
            If matches Is Nothing Then matches = New List(Of TInner)()
            _current = _res(_e.Current, matches)
            Return True
        End Function
    End Class

    ' ---------------------------------------------------------------- Enumerable

    Public Module Enumerable
        Private Function NoElements() As InvalidOperationException
            Return New InvalidOperationException("Sequence contains no elements")
        End Function

        Private Function NoMatch() As InvalidOperationException
            Return New InvalidOperationException("Sequence contains no matching element")
        End Function

        Friend Function MakeLookup(Of T, K)(source As IEnumerable(Of T), key As Func(Of T, K)) As Dictionary(Of K, List(Of T))
            Dim d As New Dictionary(Of K, List(Of T))
            For Each x In source
                Dim kv = key(x)
                If CObj(kv) Is Nothing Then Continue For
                Dim l As List(Of T) = Nothing
                If Not d.TryGetValue(kv, l) Then
                    l = New List(Of T)()
                    d.Add(kv, l)
                End If
                l.Add(x)
            Next
            Return d
        End Function

        ' -------- sources

        Public Function Range(start As Integer, count As Integer) As IEnumerable(Of Integer)
            If count < 0 Then Throw New ArgumentOutOfRangeException("count", "Non-negative number required.")
            Return New RangeIterator(start, count)
        End Function

        Public Function Repeat(Of T)(element As T, count As Integer) As IEnumerable(Of T)
            If count < 0 Then Throw New ArgumentOutOfRangeException("count", "Non-negative number required.")
            Dim l As New List(Of T)(count)
            For i = 1 To count
                l.Add(element)
            Next
            Return l
        End Function

        Public Function Empty(Of T)() As IEnumerable(Of T)
            Return New List(Of T)()
        End Function

        ' -------- lazy operators

        <Extension()>
        Public Function Where(Of T)(source As IEnumerable(Of T), predicate As Func(Of T, Boolean)) As IEnumerable(Of T)
            Return New WhereIterator(Of T)(source, predicate)
        End Function

        <Extension()>
        Public Function [Select](Of T, R)(source As IEnumerable(Of T), selector As Func(Of T, R)) As IEnumerable(Of R)
            Return New SelectIterator(Of T, R)(source, selector)
        End Function

        <Extension()>
        Public Function [Select](Of T, R)(source As IEnumerable(Of T), selector As Func(Of T, Integer, R)) As IEnumerable(Of R)
            Return New SelectIndexIterator(Of T, R)(source, selector)
        End Function

        <Extension()>
        Public Function SelectMany(Of T, R)(source As IEnumerable(Of T), selector As Func(Of T, IEnumerable(Of R))) As IEnumerable(Of R)
            Return New SelectManyIterator(Of T, R, R)(source, selector, Function(x, y) y)
        End Function

        <Extension()>
        Public Function SelectMany(Of T, C, R)(source As IEnumerable(Of T), collectionSelector As Func(Of T, IEnumerable(Of C)),
                                               resultSelector As Func(Of T, C, R)) As IEnumerable(Of R)
            Return New SelectManyIterator(Of T, C, R)(source, collectionSelector, resultSelector)
        End Function

        <Extension()>
        Public Function Take(Of T)(source As IEnumerable(Of T), count As Integer) As IEnumerable(Of T)
            Return New TakeIterator(Of T)(source, count, False)
        End Function

        <Extension()>
        Public Function Skip(Of T)(source As IEnumerable(Of T), count As Integer) As IEnumerable(Of T)
            Return New TakeIterator(Of T)(source, count, True)
        End Function

        <Extension()>
        Public Function TakeWhile(Of T)(source As IEnumerable(Of T), predicate As Func(Of T, Boolean)) As IEnumerable(Of T)
            Return New WhileIterator(Of T)(source, predicate, False)
        End Function

        <Extension()>
        Public Function SkipWhile(Of T)(source As IEnumerable(Of T), predicate As Func(Of T, Boolean)) As IEnumerable(Of T)
            Return New WhileIterator(Of T)(source, predicate, True)
        End Function

        <Extension()>
        Public Function Distinct(Of T)(source As IEnumerable(Of T)) As IEnumerable(Of T)
            Return New DistinctIterator(Of T)(source)
        End Function

        <Extension()>
        Public Function Concat(Of T)(first As IEnumerable(Of T), second As IEnumerable(Of T)) As IEnumerable(Of T)
            Return New ConcatIterator(Of T)(first, second)
        End Function

        <Extension()>
        Public Function Union(Of T)(first As IEnumerable(Of T), second As IEnumerable(Of T)) As IEnumerable(Of T)
            Return New DistinctIterator(Of T)(New ConcatIterator(Of T)(first, second))
        End Function

        <Extension()>
        Public Function Intersect(Of T)(first As IEnumerable(Of T), second As IEnumerable(Of T)) As IEnumerable(Of T)
            Return New BufferIterator(Of T)(Function()
                                                Dim keep As New HashSet(Of T)(second)
                                                Dim r As New List(Of T)
                                                For Each x In first
                                                    If keep.Remove(x) Then r.Add(x)
                                                Next
                                                Return r
                                            End Function)
        End Function

        <Extension()>
        Public Function Except(Of T)(first As IEnumerable(Of T), second As IEnumerable(Of T)) As IEnumerable(Of T)
            Return New BufferIterator(Of T)(Function()
                                                Dim drop As New HashSet(Of T)(second)
                                                Dim r As New List(Of T)
                                                For Each x In first
                                                    If drop.Add(x) Then r.Add(x)
                                                Next
                                                Return r
                                            End Function)
        End Function

        <Extension()>
        Public Function Reverse(Of T)(source As IEnumerable(Of T)) As IEnumerable(Of T)
            Return New BufferIterator(Of T)(Function()
                                                Dim l As New List(Of T)(source)
                                                l.Reverse()
                                                Return l
                                            End Function)
        End Function

        <Extension()>
        Public Function OfType(Of R)(source As System.Collections.IEnumerable) As IEnumerable(Of R)
            Dim l As New List(Of R)
            For Each o In source
                If TypeOf o Is R Then l.Add(DirectCast(o, R))
            Next
            Return l
        End Function

        <Extension()>
        Public Function Cast(Of R)(source As System.Collections.IEnumerable) As IEnumerable(Of R)
            Dim l As New List(Of R)
            For Each o In source
                l.Add(CType(o, R))
            Next
            Return l
        End Function

        ' -------- ordering

        <Extension()>
        Public Function OrderBy(Of T, K)(source As IEnumerable(Of T), keySelector As Func(Of T, K)) As OrderedSequence(Of T)
            Return New OrderedSequence(Of T)(source, Function(a, b) CollectionHelpers.CompareOf(keySelector(a), keySelector(b)))
        End Function

        <Extension()>
        Public Function OrderByDescending(Of T, K)(source As IEnumerable(Of T), keySelector As Func(Of T, K)) As OrderedSequence(Of T)
            Return New OrderedSequence(Of T)(source, Function(a, b) CollectionHelpers.CompareOf(keySelector(b), keySelector(a)))
        End Function

        <Extension()>
        Public Function ThenBy(Of T, K)(source As OrderedSequence(Of T), keySelector As Func(Of T, K)) As OrderedSequence(Of T)
            Dim first = source.Compare
            Return New OrderedSequence(Of T)(source.Source,
                Function(a, b)
                    Dim c = first(a, b)
                    If c <> 0 Then Return c
                    Return CollectionHelpers.CompareOf(keySelector(a), keySelector(b))
                End Function)
        End Function

        <Extension()>
        Public Function ThenByDescending(Of T, K)(source As OrderedSequence(Of T), keySelector As Func(Of T, K)) As OrderedSequence(Of T)
            Dim first = source.Compare
            Return New OrderedSequence(Of T)(source.Source,
                Function(a, b)
                    Dim c = first(a, b)
                    If c <> 0 Then Return c
                    Return CollectionHelpers.CompareOf(keySelector(b), keySelector(a))
                End Function)
        End Function

        ' -------- grouping and joining

        <Extension()>
        Public Function GroupBy(Of T, K)(source As IEnumerable(Of T), keySelector As Func(Of T, K)) As IEnumerable(Of Grouping(Of K, T))
            Return GroupBy(source, keySelector, Function(x) x)
        End Function

        <Extension()>
        Public Function GroupBy(Of T, K, E)(source As IEnumerable(Of T), keySelector As Func(Of T, K),
                                            elementSelector As Func(Of T, E)) As IEnumerable(Of Grouping(Of K, E))
            Return New BufferIterator(Of Grouping(Of K, E))(
                Function()
                    Dim groups As New List(Of Grouping(Of K, E))
                    Dim byKey As New Dictionary(Of K, Grouping(Of K, E))
                    Dim nullGroup As Grouping(Of K, E) = Nothing
                    For Each x In source
                        Dim kv = keySelector(x)
                        Dim g As Grouping(Of K, E) = Nothing
                        If CObj(kv) Is Nothing Then
                            If nullGroup Is Nothing Then
                                nullGroup = New Grouping(Of K, E)(kv)
                                groups.Add(nullGroup)
                            End If
                            g = nullGroup
                        ElseIf Not byKey.TryGetValue(kv, g) Then
                            g = New Grouping(Of K, E)(kv)
                            byKey.Add(kv, g)
                            groups.Add(g)
                        End If
                        g.Items.Add(elementSelector(x))
                    Next
                    Return groups
                End Function)
        End Function

        <Extension()>
        Public Function Join(Of TOuter, TInner, TKey, TResult)(outer As IEnumerable(Of TOuter), inner As IEnumerable(Of TInner),
                                                               outerKeySelector As Func(Of TOuter, TKey),
                                                               innerKeySelector As Func(Of TInner, TKey),
                                                               resultSelector As Func(Of TOuter, TInner, TResult)) As IEnumerable(Of TResult)
            Return New JoinIterator(Of TOuter, TInner, TKey, TResult)(outer, inner, outerKeySelector, innerKeySelector, resultSelector)
        End Function

        <Extension()>
        Public Function GroupJoin(Of TOuter, TInner, TKey, TResult)(outer As IEnumerable(Of TOuter), inner As IEnumerable(Of TInner),
                                                                    outerKeySelector As Func(Of TOuter, TKey),
                                                                    innerKeySelector As Func(Of TInner, TKey),
                                                                    resultSelector As Func(Of TOuter, IEnumerable(Of TInner), TResult)) As IEnumerable(Of TResult)
            Return New GroupJoinIterator(Of TOuter, TInner, TKey, TResult)(outer, inner, outerKeySelector, innerKeySelector, resultSelector)
        End Function

        ' -------- conversions

        <Extension()>
        Public Function ToList(Of T)(source As IEnumerable(Of T)) As List(Of T)
            If source Is Nothing Then Throw New ArgumentNullException("source")
            Return New List(Of T)(source)
        End Function

        <Extension()>
        Public Function ToArray(Of T)(source As IEnumerable(Of T)) As T()
            If source Is Nothing Then Throw New ArgumentNullException("source")
            Return New List(Of T)(source).ToArray()
        End Function

        <Extension()>
        Public Function ToDictionary(Of T, K)(source As IEnumerable(Of T), keySelector As Func(Of T, K)) As Dictionary(Of K, T)
            Dim d As New Dictionary(Of K, T)
            For Each x In source
                d.Add(keySelector(x), x)
            Next
            Return d
        End Function

        <Extension()>
        Public Function ToDictionary(Of T, K, V)(source As IEnumerable(Of T), keySelector As Func(Of T, K),
                                                 elementSelector As Func(Of T, V)) As Dictionary(Of K, V)
            Dim d As New Dictionary(Of K, V)
            For Each x In source
                d.Add(keySelector(x), elementSelector(x))
            Next
            Return d
        End Function

        <Extension()>
        Public Function ToHashSet(Of T)(source As IEnumerable(Of T)) As HashSet(Of T)
            Return New HashSet(Of T)(source)
        End Function

        ' -------- elements

        <Extension()>
        Public Function First(Of T)(source As IEnumerable(Of T)) As T
            For Each x In source
                Return x
            Next
            Throw NoElements()
        End Function

        <Extension()>
        Public Function First(Of T)(source As IEnumerable(Of T), predicate As Func(Of T, Boolean)) As T
            For Each x In source
                If predicate(x) Then Return x
            Next
            Throw NoMatch()
        End Function

        <Extension()>
        Public Function FirstOrDefault(Of T)(source As IEnumerable(Of T)) As T
            For Each x In source
                Return x
            Next
            Return Nothing
        End Function

        <Extension()>
        Public Function FirstOrDefault(Of T)(source As IEnumerable(Of T), predicate As Func(Of T, Boolean)) As T
            For Each x In source
                If predicate(x) Then Return x
            Next
            Return Nothing
        End Function

        <Extension()>
        Public Function Last(Of T)(source As IEnumerable(Of T)) As T
            Dim found = False
            Dim r As T = Nothing
            For Each x In source
                r = x
                found = True
            Next
            If Not found Then Throw NoElements()
            Return r
        End Function

        <Extension()>
        Public Function Last(Of T)(source As IEnumerable(Of T), predicate As Func(Of T, Boolean)) As T
            Dim found = False
            Dim r As T = Nothing
            For Each x In source
                If predicate(x) Then
                    r = x
                    found = True
                End If
            Next
            If Not found Then Throw NoMatch()
            Return r
        End Function

        <Extension()>
        Public Function LastOrDefault(Of T)(source As IEnumerable(Of T)) As T
            Dim r As T = Nothing
            For Each x In source
                r = x
            Next
            Return r
        End Function

        <Extension()>
        Public Function [Single](Of T)(source As IEnumerable(Of T)) As T
            Dim n = 0
            Dim r As T = Nothing
            For Each x In source
                n += 1
                If n > 1 Then Throw New InvalidOperationException("Sequence contains more than one element")
                r = x
            Next
            If n = 0 Then Throw NoElements()
            Return r
        End Function

        <Extension()>
        Public Function ElementAt(Of T)(source As IEnumerable(Of T), index As Integer) As T
            Dim i = 0
            For Each x In source
                If i = index Then Return x
                i += 1
            Next
            Throw New ArgumentOutOfRangeException("index", "Index was out of range. Must be non-negative and less than the size of the collection.")
        End Function

        ' -------- tests and counts

        <Extension()>
        Public Function Any(Of T)(source As IEnumerable(Of T)) As Boolean
            For Each x In source
                Return True
            Next
            Return False
        End Function

        <Extension()>
        Public Function Any(Of T)(source As IEnumerable(Of T), predicate As Func(Of T, Boolean)) As Boolean
            For Each x In source
                If predicate(x) Then Return True
            Next
            Return False
        End Function

        <Extension()>
        Public Function All(Of T)(source As IEnumerable(Of T), predicate As Func(Of T, Boolean)) As Boolean
            For Each x In source
                If Not predicate(x) Then Return False
            Next
            Return True
        End Function

        <Extension()>
        Public Function Contains(Of T)(source As IEnumerable(Of T), value As T) As Boolean
            For Each x In source
                If CollectionHelpers.AreEqual(x, value) Then Return True
            Next
            Return False
        End Function

        <Extension()>
        Public Function Count(Of T)(source As IEnumerable(Of T)) As Integer
            Dim n = 0
            For Each x In source
                n += 1
            Next
            Return n
        End Function

        <Extension()>
        Public Function Count(Of T)(source As IEnumerable(Of T), predicate As Func(Of T, Boolean)) As Integer
            Dim n = 0
            For Each x In source
                If predicate(x) Then n += 1
            Next
            Return n
        End Function

        <Extension()>
        Public Function SequenceEqual(Of T)(first As IEnumerable(Of T), second As IEnumerable(Of T)) As Boolean
            Dim a = first.ToList(), b = second.ToList()
            If a.Count <> b.Count Then Return False
            For i = 0 To a.Count - 1
                If Not CollectionHelpers.AreEqual(a(i), b(i)) Then Return False
            Next
            Return True
        End Function

        ' -------- aggregates

        <Extension()>
        Public Function Sum(source As IEnumerable(Of Integer)) As Integer
            Dim s = 0
            For Each x In source
                s += x
            Next
            Return s
        End Function

        <Extension()>
        Public Function Sum(source As IEnumerable(Of Long)) As Long
            Dim s As Long = 0
            For Each x In source
                s += x
            Next
            Return s
        End Function

        <Extension()>
        Public Function Sum(source As IEnumerable(Of Double)) As Double
            Dim s As Double = 0
            For Each x In source
                s += x
            Next
            Return s
        End Function

        <Extension()>
        Public Function Sum(Of T)(source As IEnumerable(Of T), selector As Func(Of T, Integer)) As Integer
            Dim s = 0
            For Each x In source
                s += selector(x)
            Next
            Return s
        End Function

        <Extension()>
        Public Function Sum(Of T)(source As IEnumerable(Of T), selector As Func(Of T, Long)) As Long
            Dim s As Long = 0
            For Each x In source
                s += selector(x)
            Next
            Return s
        End Function

        <Extension()>
        Public Function Sum(Of T)(source As IEnumerable(Of T), selector As Func(Of T, Double)) As Double
            Dim s As Double = 0
            For Each x In source
                s += selector(x)
            Next
            Return s
        End Function

        <Extension()>
        Public Function Average(source As IEnumerable(Of Integer)) As Double
            Dim s As Long = 0, n = 0
            For Each x In source
                s += x
                n += 1
            Next
            If n = 0 Then Throw NoElements()
            Return s / n
        End Function

        <Extension()>
        Public Function Average(source As IEnumerable(Of Double)) As Double
            Dim s As Double = 0, n = 0
            For Each x In source
                s += x
                n += 1
            Next
            If n = 0 Then Throw NoElements()
            Return s / n
        End Function

        <Extension()>
        Public Function Average(Of T)(source As IEnumerable(Of T), selector As Func(Of T, Integer)) As Double
            Dim s As Long = 0, n = 0
            For Each x In source
                s += selector(x)
                n += 1
            Next
            If n = 0 Then Throw NoElements()
            Return s / n
        End Function

        <Extension()>
        Public Function Average(Of T)(source As IEnumerable(Of T), selector As Func(Of T, Double)) As Double
            Dim s As Double = 0, n = 0
            For Each x In source
                s += selector(x)
                n += 1
            Next
            If n = 0 Then Throw NoElements()
            Return s / n
        End Function

        <Extension()>
        Public Function Min(Of T)(source As IEnumerable(Of T)) As T
            Dim found = False
            Dim r As T = Nothing
            For Each x In source
                If Not found OrElse CollectionHelpers.CompareOf(x, r) < 0 Then r = x
                found = True
            Next
            If Not found Then Throw NoElements()
            Return r
        End Function

        <Extension()>
        Public Function Max(Of T)(source As IEnumerable(Of T)) As T
            Dim found = False
            Dim r As T = Nothing
            For Each x In source
                If Not found OrElse CollectionHelpers.CompareOf(x, r) > 0 Then r = x
                found = True
            Next
            If Not found Then Throw NoElements()
            Return r
        End Function

        <Extension()>
        Public Function Min(Of T, R)(source As IEnumerable(Of T), selector As Func(Of T, R)) As R
            Return Min(New SelectIterator(Of T, R)(source, selector))
        End Function

        <Extension()>
        Public Function Max(Of T, R)(source As IEnumerable(Of T), selector As Func(Of T, R)) As R
            Return Max(New SelectIterator(Of T, R)(source, selector))
        End Function

        <Extension()>
        Public Function Aggregate(Of T)(source As IEnumerable(Of T), func As Func(Of T, T, T)) As T
            Dim found = False
            Dim r As T = Nothing
            For Each x In source
                r = If(found, func(r, x), x)
                found = True
            Next
            If Not found Then Throw NoElements()
            Return r
        End Function

        <Extension()>
        Public Function Aggregate(Of T, A)(source As IEnumerable(Of T), seed As A, func As Func(Of A, T, A)) As A
            Dim r = seed
            For Each x In source
                r = func(r, x)
            Next
            Return r
        End Function
    End Module
End Namespace
