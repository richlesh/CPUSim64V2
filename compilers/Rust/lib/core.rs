// lib/core.rs - the Rust prelude of the CPUSim64 translator, parsed as the
// crate `core` before every program (`std` and `alloc` are aliases of it).
// Language items are marked #[lang = "..."]; compiler built-ins are the
// functions of `extern "rust-intrinsic"` in `intrinsics`.  lib/README.md lists
// every lang item and intrinsic with its exact semantics.
//
// Conventions: macros below use absolute `::core::` paths; `unsafe` is not
// required around intrinsic calls (the checker does not enforce it); integer
// types are i8..i64, isize, u8..u64, usize (no 128 bit types).

// ---------------------------------------------------------------- helper macros

// Binary operator traits for primitive numbers: T op T, T op &T, &T op T, &T op &T.
macro_rules! binop_impls {
    ($Tr:ident, $m:ident, $op:tt, $($t:ident)*) => { $(
        impl ::core::ops::$Tr for $t {
            type Output = $t;
            fn $m(self, o: $t) -> $t { self $op o }
        }
        impl<'a> ::core::ops::$Tr<&'a $t> for $t {
            type Output = $t;
            fn $m(self, o: &'a $t) -> $t { self $op *o }
        }
        impl<'a> ::core::ops::$Tr<$t> for &'a $t {
            type Output = $t;
            fn $m(self, o: $t) -> $t { *self $op o }
        }
        impl<'a, 'b> ::core::ops::$Tr<&'a $t> for &'b $t {
            type Output = $t;
            fn $m(self, o: &'a $t) -> $t { *self $op *o }
        }
    )* }
}

// Same-type-only binary operators (bit operations, shifts).
macro_rules! binop_same {
    ($Tr:ident, $m:ident, $op:tt, $($t:ident)*) => { $(
        impl ::core::ops::$Tr for $t {
            type Output = $t;
            fn $m(self, o: $t) -> $t { self $op o }
        }
    )* }
}

// Compound assignment: T op= T and T op= &T.
macro_rules! assignop_impls {
    ($Tr:ident, $m:ident, $op:tt, $($t:ident)*) => { $(
        impl ::core::ops::$Tr for $t {
            fn $m(&mut self, o: $t) { *self $op o }
        }
        impl<'a> ::core::ops::$Tr<&'a $t> for $t {
            fn $m(&mut self, o: &'a $t) { *self $op *o }
        }
    )* }
}

macro_rules! assignop_same {
    ($Tr:ident, $m:ident, $op:tt, $($t:ident)*) => { $(
        impl ::core::ops::$Tr for $t {
            fn $m(&mut self, o: $t) { *self $op o }
        }
    )* }
}

macro_rules! arith_all {
    ($($t:ident)*) => {
        binop_impls!(Add, add, +, $($t)*);
        binop_impls!(Sub, sub, -, $($t)*);
        binop_impls!(Mul, mul, *, $($t)*);
        binop_impls!(Div, div, /, $($t)*);
        binop_impls!(Rem, rem, %, $($t)*);
        assignop_impls!(AddAssign, add_assign, +=, $($t)*);
        assignop_impls!(SubAssign, sub_assign, -=, $($t)*);
        assignop_impls!(MulAssign, mul_assign, *=, $($t)*);
        assignop_impls!(DivAssign, div_assign, /=, $($t)*);
        assignop_impls!(RemAssign, rem_assign, %=, $($t)*);
    }
}

macro_rules! bit_all {
    ($($t:ident)*) => {
        binop_same!(BitAnd, bitand, &, $($t)*);
        binop_same!(BitOr, bitor, |, $($t)*);
        binop_same!(BitXor, bitxor, ^, $($t)*);
        assignop_same!(BitAndAssign, bitand_assign, &=, $($t)*);
        assignop_same!(BitOrAssign, bitor_assign, |=, $($t)*);
        assignop_same!(BitXorAssign, bitxor_assign, ^=, $($t)*);
        $( impl ::core::ops::Not for $t {
            type Output = $t;
            fn not(self) -> $t { !self }
        } )*
    }
}

macro_rules! shift_all {
    ($($t:ident)*) => {
        binop_same!(Shl, shl, <<, $($t)*);
        binop_same!(Shr, shr, >>, $($t)*);
        assignop_same!(ShlAssign, shl_assign, <<=, $($t)*);
        assignop_same!(ShrAssign, shr_assign, >>=, $($t)*);
    }
}

// Clone, Default, PartialEq, PartialOrd for every primitive.
macro_rules! clone_default {
    ($($t:ident, $d:expr;)*) => { $(
        impl ::core::clone::Clone for $t { fn clone(&self) -> $t { *self } }
        impl ::core::default::Default for $t { fn default() -> $t { $d } }
    )* }
}

macro_rules! cmp_total {
    ($($t:ident)*) => { $(
        impl ::core::cmp::PartialEq for $t {
            fn eq(&self, o: &$t) -> bool { *self == *o }
            fn ne(&self, o: &$t) -> bool { *self != *o }
        }
        impl ::core::cmp::Eq for $t {}
        impl ::core::cmp::PartialOrd for $t {
            fn partial_cmp(&self, o: &$t) -> ::core::option::Option<::core::cmp::Ordering> {
                ::core::option::Option::Some(::core::cmp::Ord::cmp(self, o))
            }
            fn lt(&self, o: &$t) -> bool { *self < *o }
            fn le(&self, o: &$t) -> bool { *self <= *o }
            fn gt(&self, o: &$t) -> bool { *self > *o }
            fn ge(&self, o: &$t) -> bool { *self >= *o }
        }
        impl ::core::cmp::Ord for $t {
            fn cmp(&self, o: &$t) -> ::core::cmp::Ordering {
                if *self < *o { ::core::cmp::Ordering::Less }
                else if *self == *o { ::core::cmp::Ordering::Equal }
                else { ::core::cmp::Ordering::Greater }
            }
        }
    )* }
}

// Integers: associated consts and methods, formatting, ranges, sums, parsing.
// $t type, $ut its unsigned counterpart, $min / $max limits, $bits width.
macro_rules! int_impls {
    ($t:ident, $ut:ident, $min:expr, $max:expr, $bits:expr) => {
        impl $t {
            pub const MIN: $t = $min;
            pub const MAX: $t = $max;
            pub const BITS: u32 = $bits;
            pub fn min_value() -> $t { $min }
            pub fn max_value() -> $t { $max }
            pub fn wrapping_add(self, o: $t) -> $t { ::core::intrinsics::wrapping_add::<$t>(self, o) }
            pub fn wrapping_sub(self, o: $t) -> $t { ::core::intrinsics::wrapping_sub::<$t>(self, o) }
            pub fn wrapping_mul(self, o: $t) -> $t { ::core::intrinsics::wrapping_mul::<$t>(self, o) }
            pub fn overflowing_add(self, o: $t) -> ($t, bool) { ::core::intrinsics::add_with_overflow::<$t>(self, o) }
            pub fn overflowing_sub(self, o: $t) -> ($t, bool) { ::core::intrinsics::sub_with_overflow::<$t>(self, o) }
            pub fn overflowing_mul(self, o: $t) -> ($t, bool) { ::core::intrinsics::mul_with_overflow::<$t>(self, o) }
            pub fn checked_add(self, o: $t) -> ::core::option::Option<$t> {
                let r = ::core::intrinsics::add_with_overflow::<$t>(self, o);
                if r.1 { ::core::option::Option::None } else { ::core::option::Option::Some(r.0) }
            }
            pub fn checked_sub(self, o: $t) -> ::core::option::Option<$t> {
                let r = ::core::intrinsics::sub_with_overflow::<$t>(self, o);
                if r.1 { ::core::option::Option::None } else { ::core::option::Option::Some(r.0) }
            }
            pub fn checked_mul(self, o: $t) -> ::core::option::Option<$t> {
                let r = ::core::intrinsics::mul_with_overflow::<$t>(self, o);
                if r.1 { ::core::option::Option::None } else { ::core::option::Option::Some(r.0) }
            }
            pub fn checked_div(self, o: $t) -> ::core::option::Option<$t> {
                if o == 0 { ::core::option::Option::None } else { ::core::option::Option::Some(self / o) }
            }
            pub fn checked_rem(self, o: $t) -> ::core::option::Option<$t> {
                if o == 0 { ::core::option::Option::None } else { ::core::option::Option::Some(self % o) }
            }
            pub fn saturating_add(self, o: $t) -> $t {
                match self.checked_add(o) {
                    ::core::option::Option::Some(v) => v,
                    ::core::option::Option::None => if o < 0 { $min } else { $max },
                }
            }
            pub fn saturating_sub(self, o: $t) -> $t {
                match self.checked_sub(o) {
                    ::core::option::Option::Some(v) => v,
                    ::core::option::Option::None => if o < 0 { $max } else { $min },
                }
            }
            pub fn pow(self, exp: u32) -> $t {
                let mut r: $t = 1;
                let mut b: $t = self;
                let mut e: u32 = exp;
                while e > 0 {
                    if e & 1 == 1 { r = r * b; }
                    e = e >> 1;
                    if e > 0 { b = b * b; }
                }
                r
            }
            pub fn rem_euclid(self, o: $t) -> $t {
                let r = self % o;
                if r < 0 { if o < 0 { r - o } else { r + o } } else { r }
            }
            pub fn abs_diff(self, o: $t) -> $ut {
                if self < o { ::core::intrinsics::wrapping_sub::<$ut>(o as $ut, self as $ut) }
                else { ::core::intrinsics::wrapping_sub::<$ut>(self as $ut, o as $ut) }
            }
            pub fn count_ones(self) -> u32 {
                let mut x: $ut = self as $ut;
                let mut n: u32 = 0;
                while x != 0 { n += (x & 1) as u32; x = x >> 1; }
                n
            }
            pub fn is_power_of_two(self) -> bool { self > 0 && self.count_ones() == 1 }
            pub fn wrapping_neg(self) -> $t { ::core::intrinsics::wrapping_sub::<$t>(0, self) }
            pub fn checked_neg(self) -> ::core::option::Option<$t> {
                if self == 0 { ::core::option::Option::Some(0) }
                else if $min == 0 { ::core::option::Option::None }
                else if self == $min { ::core::option::Option::None }
                else { ::core::option::Option::Some(::core::intrinsics::wrapping_sub::<$t>(0, self)) }
            }
            pub fn checked_pow(self, exp: u32) -> ::core::option::Option<$t> {
                let mut r: $t = 1;
                let mut b: $t = self;
                let mut e: u32 = exp;
                while e > 0 {
                    if e & 1 == 1 {
                        match r.checked_mul(b) {
                            ::core::option::Option::Some(v) => { r = v; }
                            ::core::option::Option::None => { return ::core::option::Option::None; }
                        }
                    }
                    e = e >> 1;
                    if e > 0 {
                        match b.checked_mul(b) {
                            ::core::option::Option::Some(v) => { b = v; }
                            ::core::option::Option::None => { return ::core::option::Option::None; }
                        }
                    }
                }
                ::core::option::Option::Some(r)
            }
            pub fn wrapping_pow(self, exp: u32) -> $t {
                let mut r: $t = 1;
                let mut b: $t = self;
                let mut e: u32 = exp;
                while e > 0 {
                    if e & 1 == 1 { r = r.wrapping_mul(b); }
                    e = e >> 1;
                    if e > 0 { b = b.wrapping_mul(b); }
                }
                r
            }
            pub fn saturating_mul(self, o: $t) -> $t {
                match self.checked_mul(o) {
                    ::core::option::Option::Some(v) => v,
                    ::core::option::Option::None => if (self < 0) != (o < 0) { $min } else { $max },
                }
            }
            pub fn leading_zeros(self) -> u32 {
                let mut x: $ut = self as $ut;
                let mut n: u32 = 0;
                while x != 0 { n += 1; x = x >> 1; }
                $bits - n
            }
            pub fn trailing_zeros(self) -> u32 {
                let mut x: $ut = self as $ut;
                if x == 0 { return $bits; }
                let mut n: u32 = 0;
                while x & 1 == 0 { n += 1; x = x >> 1; }
                n
            }
            pub fn count_zeros(self) -> u32 { $bits - self.count_ones() }
            pub fn from_str_radix(s: &str, radix: u32) -> ::core::result::Result<$t, ::core::num::ParseIntError> {
                let mn: $t = $min;
                let mx: $t = $max;
                match ::core::num::parse_int(s, mn == 0, radix) {
                    ::core::result::Result::Err(e) => ::core::result::Result::Err(e),
                    ::core::result::Result::Ok(p) => {
                        // p = (negative, magnitude)
                        if p.0 {
                            let lim: u64 = if mn == 0 { 0 } else { ::core::intrinsics::wrapping_sub::<u64>(0, mn as u64) };
                            if p.1 > lim { ::core::result::Result::Err(::core::num::ParseIntError { kind: 3 }) }
                            else { ::core::result::Result::Ok(::core::intrinsics::wrapping_sub::<u64>(0, p.1) as $t) }
                        } else if p.1 > (mx as u64) {
                            ::core::result::Result::Err(::core::num::ParseIntError { kind: 2 })
                        } else {
                            ::core::result::Result::Ok(p.1 as $t)
                        }
                    }
                }
            }
        }
        impl ::core::fmt::Display for $t {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                let n = *self;
                let nonneg = n >= 0;
                let mag: u64 = if nonneg { n as u64 } else { ::core::intrinsics::wrapping_sub::<u64>(0, n as u64) };
                ::core::num::fmt_radix(f, nonneg, mag, 10, false, "")
            }
        }
        impl ::core::fmt::Debug for $t {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                ::core::fmt::Display::fmt(self, f)
            }
        }
        impl ::core::fmt::LowerHex for $t {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                ::core::num::fmt_radix(f, true, (*self as $ut) as u64, 16, false, "0x")
            }
        }
        impl ::core::fmt::UpperHex for $t {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                ::core::num::fmt_radix(f, true, (*self as $ut) as u64, 16, true, "0x")
            }
        }
        impl ::core::fmt::Octal for $t {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                ::core::num::fmt_radix(f, true, (*self as $ut) as u64, 8, false, "0o")
            }
        }
        impl ::core::fmt::Binary for $t {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                ::core::num::fmt_radix(f, true, (*self as $ut) as u64, 2, false, "0b")
            }
        }
        impl ::core::iter::Step for $t {
            fn succ(&self) -> $t { *self + 1 }
            fn pred(&self) -> $t { *self - 1 }
        }
        impl ::core::iter::Sum<$t> for $t {
            fn sum<I: ::core::iter::Iterator<Item = $t>>(iter: I) -> $t {
                let mut a: $t = 0;
                let mut it = iter;
                while let ::core::option::Option::Some(x) = it.next() { a = a + x; }
                a
            }
        }
        impl<'a> ::core::iter::Sum<&'a $t> for $t {
            fn sum<I: ::core::iter::Iterator<Item = &'a $t>>(iter: I) -> $t {
                let mut a: $t = 0;
                let mut it = iter;
                while let ::core::option::Option::Some(x) = it.next() { a = a + *x; }
                a
            }
        }
        impl ::core::iter::Product<$t> for $t {
            fn product<I: ::core::iter::Iterator<Item = $t>>(iter: I) -> $t {
                let mut a: $t = 1;
                let mut it = iter;
                while let ::core::option::Option::Some(x) = it.next() { a = a * x; }
                a
            }
        }
        impl<'a> ::core::iter::Product<&'a $t> for $t {
            fn product<I: ::core::iter::Iterator<Item = &'a $t>>(iter: I) -> $t {
                let mut a: $t = 1;
                let mut it = iter;
                while let ::core::option::Option::Some(x) = it.next() { a = a * *x; }
                a
            }
        }
        impl ::core::str::FromStr for $t {
            type Err = ::core::num::ParseIntError;
            fn from_str(s: &str) -> ::core::result::Result<$t, ::core::num::ParseIntError> {
                $t::from_str_radix(s, 10)
            }
        }
    }
}

macro_rules! int_signed {
    ($($t:ident)*) => { $(
        impl $t {
            pub fn abs(self) -> $t { if self < 0 { -self } else { self } }
            pub fn signum(self) -> $t { if self < 0 { -1 } else if self == 0 { 0 } else { 1 } }
            pub fn is_negative(self) -> bool { self < 0 }
            pub fn is_positive(self) -> bool { self > 0 }
            pub fn checked_abs(self) -> ::core::option::Option<$t> {
                if self < 0 { self.checked_neg() } else { ::core::option::Option::Some(self) }
            }
            pub fn wrapping_abs(self) -> $t { if self < 0 { self.wrapping_neg() } else { self } }
        }
        impl ::core::ops::Neg for $t { type Output = $t; fn neg(self) -> $t { -self } }
        impl<'a> ::core::ops::Neg for &'a $t { type Output = $t; fn neg(self) -> $t { -*self } }
    )* }
}

// Lossless conversions: From<$from> for $to.
macro_rules! from_widen {
    ($from:ident => $($to:ident)*) => { $(
        impl ::core::convert::From<$from> for $to { fn from(v: $from) -> $to { v as $to } }
    )* }
}

// Tuples: Clone, PartialEq, Eq, PartialOrd, Ord, Default, Debug.
macro_rules! tuple_impls {
    ($(($T:ident $i:tt))+) => {
        impl<$($T: ::core::clone::Clone),+> ::core::clone::Clone for ($($T,)+) {
            fn clone(&self) -> ($($T,)+) { ($(self.$i.clone(),)+) }
        }
        impl<$($T: ::core::cmp::PartialEq),+> ::core::cmp::PartialEq for ($($T,)+) {
            fn eq(&self, o: &($($T,)+)) -> bool { true $(&& self.$i == o.$i)+ }
        }
        impl<$($T: ::core::cmp::Eq),+> ::core::cmp::Eq for ($($T,)+) {}
        impl<$($T: ::core::cmp::PartialOrd),+> ::core::cmp::PartialOrd for ($($T,)+) {
            fn partial_cmp(&self, o: &($($T,)+)) -> ::core::option::Option<::core::cmp::Ordering> {
                $(
                    match ::core::cmp::PartialOrd::partial_cmp(&self.$i, &o.$i) {
                        ::core::option::Option::Some(::core::cmp::Ordering::Equal) => {}
                        other => return other,
                    }
                )+
                ::core::option::Option::Some(::core::cmp::Ordering::Equal)
            }
        }
        impl<$($T: ::core::cmp::Ord),+> ::core::cmp::Ord for ($($T,)+) {
            fn cmp(&self, o: &($($T,)+)) -> ::core::cmp::Ordering {
                $(
                    match ::core::cmp::Ord::cmp(&self.$i, &o.$i) {
                        ::core::cmp::Ordering::Equal => {}
                        other => return other,
                    }
                )+
                ::core::cmp::Ordering::Equal
            }
        }
        impl<$($T: ::core::default::Default),+> ::core::default::Default for ($($T,)+) {
            fn default() -> ($($T,)+) { ($($T::default(),)+) }
        }
        impl<$($T: ::core::fmt::Debug),+> ::core::fmt::Debug for ($($T,)+) {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                let mut b = f.debug_tuple("");
                $( b.field(&self.$i); )+
                b.finish()
            }
        }
    }
}

// Range indexing for [T], Vec<T>, str and String.
macro_rules! slice_range_index {
    ($R:ty, $r:ident, $start:expr, $end:expr) => {
        impl<T> ::core::ops::Index<$R> for [T] {
            type Output = [T];
            fn index(&self, $r: $R) -> &[T] { ::core::slice::sub_slice(self, $start, $end) }
        }
        impl<T> ::core::ops::IndexMut<$R> for [T] {
            fn index_mut(&mut self, $r: $R) -> &mut [T] { ::core::slice::sub_slice_mut(self, $start, $end) }
        }
        impl<T> ::core::ops::Index<$R> for ::core::vec::Vec<T> {
            type Output = [T];
            fn index(&self, $r: $R) -> &[T] { ::core::slice::sub_slice(self.as_slice(), $start, $end) }
        }
        impl<T> ::core::ops::IndexMut<$R> for ::core::vec::Vec<T> {
            fn index_mut(&mut self, $r: $R) -> &mut [T] { ::core::slice::sub_slice_mut(self.as_mut_slice(), $start, $end) }
        }
        impl ::core::ops::Index<$R> for str {
            type Output = str;
            fn index(&self, $r: $R) -> &str { ::core::str::sub_str(self, $start, $end) }
        }
        impl ::core::ops::Index<$R> for ::core::string::String {
            type Output = str;
            fn index(&self, $r: $R) -> &str { ::core::str::sub_str(self.as_str(), $start, $end) }
        }
    }
}

// ---------------------------------------------------------------- intrinsics

pub mod intrinsics {
    extern "rust-intrinsic" {
        // element storage: "base" is a byte address for T = u8 (packed), else a word address
        pub fn elem_alloc<T>(n: usize) -> usize;
        pub fn elem_realloc<T>(base: usize, old_n: usize, new_n: usize) -> usize;
        pub fn elem_free<T>(base: usize);
        pub fn elem_read<T>(base: usize, i: usize) -> T;
        pub fn elem_write<T>(base: usize, i: usize, v: T);
        pub fn elem_offset<T>(base: usize, n: usize) -> usize;
        pub fn elem_ref<'a, T>(base: usize, i: usize) -> &'a T;
        pub fn elem_ref_mut<'a, T>(base: usize, i: usize) -> &'a mut T;
        // places
        pub fn read_ref<T>(r: &T) -> T;
        pub fn write_ref<T>(r: &mut T, v: T);
        pub fn forget<T>(v: T);
        pub fn size_of<T>() -> usize;         // size of T in bytes as stored in memory (8 per word; packed u8 arrays not counted)
        pub fn transmute<T, U>(v: T) -> U;
        // slices and strings as (base, length) pairs
        pub fn slice_base<T>(s: &[T]) -> usize;
        pub fn slice_len<T>(s: &[T]) -> usize;
        pub fn slice_from_raw<'a, T>(base: usize, len: usize) -> &'a [T];
        pub fn slice_from_raw_mut<'a, T>(base: usize, len: usize) -> &'a mut [T];
        pub fn str_base(s: &str) -> usize;
        pub fn str_len(s: &str) -> usize;
        pub fn str_from_raw<'a>(base: usize, len: usize) -> &'a str;
        // Box<T> (T may be unsized)
        pub fn box_new<T>(v: T) -> ::core::boxed::Box<T>;
        pub fn box_deref<'a, T: ?Sized>(b: &'a ::core::boxed::Box<T>) -> &'a T;
        pub fn box_deref_mut<'a, T: ?Sized>(b: &'a mut ::core::boxed::Box<T>) -> &'a mut T;
        pub fn box_drop<T: ?Sized>(b: &mut ::core::boxed::Box<T>);
        // integers (T is any integer type)
        pub fn wrapping_add<T>(a: T, b: T) -> T;
        pub fn wrapping_sub<T>(a: T, b: T) -> T;
        pub fn wrapping_mul<T>(a: T, b: T) -> T;
        pub fn add_with_overflow<T>(a: T, b: T) -> (T, bool);
        pub fn sub_with_overflow<T>(a: T, b: T) -> (T, bool);
        pub fn mul_with_overflow<T>(a: T, b: T) -> (T, bool);
        // floats
        pub fn f64_to_bits(v: f64) -> u64;
        pub fn f64_from_bits(b: u64) -> f64;
        pub fn sqrtf64(v: f64) -> f64;
        pub fn floorf64(v: f64) -> f64;
        pub fn ceilf64(v: f64) -> f64;
        pub fn fmt_f64(v: f64, prec: usize, has_prec: bool, mode: u8, buf: usize) -> usize;
        pub fn parse_f64(base: usize, len: usize, out: &mut f64) -> bool;
        // I/O and process
        pub fn write_bytes(port: i64, base: usize, len: usize);
        pub fn read_byte(port: i64) -> i32;
        pub fn exit(code: i32) -> !;
        pub fn abort() -> !;
    }
}

// ---------------------------------------------------------------- marker

pub mod marker {
    #[lang = "sized"]
    pub trait Sized {}
    #[lang = "copy"]
    pub trait Copy: ::core::clone::Clone {}
    #[lang = "send"]
    pub trait Send {}
    #[lang = "sync"]
    pub trait Sync {}
    #[lang = "phantom_data"]
    pub struct PhantomData<T: ?Sized>;
    impl<T: ?Sized> ::core::clone::Clone for PhantomData<T> {
        fn clone(&self) -> PhantomData<T> { PhantomData }
    }
    impl<T: ?Sized> ::core::marker::Copy for PhantomData<T> {}
}

// ---------------------------------------------------------------- clone

pub mod clone {
    #[lang = "clone"]
    pub trait Clone: Sized {
        fn clone(&self) -> Self;
        fn clone_from(&mut self, source: &Self) { *self = source.clone(); }
    }
    impl<'a, T: ?Sized> Clone for &'a T {
        fn clone(&self) -> &'a T { *self }
    }
    impl Clone for () {
        fn clone(&self) -> () { () }
    }
    impl<T: Copy, const N: usize> Clone for [T; N] {
        fn clone(&self) -> [T; N] { *self }
    }
}

// ---------------------------------------------------------------- ops

pub mod ops {
    #[lang = "add"] pub trait Add<Rhs = Self> { type Output; fn add(self, rhs: Rhs) -> Self::Output; }
    #[lang = "sub"] pub trait Sub<Rhs = Self> { type Output; fn sub(self, rhs: Rhs) -> Self::Output; }
    #[lang = "mul"] pub trait Mul<Rhs = Self> { type Output; fn mul(self, rhs: Rhs) -> Self::Output; }
    #[lang = "div"] pub trait Div<Rhs = Self> { type Output; fn div(self, rhs: Rhs) -> Self::Output; }
    #[lang = "rem"] pub trait Rem<Rhs = Self> { type Output; fn rem(self, rhs: Rhs) -> Self::Output; }
    #[lang = "neg"] pub trait Neg { type Output; fn neg(self) -> Self::Output; }
    #[lang = "not"] pub trait Not { type Output; fn not(self) -> Self::Output; }
    #[lang = "bitand"] pub trait BitAnd<Rhs = Self> { type Output; fn bitand(self, rhs: Rhs) -> Self::Output; }
    #[lang = "bitor"] pub trait BitOr<Rhs = Self> { type Output; fn bitor(self, rhs: Rhs) -> Self::Output; }
    #[lang = "bitxor"] pub trait BitXor<Rhs = Self> { type Output; fn bitxor(self, rhs: Rhs) -> Self::Output; }
    #[lang = "shl"] pub trait Shl<Rhs = Self> { type Output; fn shl(self, rhs: Rhs) -> Self::Output; }
    #[lang = "shr"] pub trait Shr<Rhs = Self> { type Output; fn shr(self, rhs: Rhs) -> Self::Output; }

    #[lang = "add_assign"] pub trait AddAssign<Rhs = Self> { fn add_assign(&mut self, rhs: Rhs); }
    #[lang = "sub_assign"] pub trait SubAssign<Rhs = Self> { fn sub_assign(&mut self, rhs: Rhs); }
    #[lang = "mul_assign"] pub trait MulAssign<Rhs = Self> { fn mul_assign(&mut self, rhs: Rhs); }
    #[lang = "div_assign"] pub trait DivAssign<Rhs = Self> { fn div_assign(&mut self, rhs: Rhs); }
    #[lang = "rem_assign"] pub trait RemAssign<Rhs = Self> { fn rem_assign(&mut self, rhs: Rhs); }
    #[lang = "bitand_assign"] pub trait BitAndAssign<Rhs = Self> { fn bitand_assign(&mut self, rhs: Rhs); }
    #[lang = "bitor_assign"] pub trait BitOrAssign<Rhs = Self> { fn bitor_assign(&mut self, rhs: Rhs); }
    #[lang = "bitxor_assign"] pub trait BitXorAssign<Rhs = Self> { fn bitxor_assign(&mut self, rhs: Rhs); }
    #[lang = "shl_assign"] pub trait ShlAssign<Rhs = Self> { fn shl_assign(&mut self, rhs: Rhs); }
    #[lang = "shr_assign"] pub trait ShrAssign<Rhs = Self> { fn shr_assign(&mut self, rhs: Rhs); }

    #[lang = "index"]
    pub trait Index<Idx: ?Sized> {
        type Output: ?Sized;
        fn index(&self, index: Idx) -> &Self::Output;
    }
    #[lang = "index_mut"]
    pub trait IndexMut<Idx: ?Sized>: Index<Idx> {
        fn index_mut(&mut self, index: Idx) -> &mut Self::Output;
    }

    #[lang = "deref"]
    pub trait Deref {
        type Target: ?Sized;
        fn deref(&self) -> &Self::Target;
    }
    #[lang = "deref_mut"]
    pub trait DerefMut: Deref {
        fn deref_mut(&mut self) -> &mut Self::Target;
    }
    impl<'a, T: ?Sized> Deref for &'a T {
        type Target = T;
        fn deref(&self) -> &T { *self }
    }
    impl<'a, T: ?Sized> Deref for &'a mut T {
        type Target = T;
        fn deref(&self) -> &T { &**self }
    }
    impl<'a, T: ?Sized> DerefMut for &'a mut T {
        fn deref_mut(&mut self) -> &mut T { &mut **self }
    }

    #[lang = "drop"]
    pub trait Drop {
        fn drop(&mut self);
    }

    // `F: Fn(A, B) -> R` means `F: Fn<(A, B), Output = R>`; calling a closure or
    // fn value `f(a, b)` is `Fn::call(&f, (a, b))` (FnMut::call_mut / FnOnce::call_once).
    #[lang = "fn_once"]
    pub trait FnOnce<Args> {
        type Output;
        fn call_once(self, args: Args) -> Self::Output;
    }
    #[lang = "fn_mut"]
    pub trait FnMut<Args>: FnOnce<Args> {
        fn call_mut(&mut self, args: Args) -> Self::Output;
    }
    #[lang = "fn"]
    pub trait Fn<Args>: FnMut<Args> {
        fn call(&self, args: Args) -> Self::Output;
    }

    // `a..b`, `a..=b`, `a..`, `..b`, `..`, `..=b` are these structs.
    #[lang = "range"]
    pub struct Range<Idx> { pub start: Idx, pub end: Idx }
    #[lang = "range_inclusive"]
    pub struct RangeInclusive<Idx> { pub start: Idx, pub end: Idx, pub exhausted: bool }
    #[lang = "range_from"]
    pub struct RangeFrom<Idx> { pub start: Idx }
    #[lang = "range_to"]
    pub struct RangeTo<Idx> { pub end: Idx }
    #[lang = "range_to_inclusive"]
    pub struct RangeToInclusive<Idx> { pub end: Idx }
    #[lang = "range_full"]
    pub struct RangeFull;

    impl<Idx> RangeInclusive<Idx> {
        // `a..=b` is lowered to RangeInclusive::new(a, b)
        #[lang = "range_inclusive_new"]
        pub fn new(start: Idx, end: Idx) -> RangeInclusive<Idx> {
            RangeInclusive { start: start, end: end, exhausted: false }
        }
        pub fn start(&self) -> &Idx { &self.start }
        pub fn end(&self) -> &Idx { &self.end }
    }
    impl<Idx: ::core::cmp::PartialOrd> Range<Idx> {
        pub fn contains(&self, x: &Idx) -> bool { *x >= self.start && *x < self.end }
        pub fn is_empty(&self) -> bool { !(self.start < self.end) }
    }
    impl<Idx: ::core::cmp::PartialOrd> RangeInclusive<Idx> {
        pub fn contains(&self, x: &Idx) -> bool { *x >= self.start && *x <= self.end }
    }
    impl<Idx: ::core::cmp::PartialOrd> RangeFrom<Idx> {
        pub fn contains(&self, x: &Idx) -> bool { *x >= self.start }
    }
    impl<Idx: ::core::cmp::PartialOrd> RangeTo<Idx> {
        pub fn contains(&self, x: &Idx) -> bool { *x < self.end }
    }
    impl<Idx: ::core::clone::Clone> ::core::clone::Clone for Range<Idx> {
        fn clone(&self) -> Range<Idx> { Range { start: self.start.clone(), end: self.end.clone() } }
    }
    impl<Idx: ::core::clone::Clone> ::core::clone::Clone for RangeInclusive<Idx> {
        fn clone(&self) -> RangeInclusive<Idx> {
            RangeInclusive { start: self.start.clone(), end: self.end.clone(), exhausted: self.exhausted }
        }
    }
    impl<Idx: ::core::fmt::Debug> ::core::fmt::Debug for Range<Idx> {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
            ::core::fmt::Debug::fmt(&self.start, f)?;
            f.write_str("..")?;
            ::core::fmt::Debug::fmt(&self.end, f)
        }
    }
    impl<Idx: ::core::fmt::Debug> ::core::fmt::Debug for RangeInclusive<Idx> {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
            ::core::fmt::Debug::fmt(&self.start, f)?;
            f.write_str("..=")?;
            ::core::fmt::Debug::fmt(&self.end, f)
        }
    }
    impl<Idx: ::core::cmp::PartialEq> ::core::cmp::PartialEq for Range<Idx> {
        fn eq(&self, o: &Range<Idx>) -> bool { self.start == o.start && self.end == o.end }
    }

    // (start, end) of a usize range; end == usize::MAX means "to the end" (used by drain and friends)
    pub trait RangeArg { fn bounds(&self) -> (usize, usize); }
    impl RangeArg for Range<usize> { fn bounds(&self) -> (usize, usize) { (self.start, self.end) } }
    impl RangeArg for RangeInclusive<usize> { fn bounds(&self) -> (usize, usize) { (self.start, self.end + 1) } }
    impl RangeArg for RangeFrom<usize> { fn bounds(&self) -> (usize, usize) { (self.start, usize::MAX) } }
    impl RangeArg for RangeTo<usize> { fn bounds(&self) -> (usize, usize) { (0, self.end) } }
    impl RangeArg for RangeToInclusive<usize> { fn bounds(&self) -> (usize, usize) { (0, self.end + 1) } }
    impl RangeArg for RangeFull { fn bounds(&self) -> (usize, usize) { (0, usize::MAX) } }
}

// Instantiations live in a private module so that the primitive names (i8 .. f64,
// char, str) are never shadowed by the modules of the same names at the crate root.
mod prim_impls {
    arith_all!(i8 i16 i32 i64 isize u8 u16 u32 u64 usize f64);
    bit_all!(i8 i16 i32 i64 isize u8 u16 u32 u64 usize bool);
    shift_all!(i8 i16 i32 i64 isize u8 u16 u32 u64 usize);
    clone_default! {
        i8, 0; i16, 0; i32, 0; i64, 0; isize, 0;
        u8, 0; u16, 0; u32, 0; u64, 0; usize, 0;
        f64, 0.0; bool, false; char, '\0';
    }
    cmp_total!(i8 i16 i32 i64 isize u8 u16 u32 u64 usize char bool);
    int_signed!(i8 i16 i32 i64 isize);
    int_impls!(i8, u8, -128, 127, 8);
    int_impls!(i16, u16, -32768, 32767, 16);
    int_impls!(i32, u32, -2147483648, 2147483647, 32);
    int_impls!(i64, u64, -9223372036854775807 - 1, 9223372036854775807, 64);
    int_impls!(isize, usize, -9223372036854775807 - 1, 9223372036854775807, 64);
    int_impls!(u8, u8, 0, 255, 8);
    int_impls!(u16, u16, 0, 65535, 16);
    int_impls!(u32, u32, 0, 4294967295, 32);
    int_impls!(u64, u64, 0, 18446744073709551615, 64);
    int_impls!(usize, usize, 0, 18446744073709551615, 64);
    impl u8 {
        pub fn is_ascii(&self) -> bool { *self < 128 }
        pub fn is_ascii_digit(&self) -> bool { *self >= b'0' && *self <= b'9' }
        pub fn is_ascii_uppercase(&self) -> bool { *self >= b'A' && *self <= b'Z' }
        pub fn is_ascii_lowercase(&self) -> bool { *self >= b'a' && *self <= b'z' }
        pub fn is_ascii_alphabetic(&self) -> bool { self.is_ascii_uppercase() || self.is_ascii_lowercase() }
        pub fn is_ascii_alphanumeric(&self) -> bool { self.is_ascii_alphabetic() || self.is_ascii_digit() }
        pub fn is_ascii_hexdigit(&self) -> bool {
            self.is_ascii_digit() || (*self >= b'a' && *self <= b'f') || (*self >= b'A' && *self <= b'F')
        }
        pub fn is_ascii_whitespace(&self) -> bool {
            *self == b' ' || *self == b'\t' || *self == b'\n' || *self == b'\r' || *self == 12u8
        }
        pub fn is_ascii_punctuation(&self) -> bool {
            let u = *self;
            (u >= 33 && u <= 47) || (u >= 58 && u <= 64) || (u >= 91 && u <= 96) || (u >= 123 && u <= 126)
        }
        pub fn is_ascii_graphic(&self) -> bool { *self >= 33 && *self <= 126 }
        pub fn is_ascii_control(&self) -> bool { *self < 32 || *self == 127 }
        pub fn to_ascii_uppercase(&self) -> u8 { if self.is_ascii_lowercase() { *self - 32 } else { *self } }
        pub fn to_ascii_lowercase(&self) -> u8 { if self.is_ascii_uppercase() { *self + 32 } else { *self } }
        pub fn make_ascii_uppercase(&mut self) { *self = self.to_ascii_uppercase(); }
        pub fn make_ascii_lowercase(&mut self) { *self = self.to_ascii_lowercase(); }
        pub fn eq_ignore_ascii_case(&self, o: &u8) -> bool { self.to_ascii_lowercase() == o.to_ascii_lowercase() }
    }
    from_widen!(u8 => u16 u32 u64 usize i16 i32 i64 isize f64);
    from_widen!(u16 => u32 u64 usize i32 i64 f64);
    from_widen!(u32 => u64 i64 f64);
    from_widen!(i8 => i16 i32 i64 isize f64);
    from_widen!(i16 => i32 i64 isize f64);
    from_widen!(i32 => i64 f64);
    from_widen!(bool => u8 u16 u32 u64 usize i8 i16 i32 i64 isize);
    tuple_impls!((A 0));
    tuple_impls!((A 0) (B 1));
    tuple_impls!((A 0) (B 1) (C 2));
    tuple_impls!((A 0) (B 1) (C 2) (D 3));
    slice_range_index!(::core::ops::Range<usize>, r, r.start, r.end);
    slice_range_index!(::core::ops::RangeInclusive<usize>, r, r.start, r.end + 1);
    slice_range_index!(::core::ops::RangeFrom<usize>, r, r.start, usize::MAX);
    slice_range_index!(::core::ops::RangeTo<usize>, r, 0, r.end);
    slice_range_index!(::core::ops::RangeToInclusive<usize>, r, 0, r.end + 1);
    slice_range_index!(::core::ops::RangeFull, r, 0, usize::MAX);
}

// ---------------------------------------------------------------- cmp

pub mod cmp {
    use ::core::option::Option::{self, Some, None};

    #[lang = "eq"]
    pub trait PartialEq<Rhs: ?Sized = Self> {
        fn eq(&self, other: &Rhs) -> bool;
        fn ne(&self, other: &Rhs) -> bool { !self.eq(other) }
    }
    #[lang = "eq_total"]
    pub trait Eq: PartialEq<Self> {}

    #[lang = "ordering"]
    pub enum Ordering { Less = -1, Equal = 0, Greater = 1 }

    #[lang = "partial_ord"]
    pub trait PartialOrd<Rhs: ?Sized = Self>: PartialEq<Rhs> {
        fn partial_cmp(&self, other: &Rhs) -> Option<Ordering>;
        fn lt(&self, other: &Rhs) -> bool {
            match self.partial_cmp(other) { Some(Ordering::Less) => true, _ => false }
        }
        fn le(&self, other: &Rhs) -> bool {
            match self.partial_cmp(other) { Some(Ordering::Less) => true, Some(Ordering::Equal) => true, _ => false }
        }
        fn gt(&self, other: &Rhs) -> bool {
            match self.partial_cmp(other) { Some(Ordering::Greater) => true, _ => false }
        }
        fn ge(&self, other: &Rhs) -> bool {
            match self.partial_cmp(other) { Some(Ordering::Greater) => true, Some(Ordering::Equal) => true, _ => false }
        }
    }

    #[lang = "ord"]
    pub trait Ord: Eq + PartialOrd<Self> {
        fn cmp(&self, other: &Self) -> Ordering;
        fn max(self, other: Self) -> Self where Self: Sized {
            match self.cmp(&other) { Ordering::Greater => self, _ => other }
        }
        fn min(self, other: Self) -> Self where Self: Sized {
            match self.cmp(&other) { Ordering::Greater => other, _ => self }
        }
        fn clamp(self, lo: Self, hi: Self) -> Self where Self: Sized {
            if self.cmp(&lo) == Ordering::Less { lo }
            else if self.cmp(&hi) == Ordering::Greater { hi }
            else { self }
        }
    }

    pub fn max<T: Ord>(a: T, b: T) -> T { a.max(b) }
    pub fn min<T: Ord>(a: T, b: T) -> T { a.min(b) }
    pub fn max_by_key<T, K: Ord, F: FnMut(&T) -> K>(a: T, b: T, mut f: F) -> T {
        if f(&b) >= f(&a) { b } else { a }
    }

    impl Ordering {
        pub fn reverse(self) -> Ordering {
            match self { Ordering::Less => Ordering::Greater, Ordering::Equal => Ordering::Equal, Ordering::Greater => Ordering::Less }
        }
        pub fn then(self, other: Ordering) -> Ordering {
            match self { Ordering::Equal => other, _ => self }
        }
        pub fn then_with<F: FnOnce() -> Ordering>(self, f: F) -> Ordering {
            match self { Ordering::Equal => f(), _ => self }
        }
        pub fn is_eq(self) -> bool { self == Ordering::Equal }
        pub fn is_ne(self) -> bool { self != Ordering::Equal }
        pub fn is_lt(self) -> bool { self == Ordering::Less }
        pub fn is_gt(self) -> bool { self == Ordering::Greater }
        pub fn is_le(self) -> bool { self != Ordering::Greater }
        pub fn is_ge(self) -> bool { self != Ordering::Less }
    }
    impl ::core::clone::Clone for Ordering { fn clone(&self) -> Ordering { *self } }
    impl ::core::marker::Copy for Ordering {}
    impl PartialEq for Ordering { fn eq(&self, o: &Ordering) -> bool { *self as i32 == *o as i32 } }
    impl Eq for Ordering {}
    impl PartialOrd for Ordering {
        fn partial_cmp(&self, o: &Ordering) -> Option<Ordering> { Some(Ord::cmp(self, o)) }
    }
    impl Ord for Ordering {
        fn cmp(&self, o: &Ordering) -> Ordering { (*self as i32).cmp(&(*o as i32)) }
    }
    impl ::core::fmt::Debug for Ordering {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
            f.write_str(match *self { Ordering::Less => "Less", Ordering::Equal => "Equal", Ordering::Greater => "Greater" })
        }
    }

    pub struct Reverse<T>(pub T);
    impl<T: PartialEq> PartialEq for Reverse<T> { fn eq(&self, o: &Reverse<T>) -> bool { self.0 == o.0 } }
    impl<T: Eq> Eq for Reverse<T> {}
    impl<T: PartialOrd> PartialOrd for Reverse<T> {
        fn partial_cmp(&self, o: &Reverse<T>) -> Option<Ordering> { o.0.partial_cmp(&self.0) }
    }
    impl<T: Ord> Ord for Reverse<T> { fn cmp(&self, o: &Reverse<T>) -> Ordering { o.0.cmp(&self.0) } }
    impl<T: ::core::clone::Clone> ::core::clone::Clone for Reverse<T> {
        fn clone(&self) -> Reverse<T> { Reverse(self.0.clone()) }
    }
    impl<T: ::core::fmt::Debug> ::core::fmt::Debug for Reverse<T> {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
            f.debug_tuple("Reverse").field(&self.0).finish()
        }
    }
    impl<T: ::core::marker::Copy> ::core::marker::Copy for Reverse<T> {}

    // f64: partial order only
    impl PartialEq for f64 {
        fn eq(&self, o: &f64) -> bool { *self == *o }
        fn ne(&self, o: &f64) -> bool { *self != *o }
    }
    impl PartialOrd for f64 {
        fn partial_cmp(&self, o: &f64) -> Option<Ordering> {
            if *self < *o { Some(Ordering::Less) }
            else if *self > *o { Some(Ordering::Greater) }
            else if *self == *o { Some(Ordering::Equal) }
            else { None }
        }
        fn lt(&self, o: &f64) -> bool { *self < *o }
        fn le(&self, o: &f64) -> bool { *self <= *o }
        fn gt(&self, o: &f64) -> bool { *self > *o }
        fn ge(&self, o: &f64) -> bool { *self >= *o }
    }

    impl PartialEq for () { fn eq(&self, o: &()) -> bool { true } }
    impl Eq for () {}
    impl PartialOrd for () { fn partial_cmp(&self, o: &()) -> Option<Ordering> { Some(Ordering::Equal) } }
    impl Ord for () { fn cmp(&self, o: &()) -> Ordering { Ordering::Equal } }

    impl<'a, 'b, A: ?Sized + PartialEq<B>, B: ?Sized> PartialEq<&'b B> for &'a A {
        fn eq(&self, o: &&'b B) -> bool { PartialEq::eq(*self, *o) }
        fn ne(&self, o: &&'b B) -> bool { PartialEq::ne(*self, *o) }
    }
    impl<'a, 'b, A: ?Sized + PartialEq<B>, B: ?Sized> PartialEq<&'b mut B> for &'a mut A {
        fn eq(&self, o: &&'b mut B) -> bool { PartialEq::eq(&**self, &**o) }
    }
    impl<'a, A: ?Sized + Eq> Eq for &'a A {}
    impl<'a, 'b, A: ?Sized + PartialOrd<B>, B: ?Sized> PartialOrd<&'b B> for &'a A {
        fn partial_cmp(&self, o: &&'b B) -> Option<Ordering> { PartialOrd::partial_cmp(*self, *o) }
    }
    impl<'a, A: ?Sized + Ord> Ord for &'a A {
        fn cmp(&self, o: &&'a A) -> Ordering { Ord::cmp(*self, *o) }
    }
}

// ---------------------------------------------------------------- mem, process

pub mod mem {
    pub fn replace<T>(dest: &mut T, src: T) -> T {
        let old = ::core::intrinsics::read_ref::<T>(dest);
        ::core::intrinsics::write_ref::<T>(dest, src);
        old
    }
    pub fn swap<T>(a: &mut T, b: &mut T) {
        let t = ::core::intrinsics::read_ref::<T>(a);
        let u = ::core::intrinsics::read_ref::<T>(b);
        ::core::intrinsics::write_ref::<T>(a, u);
        ::core::intrinsics::write_ref::<T>(b, t);
    }
    pub fn take<T: ::core::default::Default>(dest: &mut T) -> T { replace(dest, T::default()) }
    pub fn forget<T>(v: T) { ::core::intrinsics::forget::<T>(v) }
    pub fn drop<T>(_x: T) {}
    pub fn size_of<T>() -> usize { ::core::intrinsics::size_of::<T>() }
}

pub mod process {
    pub fn exit(code: i32) -> ! { ::core::intrinsics::exit(code) }
    pub fn abort() -> ! { ::core::intrinsics::abort() }
}

// ---------------------------------------------------------------- default, convert, borrow

pub mod default {
    #[lang = "default"]
    pub trait Default: Sized {
        fn default() -> Self;
    }
    impl Default for () { fn default() -> () { () } }
    impl<'a> Default for &'a str { fn default() -> &'a str { "" } }
}

pub mod convert {
    #[lang = "from"]
    pub trait From<T>: Sized {
        fn from(value: T) -> Self;
    }
    #[lang = "into"]
    pub trait Into<T>: Sized {
        fn into(self) -> T;
    }
    impl<T, U: From<T>> Into<U> for T {
        fn into(self) -> U { U::from(self) }
    }
    impl<T> From<T> for T {
        fn from(t: T) -> T { t }
    }
    pub trait AsRef<T: ?Sized> { fn as_ref(&self) -> &T; }
    pub trait AsMut<T: ?Sized> { fn as_mut(&mut self) -> &mut T; }
    impl<'a, T: ?Sized + AsRef<U>, U: ?Sized> AsRef<U> for &'a T {
        fn as_ref(&self) -> &U { AsRef::as_ref(*self) }
    }
    impl AsRef<str> for str { fn as_ref(&self) -> &str { self } }
    impl<T> AsRef<[T]> for [T] { fn as_ref(&self) -> &[T] { self } }
}

pub mod borrow {
    pub trait ToOwned {
        type Owned;
        fn to_owned(&self) -> Self::Owned;
    }
    impl<T: ::core::clone::Clone> ToOwned for T {
        type Owned = T;
        fn to_owned(&self) -> T { self.clone() }
    }
}

// ---------------------------------------------------------------- option

pub mod option {
    use ::core::cmp::Ordering;
    use ::core::option::Option::{None, Some};

    // `e?` on an Option<T> in a fn returning Option<U> lowers to
    //   match e { Some(v) => v, None => return None }
    #[lang = "option"]
    pub enum Option<T> {
        None,
        Some(T),
    }

    impl<T> Option<T> {
        pub fn is_some(&self) -> bool { match *self { Some(_) => true, None => false } }
        pub fn is_none(&self) -> bool { match *self { Some(_) => false, None => true } }
        pub fn is_some_and<F: FnOnce(T) -> bool>(self, f: F) -> bool { match self { Some(x) => f(x), None => false } }
        pub fn as_ref(&self) -> Option<&T> { match *self { Some(ref x) => Some(x), None => None } }
        pub fn as_mut(&mut self) -> Option<&mut T> { match *self { Some(ref mut x) => Some(x), None => None } }
        pub fn unwrap(self) -> T {
            match self {
                Some(x) => x,
                None => ::core::panicking::panic_str("library/core/src/option.rs", 1, 1, "called `Option::unwrap()` on a `None` value"),
            }
        }
        pub fn expect(self, msg: &str) -> T {
            match self {
                Some(x) => x,
                None => ::core::panicking::panic_str("library/core/src/option.rs", 1, 1, msg),
            }
        }
        pub fn unwrap_or(self, default: T) -> T { match self { Some(x) => x, None => default } }
        pub fn unwrap_or_else<F: FnOnce() -> T>(self, f: F) -> T { match self { Some(x) => x, None => f() } }
        pub fn unwrap_or_default(self) -> T where T: ::core::default::Default {
            match self { Some(x) => x, None => T::default() }
        }
        pub fn map<U, F: FnOnce(T) -> U>(self, f: F) -> Option<U> { match self { Some(x) => Some(f(x)), None => None } }
        pub fn map_or<U, F: FnOnce(T) -> U>(self, default: U, f: F) -> U { match self { Some(x) => f(x), None => default } }
        pub fn map_or_else<U, D: FnOnce() -> U, F: FnOnce(T) -> U>(self, d: D, f: F) -> U {
            match self { Some(x) => f(x), None => d() }
        }
        pub fn and_then<U, F: FnOnce(T) -> Option<U>>(self, f: F) -> Option<U> { match self { Some(x) => f(x), None => None } }
        pub fn and<U>(self, o: Option<U>) -> Option<U> { match self { Some(_) => o, None => None } }
        pub fn or(self, o: Option<T>) -> Option<T> { match self { Some(x) => Some(x), None => o } }
        pub fn or_else<F: FnOnce() -> Option<T>>(self, f: F) -> Option<T> { match self { Some(x) => Some(x), None => f() } }
        pub fn ok_or<E>(self, err: E) -> ::core::result::Result<T, E> {
            match self { Some(x) => ::core::result::Result::Ok(x), None => ::core::result::Result::Err(err) }
        }
        pub fn ok_or_else<E, F: FnOnce() -> E>(self, f: F) -> ::core::result::Result<T, E> {
            match self { Some(x) => ::core::result::Result::Ok(x), None => ::core::result::Result::Err(f()) }
        }
        pub fn filter<P: FnOnce(&T) -> bool>(self, p: P) -> Option<T> {
            match self { Some(x) => if p(&x) { Some(x) } else { None }, None => None }
        }
        pub fn take(&mut self) -> Option<T> { ::core::mem::replace(self, None) }
        pub fn replace(&mut self, v: T) -> Option<T> { ::core::mem::replace(self, Some(v)) }
        pub fn zip<U>(self, o: Option<U>) -> Option<(T, U)> {
            match self { Some(a) => match o { Some(b) => Some((a, b)), None => None }, None => None }
        }
        pub fn get_or_insert_with<F: FnOnce() -> T>(&mut self, f: F) -> &mut T {
            if self.is_none() { *self = Some(f()); }
            match *self { Some(ref mut x) => x, None => ::core::panicking::panic_str("library/core/src/option.rs", 1, 1, "unreachable") }
        }
        pub fn iter(&self) -> ::core::vec::IntoIter<&T> {
            let mut v = ::core::vec::Vec::new();
            if let Some(ref x) = *self { v.push(x); }
            v.into_iter()
        }
    }
    impl<'a, T: ::core::clone::Clone> Option<&'a T> {
        pub fn cloned(self) -> Option<T> { match self { Some(x) => Some(x.clone()), None => None } }
    }
    impl<'a, T: ::core::marker::Copy> Option<&'a T> {
        pub fn copied(self) -> Option<T> { match self { Some(x) => Some(*x), None => None } }
    }
    impl<T: ::core::clone::Clone> ::core::clone::Clone for Option<T> {
        fn clone(&self) -> Option<T> { match *self { Some(ref x) => Some(x.clone()), None => None } }
    }
    impl<T: ::core::marker::Copy> ::core::marker::Copy for Option<T> {}
    impl<T> ::core::default::Default for Option<T> { fn default() -> Option<T> { None } }
    impl<T> ::core::convert::From<T> for Option<T> { fn from(v: T) -> Option<T> { Some(v) } }
    impl<T: ::core::cmp::PartialEq> ::core::cmp::PartialEq for Option<T> {
        fn eq(&self, o: &Option<T>) -> bool {
            match *self {
                Some(ref a) => match *o { Some(ref b) => *a == *b, None => false },
                None => match *o { Some(_) => false, None => true },
            }
        }
    }
    impl<T: ::core::cmp::Eq> ::core::cmp::Eq for Option<T> {}
    impl<T: ::core::cmp::PartialOrd> ::core::cmp::PartialOrd for Option<T> {
        fn partial_cmp(&self, o: &Option<T>) -> Option<Ordering> {
            match *self {
                None => match *o { None => Some(Ordering::Equal), Some(_) => Some(Ordering::Less) },
                Some(ref a) => match *o { None => Some(Ordering::Greater), Some(ref b) => a.partial_cmp(b) },
            }
        }
    }
    impl<T: ::core::cmp::Ord> ::core::cmp::Ord for Option<T> {
        fn cmp(&self, o: &Option<T>) -> Ordering {
            match *self {
                None => match *o { None => Ordering::Equal, Some(_) => Ordering::Less },
                Some(ref a) => match *o { None => Ordering::Greater, Some(ref b) => a.cmp(b) },
            }
        }
    }
    impl<T: ::core::fmt::Debug> ::core::fmt::Debug for Option<T> {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
            match *self {
                Some(ref x) => f.debug_tuple("Some").field(x).finish(),
                None => f.write_str("None"),
            }
        }
    }
}

// ---------------------------------------------------------------- result

pub mod result {
    use ::core::result::Result::{Err, Ok};
    use ::core::option::Option::{None, Some};

    // `e?` on a Result<T, E> in a fn returning Result<U, F> lowers to
    //   match e { Ok(v) => v, Err(err) => return Err(From::from(err)) }
    // with From::from the lang item "from" (the identity impl covers E == F).
    #[lang = "result"]
    pub enum Result<T, E> {
        Ok(T),
        Err(E),
    }

    impl<T, E> Result<T, E> {
        pub fn is_ok(&self) -> bool { match *self { Ok(_) => true, Err(_) => false } }
        pub fn is_err(&self) -> bool { match *self { Ok(_) => false, Err(_) => true } }
        pub fn ok(self) -> ::core::option::Option<T> { match self { Ok(x) => Some(x), Err(_) => None } }
        pub fn err(self) -> ::core::option::Option<E> { match self { Ok(_) => None, Err(e) => Some(e) } }
        pub fn as_ref(&self) -> Result<&T, &E> { match *self { Ok(ref x) => Ok(x), Err(ref e) => Err(e) } }
        pub fn as_mut(&mut self) -> Result<&mut T, &mut E> { match *self { Ok(ref mut x) => Ok(x), Err(ref mut e) => Err(e) } }
        pub fn map<U, F: FnOnce(T) -> U>(self, f: F) -> Result<U, E> { match self { Ok(x) => Ok(f(x)), Err(e) => Err(e) } }
        pub fn map_err<G, F: FnOnce(E) -> G>(self, f: F) -> Result<T, G> { match self { Ok(x) => Ok(x), Err(e) => Err(f(e)) } }
        pub fn and_then<U, F: FnOnce(T) -> Result<U, E>>(self, f: F) -> Result<U, E> { match self { Ok(x) => f(x), Err(e) => Err(e) } }
        pub fn or_else<G, F: FnOnce(E) -> Result<T, G>>(self, f: F) -> Result<T, G> { match self { Ok(x) => Ok(x), Err(e) => f(e) } }
        pub fn unwrap_or(self, default: T) -> T { match self { Ok(x) => x, Err(_) => default } }
        pub fn unwrap_or_else<F: FnOnce(E) -> T>(self, f: F) -> T { match self { Ok(x) => x, Err(e) => f(e) } }
        pub fn unwrap_or_default(self) -> T where T: ::core::default::Default {
            match self { Ok(x) => x, Err(_) => T::default() }
        }
    }
    impl<T, E: ::core::fmt::Debug> Result<T, E> {
        pub fn unwrap(self) -> T {
            match self {
                Ok(x) => x,
                Err(e) => ::core::panicking::unwrap_failed("called `Result::unwrap()` on an `Err` value", &e),
            }
        }
        pub fn expect(self, msg: &str) -> T {
            match self { Ok(x) => x, Err(e) => ::core::panicking::unwrap_failed(msg, &e) }
        }
    }
    impl<T: ::core::fmt::Debug, E> Result<T, E> {
        pub fn unwrap_err(self) -> E {
            match self {
                Ok(x) => ::core::panicking::unwrap_failed("called `Result::unwrap_err()` on an `Ok` value", &x),
                Err(e) => e,
            }
        }
        pub fn expect_err(self, msg: &str) -> E {
            match self { Ok(x) => ::core::panicking::unwrap_failed(msg, &x), Err(e) => e }
        }
    }
    impl<T: ::core::clone::Clone, E: ::core::clone::Clone> ::core::clone::Clone for Result<T, E> {
        fn clone(&self) -> Result<T, E> {
            match *self { Ok(ref x) => Ok(x.clone()), Err(ref e) => Err(e.clone()) }
        }
    }
    impl<T: ::core::marker::Copy, E: ::core::marker::Copy> ::core::marker::Copy for Result<T, E> {}
    impl<T: ::core::cmp::PartialEq, E: ::core::cmp::PartialEq> ::core::cmp::PartialEq for Result<T, E> {
        fn eq(&self, o: &Result<T, E>) -> bool {
            match *self {
                Ok(ref a) => match *o { Ok(ref b) => *a == *b, Err(_) => false },
                Err(ref a) => match *o { Err(ref b) => *a == *b, Ok(_) => false },
            }
        }
    }
    impl<T: ::core::cmp::Eq, E: ::core::cmp::Eq> ::core::cmp::Eq for Result<T, E> {}
    impl<T: ::core::fmt::Debug, E: ::core::fmt::Debug> ::core::fmt::Debug for Result<T, E> {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
            match *self {
                Ok(ref x) => f.debug_tuple("Ok").field(x).finish(),
                Err(ref e) => f.debug_tuple("Err").field(e).finish(),
            }
        }
    }
}

// ---------------------------------------------------------------- fmt

pub mod fmt {
    use ::core::option::Option::{self, Some, None};
    use ::core::result::Result::{Ok, Err};
    use ::core::string::String;

    pub struct Error;
    impl ::core::clone::Clone for Error { fn clone(&self) -> Error { Error } }
    impl ::core::marker::Copy for Error {}
    impl Debug for Error {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result { f.write_str("Error") }
    }
    impl Display for Error {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result { f.write_str("an error occurred when formatting an argument") }
    }
    pub type Result = ::core::result::Result<(), Error>;
    // `Ok(())` of type fmt::Result (what write!/writeln! should end with, so the error type is known)
    pub fn ok() -> Result { ::core::result::Result::Ok(()) }

    pub trait Write {
        fn write_str(&mut self, s: &str) -> Result;
        fn write_char(&mut self, c: char) -> Result {
            let mut b = [0u8; 4];
            let n = ::core::char::encode_utf8(c, &mut b);
            self.write_str(::core::str::buf_str(&b, n))
        }
    }
    impl<'a, W: Write + ?Sized> Write for &'a mut W {
        fn write_str(&mut self, s: &str) -> Result { (**self).write_str(s) }
    }
    // The destination of write!/writeln!: `dst.__write_target()` is a method call, so
    // the receiver is found as for std's `dst.write_fmt(..)` (a String, a &mut String,
    // a Formatter, a stdout handle through Deref), never a deref to `str`.
    pub trait WriteTarget { fn __write_target(&mut self) -> &mut dyn Write; }
    impl<W: Write> WriteTarget for W {
        fn __write_target(&mut self) -> &mut dyn Write { self }
    }

    pub trait Display { fn fmt(&self, f: &mut Formatter<'_>) -> Result; }
    pub trait Debug { fn fmt(&self, f: &mut Formatter<'_>) -> Result; }
    pub trait LowerHex { fn fmt(&self, f: &mut Formatter<'_>) -> Result; }
    pub trait UpperHex { fn fmt(&self, f: &mut Formatter<'_>) -> Result; }
    pub trait Octal { fn fmt(&self, f: &mut Formatter<'_>) -> Result; }
    pub trait Binary { fn fmt(&self, f: &mut Formatter<'_>) -> Result; }
    pub trait LowerExp { fn fmt(&self, f: &mut Formatter<'_>) -> Result; }
    pub trait UpperExp { fn fmt(&self, f: &mut Formatter<'_>) -> Result; }

    #[lang = "formatter"]
    pub struct Formatter<'a> {
        out: &'a mut dyn Write,
        fill: char,
        align: u8,
        sign: u8,
        alt: bool,
        zero: bool,
        width: usize,
        has_width: bool,
        prec: usize,
        has_prec: bool,
    }

    impl<'a> Formatter<'a> {
        pub fn new(out: &'a mut dyn Write) -> Formatter<'a> {
            Formatter {
                out: out, fill: ' ', align: 0, sign: 0, alt: false, zero: false,
                width: 0, has_width: false, prec: 0, has_prec: false,
            }
        }
        // a formatter with the same spec writing to another sink
        fn with_out<'b>(&self, out: &'b mut dyn Write) -> Formatter<'b> {
            Formatter {
                out: out, fill: self.fill, align: self.align, sign: self.sign, alt: self.alt, zero: self.zero,
                width: self.width, has_width: self.has_width, prec: self.prec, has_prec: self.has_prec,
            }
        }
        pub fn write_str(&mut self, s: &str) -> Result { self.out.write_str(s) }
        pub fn write_char(&mut self, c: char) -> Result { self.out.write_char(c) }
        pub fn set_spec(&mut self, fill: char, align: u8, sign: u8, alt: bool, zero: bool,
                        width: usize, has_width: bool, prec: usize, has_prec: bool) {
            self.fill = fill; self.align = align; self.sign = sign; self.alt = alt; self.zero = zero;
            self.width = width; self.has_width = has_width; self.prec = prec; self.has_prec = has_prec;
        }
        pub fn clear_spec(&mut self) {
            self.fill = ' '; self.align = 0; self.sign = 0; self.alt = false; self.zero = false;
            self.width = 0; self.has_width = false; self.prec = 0; self.has_prec = false;
        }
        pub fn alternate(&self) -> bool { self.alt }
        pub fn sign_plus(&self) -> bool { self.sign == 1 }
        pub fn width(&self) -> Option<usize> { if self.has_width { Some(self.width) } else { None } }
        pub fn precision(&self) -> Option<usize> { if self.has_prec { Some(self.prec) } else { None } }
        pub fn fill(&self) -> char { self.fill }

        fn pad_chars(&mut self, n: usize, c: char) -> Result {
            let mut i = 0;
            while i < n { self.out.write_char(c)?; i += 1; }
            Ok(())
        }

        // applies width, alignment (default left) and precision (max chars) to a string
        pub fn pad(&mut self, s: &str) -> Result {
            let mut s = s;
            if self.has_prec {
                let mut it = s.chars();
                let mut bytes = 0usize;
                let mut k = 0usize;
                while k < self.prec {
                    match it.next() {
                        Some(c) => { bytes += c.len_utf8(); k += 1; }
                        None => break,
                    }
                }
                s = ::core::str::sub_str(s, 0, bytes);
            }
            if !self.has_width { return self.out.write_str(s); }
            let n = s.chars().count();
            if n >= self.width { return self.out.write_str(s); }
            let padn = self.width - n;
            let fill = self.fill;
            let (pre, post) = if self.align == 3 { (padn, 0) }
                              else if self.align == 2 { (padn / 2, padn - padn / 2) }
                              else { (0, padn) };
            self.pad_chars(pre, fill)?;
            self.out.write_str(s)?;
            self.pad_chars(post, fill)
        }

        // sign, optional radix prefix (with `#`), zero or space padding (default right)
        pub fn pad_integral(&mut self, nonneg: bool, prefix: &str, digits: &str) -> Result {
            let sign: Option<char> = if !nonneg { Some('-') } else if self.sign == 1 { Some('+') } else { None };
            let pre: &str = if self.alt { prefix } else { "" };
            let mut len = digits.len() + pre.len();
            if sign.is_some() { len += 1; }
            if !self.has_width || self.width <= len {
                if let Some(c) = sign { self.out.write_char(c)?; }
                self.out.write_str(pre)?;
                return self.out.write_str(digits);
            }
            let padn = self.width - len;
            let fill = self.fill;
            if self.zero {
                if let Some(c) = sign { self.out.write_char(c)?; }
                self.out.write_str(pre)?;
                self.pad_chars(padn, '0')?;
                return self.out.write_str(digits);
            }
            let (before, after) = if self.align == 1 { (0, padn) }
                                  else if self.align == 2 { (padn / 2, padn - padn / 2) }
                                  else { (padn, 0) };
            self.pad_chars(before, fill)?;
            if let Some(c) = sign { self.out.write_char(c)?; }
            self.out.write_str(pre)?;
            self.out.write_str(digits)?;
            self.pad_chars(after, fill)
        }

        pub fn debug_struct<'b>(&'b mut self, name: &str) -> DebugStruct<'b, 'a> {
            let r = self.write_str(name);
            DebugStruct { fmt: self, result: r, has_fields: false }
        }
        pub fn debug_tuple<'b>(&'b mut self, name: &str) -> DebugTuple<'b, 'a> {
            let r = self.write_str(name);
            DebugTuple { fmt: self, result: r, fields: 0, empty_name: name.is_empty() }
        }
        pub fn debug_list<'b>(&'b mut self) -> DebugList<'b, 'a> {
            let r = self.write_str("[");
            DebugList { fmt: self, result: r, has_fields: false }
        }
    }

    impl<'a> Write for Formatter<'a> {
        fn write_str(&mut self, s: &str) -> Result { self.out.write_str(s) }
        fn write_char(&mut self, c: char) -> Result { self.out.write_char(c) }
    }

    // inserts four spaces at the start of every line (for `{:#?}`)
    struct PadAdapter<'a> { out: &'a mut dyn Write, on_newline: bool }
    impl<'a> Write for PadAdapter<'a> {
        fn write_str(&mut self, s: &str) -> Result {
            let mut it = s.chars();
            while let Some(c) = it.next() {
                if self.on_newline { self.out.write_str("    ")?; }
                self.on_newline = c == '\n';
                self.out.write_char(c)?;
            }
            Ok(())
        }
    }

    // writes `value` through a PadAdapter followed by `tail`
    fn write_padded(f: &mut Formatter<'_>, first: &str, value: &dyn Debug, tail: &str) -> Result {
        f.write_str(first)?;
        let mut pad = PadAdapter { out: &mut *f.out, on_newline: true };
        let mut sub = f.with_out(&mut pad);
        Debug::fmt(value, &mut sub)?;
        sub.write_str(tail)
    }

    fn write_padded_field(f: &mut Formatter<'_>, first: &str, name: &str, value: &dyn Debug) -> Result {
        f.write_str(first)?;
        let mut pad = PadAdapter { out: &mut *f.out, on_newline: true };
        let mut sub = f.with_out(&mut pad);
        sub.write_str(name)?;
        sub.write_str(": ")?;
        Debug::fmt(value, &mut sub)?;
        sub.write_str(",\n")
    }
    fn write_plain_field(f: &mut Formatter<'_>, prefix: &str, name: &str, value: &dyn Debug) -> Result {
        f.write_str(prefix)?;
        f.write_str(name)?;
        f.write_str(": ")?;
        Debug::fmt(value, f)
    }
    fn write_plain_value(f: &mut Formatter<'_>, prefix: &str, value: &dyn Debug) -> Result {
        f.write_str(prefix)?;
        Debug::fmt(value, f)
    }

    pub struct DebugStruct<'a, 'b> { fmt: &'a mut Formatter<'b>, result: Result, has_fields: bool }
    impl<'a, 'b> DebugStruct<'a, 'b> {
        pub fn field(&mut self, name: &str, value: &dyn Debug) -> &mut DebugStruct<'a, 'b> {
            if self.result.is_ok() {
                self.result = if self.fmt.alt {
                    let first = if self.has_fields { "" } else { " {\n" };
                    write_padded_field(&mut *self.fmt, first, name, value)
                } else {
                    let prefix = if self.has_fields { ", " } else { " { " };
                    write_plain_field(&mut *self.fmt, prefix, name, value)
                };
            }
            self.has_fields = true;
            self
        }
        pub fn finish(&mut self) -> Result {
            if self.has_fields && self.result.is_ok() {
                self.result = if self.fmt.alt { self.fmt.write_str("}") } else { self.fmt.write_str(" }") };
            }
            self.result
        }
    }

    pub struct DebugTuple<'a, 'b> { fmt: &'a mut Formatter<'b>, result: Result, fields: usize, empty_name: bool }
    impl<'a, 'b> DebugTuple<'a, 'b> {
        pub fn field(&mut self, value: &dyn Debug) -> &mut DebugTuple<'a, 'b> {
            if self.result.is_ok() {
                self.result = if self.fmt.alt {
                    let first = if self.fields == 0 { "(\n" } else { "" };
                    write_padded(&mut *self.fmt, first, value, ",\n")
                } else {
                    let prefix = if self.fields == 0 { "(" } else { ", " };
                    write_plain_value(&mut *self.fmt, prefix, value)
                };
            }
            self.fields += 1;
            self
        }
        pub fn finish(&mut self) -> Result {
            if self.fields > 0 && self.result.is_ok() {
                if self.fields == 1 && self.empty_name && !self.fmt.alt {
                    self.result = self.fmt.write_str(",");
                }
                if self.result.is_ok() { self.result = self.fmt.write_str(")"); }
            }
            self.result
        }
    }

    pub struct DebugList<'a, 'b> { fmt: &'a mut Formatter<'b>, result: Result, has_fields: bool }
    impl<'a, 'b> DebugList<'a, 'b> {
        pub fn entry(&mut self, value: &dyn Debug) -> &mut DebugList<'a, 'b> {
            if self.result.is_ok() {
                self.result = if self.fmt.alt {
                    let first = if self.has_fields { "" } else { "\n" };
                    write_padded(&mut *self.fmt, first, value, ",\n")
                } else {
                    let prefix = if self.has_fields { ", " } else { "" };
                    write_plain_value(&mut *self.fmt, prefix, value)
                };
            }
            self.has_fields = true;
            self
        }
        pub fn entries<I: ::core::iter::IntoIterator>(&mut self, entries: I) -> &mut DebugList<'a, 'b>
            where I::Item: Debug {
            let mut it = entries.into_iter();
            while let Some(e) = it.next() { self.entry(&e); }
            self
        }
        pub fn finish(&mut self) -> Result {
            if self.result.is_ok() { self.result = self.fmt.write_str("]"); }
            self.result
        }
    }

    // ---- impls for references, str, char, bool, unit, float
    impl<'a, T: Display + ?Sized> Display for &'a T {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result { Display::fmt(&**self, f) }
    }
    impl<'a, T: Display + ?Sized> Display for &'a mut T {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result { Display::fmt(&**self, f) }
    }
    impl<'a, T: Debug + ?Sized> Debug for &'a T {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result { Debug::fmt(&**self, f) }
    }
    impl<'a, T: Debug + ?Sized> Debug for &'a mut T {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result { Debug::fmt(&**self, f) }
    }
    impl Display for str {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result { f.pad(self) }
    }
    impl Debug for str {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result {
            f.write_char('"')?;
            let mut it = self.chars();
            while let Some(c) = it.next() { write_escaped(f, c, '"')?; }
            f.write_char('"')
        }
    }
    impl Display for char {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result {
            if !f.has_width && !f.has_prec { return f.write_char(*self); }
            let mut b = [0u8; 4];
            let n = ::core::char::encode_utf8(*self, &mut b);
            f.pad(::core::str::buf_str(&b, n))
        }
    }
    impl Debug for char {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result {
            f.write_char('\'')?;
            write_escaped(f, *self, '\'')?;
            f.write_char('\'')
        }
    }
    fn write_escaped(f: &mut Formatter<'_>, c: char, quote: char) -> Result {
        if c == '\n' { f.write_str("\\n") }
        else if c == '\r' { f.write_str("\\r") }
        else if c == '\t' { f.write_str("\\t") }
        else if c == '\\' { f.write_str("\\\\") }
        else if c == '\0' { f.write_str("\\0") }
        else if c == quote { f.write_char('\\')?; f.write_char(c) }
        else { f.write_char(c) }
    }
    impl Display for bool {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result { f.pad(if *self { "true" } else { "false" }) }
    }
    impl Debug for bool {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result { Display::fmt(self, f) }
    }
    impl Debug for () {
        fn fmt(&self, f: &mut Formatter<'_>) -> Result { f.pad("()") }
    }

    // f64: mode 0 Display, 1 Debug, 2 LowerExp, 3 UpperExp
    pub fn fmt_float(f: &mut Formatter<'_>, v: f64, mode: u8) -> Result {
        let bits = ::core::intrinsics::f64_to_bits(v);
        let neg = (bits >> 63) != 0 && v == v;
        let mut buf = [0u8; 400];
        let n = ::core::intrinsics::fmt_f64(v, f.prec, f.has_prec, mode, ::core::intrinsics::slice_base::<u8>(&buf));
        f.pad_integral(!neg, "", ::core::str::buf_str(&buf, n))
    }
    impl Display for f64 { fn fmt(&self, f: &mut Formatter<'_>) -> Result { fmt_float(f, *self, 0) } }
    impl Debug for f64 { fn fmt(&self, f: &mut Formatter<'_>) -> Result { fmt_float(f, *self, 1) } }
    impl LowerExp for f64 { fn fmt(&self, f: &mut Formatter<'_>) -> Result { fmt_float(f, *self, 2) } }
    impl UpperExp for f64 { fn fmt(&self, f: &mut Formatter<'_>) -> Result { fmt_float(f, *self, 3) } }
}

// ---------------------------------------------------------------- io

pub mod io {
    use ::core::fmt;
    use ::core::string::String;

    pub struct RawOut { port: i64 }
    impl fmt::Write for RawOut {
        fn write_str(&mut self, s: &str) -> fmt::Result {
            ::core::intrinsics::write_bytes(self.port, ::core::intrinsics::str_base(s), ::core::intrinsics::str_len(s));
            ::core::result::Result::Ok(())
        }
    }
    pub fn stdout_raw() -> RawOut { RawOut { port: 1 } }
    pub fn stderr_raw() -> RawOut { RawOut { port: 2 } }

    pub struct Error { kind: u8 }
    impl fmt::Debug for Error {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { f.write_str("Error") }
    }
    impl fmt::Display for Error {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { f.write_str("I/O error") }
    }
    impl ::core::convert::From<fmt::Error> for Error {
        fn from(e: fmt::Error) -> Error { Error { kind: 1 } }
    }
    pub type Result<T> = ::core::result::Result<T, Error>;

    pub trait Write {
        fn write(&mut self, buf: &[u8]) -> Result<usize>;
        fn flush(&mut self) -> Result<()>;
        fn write_all(&mut self, buf: &[u8]) -> Result<()> {
            let n = buf.len();
            let mut done = 0usize;
            while done < n {
                match self.write(::core::slice::sub_slice(buf, done, n)) {
                    ::core::result::Result::Ok(0) => return ::core::result::Result::Err(Error { kind: 2 }),
                    ::core::result::Result::Ok(k) => { done += k; }
                    ::core::result::Result::Err(e) => return ::core::result::Result::Err(e),
                }
            }
            ::core::result::Result::Ok(())
        }
    }
    impl<'a, W: Write + ?Sized> Write for &'a mut W {
        fn write(&mut self, buf: &[u8]) -> Result<usize> { (**self).write(buf) }
        fn flush(&mut self) -> Result<()> { (**self).flush() }
    }
    impl Write for ::core::vec::Vec<u8> {
        fn write(&mut self, buf: &[u8]) -> Result<usize> { self.extend_from_slice(buf); ::core::result::Result::Ok(buf.len()) }
        fn flush(&mut self) -> Result<()> { ::core::result::Result::Ok(()) }
    }
    pub trait Read {
        fn read(&mut self, buf: &mut [u8]) -> Result<usize>;
        fn read_to_end(&mut self, out: &mut ::core::vec::Vec<u8>) -> Result<usize> {
            let mut one = [0u8; 1];
            let mut n = 0usize;
            loop {
                match self.read(&mut one) {
                    ::core::result::Result::Ok(0) => break,
                    ::core::result::Result::Ok(_) => { out.push(one[0]); n += 1; }
                    ::core::result::Result::Err(e) => return ::core::result::Result::Err(e),
                }
            }
            ::core::result::Result::Ok(n)
        }
        fn read_to_string(&mut self, buf: &mut String) -> Result<usize> {
            let mut bytes: ::core::vec::Vec<u8> = ::core::vec::Vec::new();
            let n = match self.read_to_end(&mut bytes) {
                ::core::result::Result::Ok(n) => n,
                ::core::result::Result::Err(e) => return ::core::result::Result::Err(e),
            };
            if !::core::str::is_valid_utf8(&bytes) { return ::core::result::Result::Err(Error { kind: 3 }); }
            buf.vec.append(&mut bytes);
            ::core::result::Result::Ok(n)
        }
    }
    pub trait BufRead: Read {
        fn read_line(&mut self, buf: &mut String) -> Result<usize> {
            let mut one = [0u8; 1];
            let mut n = 0usize;
            loop {
                match self.read(&mut one) {
                    ::core::result::Result::Ok(0) => break,
                    ::core::result::Result::Ok(_) => { buf.vec.push(one[0]); n += 1; if one[0] == 10u8 { break; } }
                    ::core::result::Result::Err(e) => return ::core::result::Result::Err(e),
                }
            }
            ::core::result::Result::Ok(n)
        }
        fn lines(self) -> Lines<Self> where Self: Sized { Lines { buf: self } }
    }
    pub struct Lines<B> { buf: B }
    impl<B: BufRead> ::core::iter::Iterator for Lines<B> {
        type Item = Result<String>;
        fn next(&mut self) -> ::core::option::Option<Result<String>> {
            let mut s = String::new();
            match self.buf.read_line(&mut s) {
                ::core::result::Result::Ok(0) => ::core::option::Option::None,
                ::core::result::Result::Ok(_) => {
                    if s.vec.len > 0 && s.vec.get(s.vec.len - 1).unwrap() == &10u8 {
                        s.vec.pop();
                        if s.vec.len > 0 && s.vec.get(s.vec.len - 1).unwrap() == &13u8 { s.vec.pop(); }
                    }
                    ::core::option::Option::Some(::core::result::Result::Ok(s))
                }
                ::core::result::Result::Err(e) => ::core::option::Option::Some(::core::result::Result::Err(e)),
            }
        }
    }
    pub struct BufReader<R> { inner: R }
    impl<R: Read> BufReader<R> {
        pub fn new(inner: R) -> BufReader<R> { BufReader { inner: inner } }
    }
    impl<R: Read> Read for BufReader<R> {
        fn read(&mut self, buf: &mut [u8]) -> Result<usize> { self.inner.read(buf) }
    }
    impl<R: Read> BufRead for BufReader<R> {}
    pub struct BufWriter<W: Write> { inner: W }
    impl<W: Write> BufWriter<W> {
        pub fn new(inner: W) -> BufWriter<W> { BufWriter { inner: inner } }
    }
    impl<W: Write> Write for BufWriter<W> {
        fn write(&mut self, buf: &[u8]) -> Result<usize> { self.inner.write(buf) }
        fn flush(&mut self) -> Result<()> { self.inner.flush() }
    }
    impl<W: Write> fmt::Write for BufWriter<W> {
        fn write_str(&mut self, s: &str) -> fmt::Result {
            match self.inner.write_all(s.as_bytes()) {
                ::core::result::Result::Ok(_) => ::core::result::Result::Ok(()),
                ::core::result::Result::Err(_) => ::core::result::Result::Err(fmt::Error),
            }
        }
    }
    pub struct Stdout { raw: RawOut }
    pub struct Stderr { raw: RawOut }
    pub fn stdout() -> Stdout { Stdout { raw: RawOut { port: 1 } } }
    pub fn stderr() -> Stderr { Stderr { raw: RawOut { port: 2 } } }
    impl Stdout { pub fn lock(&self) -> Stdout { stdout() } }
    impl Stderr { pub fn lock(&self) -> Stderr { stderr() } }
    // write!(w, ..) expands to `&mut *(w)`; these Derefs let it accept an owned handle
    impl ::core::ops::Deref for Stdout { type Target = RawOut; fn deref(&self) -> &RawOut { &self.raw } }
    impl ::core::ops::DerefMut for Stdout { fn deref_mut(&mut self) -> &mut RawOut { &mut self.raw } }
    impl ::core::ops::Deref for Stderr { type Target = RawOut; fn deref(&self) -> &RawOut { &self.raw } }
    impl ::core::ops::DerefMut for Stderr { fn deref_mut(&mut self) -> &mut RawOut { &mut self.raw } }
    impl fmt::Write for Stdout {
        fn write_str(&mut self, s: &str) -> fmt::Result {
            ::core::intrinsics::write_bytes(1, ::core::intrinsics::str_base(s), ::core::intrinsics::str_len(s));
            ::core::result::Result::Ok(())
        }
    }
    impl fmt::Write for Stderr {
        fn write_str(&mut self, s: &str) -> fmt::Result {
            ::core::intrinsics::write_bytes(2, ::core::intrinsics::str_base(s), ::core::intrinsics::str_len(s));
            ::core::result::Result::Ok(())
        }
    }
    impl Write for Stdout {
        fn write(&mut self, buf: &[u8]) -> Result<usize> {
            ::core::intrinsics::write_bytes(1, ::core::intrinsics::slice_base::<u8>(buf), buf.len());
            ::core::result::Result::Ok(buf.len())
        }
        fn flush(&mut self) -> Result<()> { ::core::result::Result::Ok(()) }
    }
    impl Write for Stderr {
        fn write(&mut self, buf: &[u8]) -> Result<usize> {
            ::core::intrinsics::write_bytes(2, ::core::intrinsics::slice_base::<u8>(buf), buf.len());
            ::core::result::Result::Ok(buf.len())
        }
        fn flush(&mut self) -> Result<()> { ::core::result::Result::Ok(()) }
    }

    pub struct Stdin;
    pub fn stdin() -> Stdin { Stdin }
    // appends one line including its '\n'; returns the number of bytes read (0 at end of input)
    fn stdin_read_line(buf: &mut String) -> Result<usize> {
        let mut n = 0usize;
        loop {
            let b = ::core::intrinsics::read_byte(0);
            if b < 0 { break; }
            buf.vec.push(b as u8);
            n += 1;
            if b == 10 { break; }
        }
        ::core::result::Result::Ok(n)
    }
    fn stdin_read_all(buf: &mut String) -> Result<usize> {
        let mut n = 0usize;
        loop {
            let b = ::core::intrinsics::read_byte(0);
            if b < 0 { break; }
            buf.vec.push(b as u8);
            n += 1;
        }
        ::core::result::Result::Ok(n)
    }
    impl Stdin {
        pub fn lock(&self) -> Stdin { Stdin }
        pub fn read_line(&self, buf: &mut String) -> Result<usize> { stdin_read_line(buf) }
        pub fn read_to_string(&self, buf: &mut String) -> Result<usize> { stdin_read_all(buf) }
        pub fn lines(self) -> Lines<Stdin> { Lines { buf: Stdin } }
    }
    impl Read for Stdin {
        fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
            let n = buf.len();
            let mut i = 0usize;
            while i < n {
                let b = ::core::intrinsics::read_byte(0);
                if b < 0 { break; }
                buf[i] = b as u8;
                i += 1;
            }
            ::core::result::Result::Ok(i)
        }
        fn read_to_string(&mut self, buf: &mut String) -> Result<usize> { stdin_read_all(buf) }
    }
    impl BufRead for Stdin {
        fn read_line(&mut self, buf: &mut String) -> Result<usize> { stdin_read_line(buf) }
    }
}

pub mod env {
    use ::core::string::String;
    // the simulator passes no command line: args() is just the program name
    pub struct Args { done: bool }
    pub fn args() -> Args { Args { done: false } }
    impl ::core::iter::Iterator for Args {
        type Item = String;
        fn next(&mut self) -> ::core::option::Option<String> {
            if self.done { ::core::option::Option::None }
            else { self.done = true; ::core::option::Option::Some(String::from("main")) }
        }
    }
}

// ---------------------------------------------------------------- panicking

pub mod panicking {
    use ::core::fmt::{Debug, Formatter, Write};
    use ::core::io::RawOut;

    const NOTE: &str = "\nnote: run with `RUST_BACKTRACE=1` environment variable to display a backtrace\n";

    // prints "thread 'main' panicked at FILE:LINE:COL:\n" to stderr; the caller writes the message
    pub fn begin(file: &str, line: u32, col: u32) -> RawOut {
        let mut out = ::core::io::stderr_raw();
        {
            let mut f = Formatter::new(&mut out);
            let _ = f.write_str("thread 'main' panicked at ");
            let _ = f.write_str(file);
            let _ = f.write_str(":");
            let _ = ::core::fmt::Display::fmt(&line, &mut f);
            let _ = f.write_str(":");
            let _ = ::core::fmt::Display::fmt(&col, &mut f);
            let _ = f.write_str(":\n");
        }
        out
    }

    // prints the backtrace note and exits with status 101
    #[lang = "panic_end"]
    pub fn end() -> ! {
        let mut out = ::core::io::stderr_raw();
        let _ = out.write_str(NOTE);
        ::core::intrinsics::exit(101)
    }

    #[lang = "panic_str"]
    pub fn panic_str(file: &str, line: u32, col: u32, msg: &str) -> ! {
        let mut out = begin(file, line, col);
        let _ = out.write_str(msg);
        end()
    }

    #[lang = "panic_bounds_check"]
    pub fn panic_bounds_check(file: &str, line: u32, col: u32, index: usize, len: usize) -> ! {
        let mut out = begin(file, line, col);
        {
            let mut f = Formatter::new(&mut out);
            let _ = f.write_str("index out of bounds: the len is ");
            let _ = ::core::fmt::Display::fmt(&len, &mut f);
            let _ = f.write_str(" but the index is ");
            let _ = ::core::fmt::Display::fmt(&index, &mut f);
        }
        end()
    }

    // `msg: {value:?}` for unwrap / expect on Err and Ok
    pub fn unwrap_failed(msg: &str, value: &dyn Debug) -> ! {
        let mut out = begin("library/core/src/result.rs", 1, 1);
        {
            let mut f = Formatter::new(&mut out);
            let _ = f.write_str(msg);
            let _ = f.write_str(": ");
            let _ = Debug::fmt(value, &mut f);
        }
        end()
    }

    pub fn assert_failed<T: Debug + ?Sized, U: Debug + ?Sized>(op: &str, l: &T, r: &U, file: &str, line: u32, col: u32) -> ! {
        let mut out = begin(file, line, col);
        {
            let mut f = Formatter::new(&mut out);
            let _ = f.write_str("assertion `left ");
            let _ = f.write_str(op);
            let _ = f.write_str(" right` failed\n  left: ");
            let _ = Debug::fmt(l, &mut f);
            let _ = f.write_str("\n right: ");
            let _ = Debug::fmt(r, &mut f);
        }
        end()
    }

    pub fn assert_failed_msg<T: Debug + ?Sized, U: Debug + ?Sized>(op: &str, l: &T, r: &U, file: &str, line: u32, col: u32, msg: &str) -> ! {
        let mut out = begin(file, line, col);
        {
            let mut f = Formatter::new(&mut out);
            let _ = f.write_str("assertion `left ");
            let _ = f.write_str(op);
            let _ = f.write_str(" right` failed: ");
            let _ = f.write_str(msg);
            let _ = f.write_str("\n  left: ");
            let _ = Debug::fmt(l, &mut f);
            let _ = f.write_str("\n right: ");
            let _ = Debug::fmt(r, &mut f);
        }
        end()
    }
}

// ---------------------------------------------------------------- num

pub mod num {
    use ::core::fmt;
    use ::core::option::Option::{self, Some, None};
    use ::core::result::Result::{self, Ok, Err};

    // digits of `mag` in `radix` (2..=16) written through Formatter::pad_integral
    pub fn fmt_radix(f: &mut fmt::Formatter<'_>, nonneg: bool, mag: u64, radix: u64, upper: bool, prefix: &str) -> fmt::Result {
        let mut buf = [0u8; 70];
        let mut i = 69usize;
        let mut m = mag;
        loop {
            let d = (m % radix) as u8;
            buf[i] = if d < 10 { b'0' + d } else if upper { b'A' + (d - 10) } else { b'a' + (d - 10) };
            m = m / radix;
            if m == 0 { break; }
            i -= 1;
        }
        f.pad_integral(nonneg, prefix, ::core::str::buf_range(&buf, i, 70))
    }

    // kind: 0 Empty, 1 InvalidDigit, 2 PosOverflow, 3 NegOverflow
    pub struct ParseIntError { pub kind: u8 }
    impl ParseIntError {
        fn name(&self) -> &str {
            if self.kind == 0 { "Empty" } else if self.kind == 1 { "InvalidDigit" }
            else if self.kind == 2 { "PosOverflow" } else { "NegOverflow" }
        }
    }
    impl fmt::Debug for ParseIntError {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            f.write_str("ParseIntError { kind: ")?;
            f.write_str(self.name())?;
            f.write_str(" }")
        }
    }
    impl fmt::Display for ParseIntError {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            f.pad(if self.kind == 0 { "cannot parse integer from empty string" }
                  else if self.kind == 1 { "invalid digit found in string" }
                  else if self.kind == 2 { "number too large to fit in target type" }
                  else { "number too small to fit in target type" })
        }
    }
    impl ::core::clone::Clone for ParseIntError { fn clone(&self) -> ParseIntError { ParseIntError { kind: self.kind } } }
    impl ::core::cmp::PartialEq for ParseIntError { fn eq(&self, o: &ParseIntError) -> bool { self.kind == o.kind } }
    impl ::core::cmp::Eq for ParseIntError {}

    // (negative, magnitude); "+5" and "-5" are accepted, "-" alone is InvalidDigit
    pub fn parse_int(s: &str, unsigned: bool, radix: u32) -> Result<(bool, u64), ParseIntError> {
        let n = s.len();
        if n == 0 { return Err(ParseIntError { kind: 0 }); }
        let bytes = s.as_bytes();
        let mut i = 0usize;
        let mut neg = false;
        if bytes[0] == b'+' { i = 1; }
        else if bytes[0] == b'-' {
            if unsigned { return Err(ParseIntError { kind: 1 }); }
            neg = true;
            i = 1;
        }
        if i == n { return Err(ParseIntError { kind: 1 }); }
        let mut acc: u64 = 0;
        while i < n {
            let c = bytes[i];
            let dv: u32 = if c >= b'0' && c <= b'9' { (c - b'0') as u32 }
                else if c >= b'a' && c <= b'z' { (c - b'a') as u32 + 10 }
                else if c >= b'A' && c <= b'Z' { (c - b'A') as u32 + 10 }
                else { 99 };
            if dv >= radix { return Err(ParseIntError { kind: 1 }); }
            let d = dv as u64;
            let ovf = ParseIntError { kind: if neg { 3 } else { 2 } };
            match acc.checked_mul(radix as u64) {
                None => return Err(ovf),
                Some(v) => match v.checked_add(d) {
                    None => return Err(ovf),
                    Some(w) => { acc = w; }
                },
            }
            i += 1;
        }
        Ok((neg, acc))
    }

    pub struct ParseFloatError { kind: u8 }
    impl fmt::Debug for ParseFloatError {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            f.write_str(if self.kind == 0 { "ParseFloatError { kind: Empty }" } else { "ParseFloatError { kind: Invalid }" })
        }
    }
    impl fmt::Display for ParseFloatError {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            f.pad(if self.kind == 0 { "cannot parse float from empty string" } else { "invalid float literal" })
        }
    }
    impl ::core::str::FromStr for f64 {
        type Err = ParseFloatError;
        fn from_str(s: &str) -> Result<f64, ParseFloatError> {
            if s.len() == 0 { return Err(ParseFloatError { kind: 0 }); }
            let mut v = 0.0f64;
            if ::core::intrinsics::parse_f64(::core::intrinsics::str_base(s), s.len(), &mut v) { Ok(v) }
            else { Err(ParseFloatError { kind: 1 }) }
        }
    }

    impl f64 {
        pub const MAX: f64 = 1.7976931348623157e308;
        pub const MIN: f64 = -1.7976931348623157e308;
        pub const MIN_POSITIVE: f64 = 2.2250738585072014e-308;
        pub const EPSILON: f64 = 2.220446049250313e-16;
        pub const INFINITY: f64 = 1.0e308 * 10.0;
        pub const NEG_INFINITY: f64 = -1.0e308 * 10.0;
        pub const NAN: f64 = 0.0 / 0.0;
        pub fn abs(self) -> f64 { ::core::intrinsics::f64_from_bits(::core::intrinsics::f64_to_bits(self) & 0x7fffffffffffffffu64) }
        pub fn sqrt(self) -> f64 { ::core::intrinsics::sqrtf64(self) }
        pub fn floor(self) -> f64 { ::core::intrinsics::floorf64(self) }
        pub fn ceil(self) -> f64 { ::core::intrinsics::ceilf64(self) }
        pub fn trunc(self) -> f64 { if self >= 0.0 { self.floor() } else { self.ceil() } }
        pub fn round(self) -> f64 {
            if self.is_nan() || self.is_infinite() { return self; }
            let a = self.abs();
            if a >= 4503599627370496.0 { return self; }
            let t = a.floor();
            let r = if a - t >= 0.5 { t + 1.0 } else { t };
            r.copysign(self)
        }
        pub fn fract(self) -> f64 { self - self.trunc() }
        pub fn powi(self, n: i32) -> f64 {
            let mut e: i64 = if n < 0 { -(n as i64) } else { n as i64 };
            let mut b = self;
            let mut r = 1.0f64;
            while e > 0 {
                if e & 1 == 1 { r = r * b; }
                b = b * b;
                e = e >> 1;
            }
            if n < 0 { 1.0 / r } else { r }
        }
        pub fn min(self, o: f64) -> f64 { if self.is_nan() { o } else if o.is_nan() { self } else if self < o { self } else { o } }
        pub fn max(self, o: f64) -> f64 { if self.is_nan() { o } else if o.is_nan() { self } else if self > o { self } else { o } }
        pub fn is_nan(self) -> bool { (::core::intrinsics::f64_to_bits(self) & 0x7fffffffffffffffu64) > 0x7ff0000000000000u64 }
        pub fn is_infinite(self) -> bool { self == f64::INFINITY || self == f64::NEG_INFINITY }
        pub fn is_finite(self) -> bool { !self.is_nan() && !self.is_infinite() }
        pub fn is_sign_negative(self) -> bool { (::core::intrinsics::f64_to_bits(self) >> 63) != 0 }
        pub fn signum(self) -> f64 { if self.is_nan() { self } else if self.is_sign_negative() { -1.0 } else { 1.0 } }
        pub fn to_bits(self) -> u64 { ::core::intrinsics::f64_to_bits(self) }
        pub fn from_bits(b: u64) -> f64 { ::core::intrinsics::f64_from_bits(b) }
        pub fn clamp(self, lo: f64, hi: f64) -> f64 { if self < lo { lo } else if self > hi { hi } else { self } }
        pub fn abs_sub(self, o: f64) -> f64 { if self > o { self - o } else { 0.0 } }
    }
    // 2^k for -1022 <= k <= 1023
    pub fn pow2i(k: i64) -> f64 { ::core::intrinsics::f64_from_bits(((k + 1023) as u64) << 52) }
    pub fn scale2(v: f64, k: i64) -> f64 {
        if k > 1023 { v * pow2i(1023) * pow2i(k - 1023) }
        else if k < -1022 { v * pow2i(-1000) * pow2i(k + 1000) }
        else { v * pow2i(k) }
    }
    // x = lnm_e.1 * 2^k * m with m in [sqrt(1/2), sqrt(2)): returns (ln m, k)
    pub fn ln_parts(x: f64) -> (f64, i64) {
        let mut bits = ::core::intrinsics::f64_to_bits(x);
        let mut e: i64 = 0;
        if (bits >> 52) == 0 {
            bits = ::core::intrinsics::f64_to_bits(x * 18014398509481984.0);
            e = -54;
        }
        e += ((bits >> 52) & 0x7ff) as i64 - 1023;
        let mut m = ::core::intrinsics::f64_from_bits((bits & 0x000fffffffffffffu64) | 0x3ff0000000000000u64);
        if m > 1.4142135623730951 { m = m * 0.5; e += 1; }
        let f = (m - 1.0) / (m + 1.0);
        let s = f * f;
        let mut term = s;
        let mut sum = 1.0f64;
        let mut k = 3.0f64;
        let mut i = 0;
        while i < 13 { sum = sum + term / k; term = term * s; k = k + 2.0; i += 1; }
        (2.0 * f * sum, e)
    }
    // x = q * pi/2 + r with |r| <= pi/4: returns (sin r, cos r, q mod 4)
    pub fn trig_parts(x: f64) -> (f64, f64, i64) {
        let kf = (x * 0.6366197723675814).round();
        let r = (x - kf * 1.57079632673412561417e+00) - kf * 6.07710050650619224932e-11;
        let s = r * r;
        let mut st = r;
        let mut sn = r;
        let mut ct = 1.0f64;
        let mut cs = 1.0f64;
        let mut i = 1;
        while i <= 13 {
            let a = (2 * i) as f64;
            ct = -ct * s / ((a - 1.0) * a);
            cs = cs + ct;
            st = -st * s / (a * (a + 1.0));
            sn = sn + st;
            i += 1;
        }
        let k = kf as i64;
        (sn, cs, ((k % 4) + 4) % 4)
    }

    impl f64 {
        pub const RADIX: u32 = 2;
        pub fn copysign(self, s: f64) -> f64 {
            ::core::intrinsics::f64_from_bits((::core::intrinsics::f64_to_bits(self) & 0x7fffffffffffffffu64)
                | (::core::intrinsics::f64_to_bits(s) & 0x8000000000000000u64))
        }
        pub fn is_sign_positive(self) -> bool { !self.is_sign_negative() }
        pub fn recip(self) -> f64 { 1.0 / self }
        pub fn mul_add(self, a: f64, b: f64) -> f64 { self * a + b }
        pub fn to_radians(self) -> f64 { self * (3.14159265358979323846264338327950288 / 180.0) }
        pub fn to_degrees(self) -> f64 { self * 57.2957795130823208767981548141051703 }
        pub fn exp(self) -> f64 {
            let x = self;
            if x.is_nan() { return x; }
            if x > 709.782712893384 { return f64::INFINITY; }
            if x < -745.2 { return 0.0; }
            let kf = (x * 1.4426950408889634).round();
            let r = (x - kf * 6.93147180369123816490e-01) - kf * 1.90821492927058770002e-10;
            let mut term = 1.0f64;
            let mut sum = 1.0f64;
            let mut n = 1.0f64;
            while n < 22.0 { term = term * r / n; sum = sum + term; n = n + 1.0; }
            scale2(sum, kf as i64)
        }
        pub fn exp2(self) -> f64 { (self * 0.6931471805599453).exp() }
        pub fn ln(self) -> f64 {
            let x = self;
            if x.is_nan() || x < 0.0 { return f64::NAN; }
            if x == 0.0 { return f64::NEG_INFINITY; }
            if x == f64::INFINITY { return x; }
            let p = ln_parts(x);
            let ef = p.1 as f64;
            ef * 6.93147180369123816490e-01 + (ef * 1.90821492927058770002e-10 + p.0)
        }
        pub fn log2(self) -> f64 {
            let x = self;
            if x.is_nan() || x < 0.0 { return f64::NAN; }
            if x == 0.0 { return f64::NEG_INFINITY; }
            if x == f64::INFINITY { return x; }
            let p = ln_parts(x);
            (p.1 as f64) + p.0 * 1.4426950408889634
        }
        pub fn log10(self) -> f64 { self.ln() / 2.302585092994046 }
        pub fn log(self, base: f64) -> f64 { self.ln() / base.ln() }
        pub fn powf(self, y: f64) -> f64 {
            let x = self;
            if y == 0.0 || x == 1.0 { return 1.0; }
            if x.is_nan() || y.is_nan() { return f64::NAN; }
            let yi = y.trunc();
            if x == 0.0 {
                let odd = yi == y && y.abs() < 9007199254740992.0 && ((yi.abs() as u64) & 1) == 1;
                return if y > 0.0 { if odd { x } else { 0.0 } }
                       else { if odd { 1.0 / x } else { f64::INFINITY } };
            }
            if yi == y && y.abs() <= 2147483647.0 { return x.powi(y as i32); }
            if x < 0.0 { return f64::NAN; }
            if y.abs() < 2147483647.0 {
                let yf = y - yi;
                let frac = if yf == 0.5 { x.sqrt() }
                           else if yf == -0.5 { 1.0 / x.sqrt() }
                           else { (yf * x.ln()).exp() };
                return x.powi(yi as i32) * frac;
            }
            (y * x.ln()).exp()
        }
        pub fn cbrt(self) -> f64 {
            let x = self;
            if x == 0.0 || x.is_nan() || x.is_infinite() { return x; }
            let a = x.abs();
            let mut y = (a.ln() / 3.0).exp();
            y = y - (y * y * y - a) / (3.0 * y * y);
            y = y - (y * y * y - a) / (3.0 * y * y);
            let r = y.round();
            if r * r * r == a { y = r; }
            if x < 0.0 { -y } else { y }
        }
        pub fn hypot(self, o: f64) -> f64 {
            let mut a = self.abs();
            let mut b = o.abs();
            if a.is_infinite() || b.is_infinite() { return f64::INFINITY; }
            if a.is_nan() || b.is_nan() { return f64::NAN; }
            if a < b { let t = a; a = b; b = t; }
            if a == 0.0 { return 0.0; }
            let r = b / a;
            a * (1.0 + r * r).sqrt()
        }
        pub fn sin(self) -> f64 {
            if self.is_nan() || self.is_infinite() { return f64::NAN; }
            let t = trig_parts(self);
            if t.2 == 0 { t.0 } else if t.2 == 1 { t.1 } else if t.2 == 2 { -t.0 } else { -t.1 }
        }
        pub fn cos(self) -> f64 {
            if self.is_nan() || self.is_infinite() { return f64::NAN; }
            let t = trig_parts(self);
            if t.2 == 0 { t.1 } else if t.2 == 1 { -t.0 } else if t.2 == 2 { -t.1 } else { t.0 }
        }
        pub fn tan(self) -> f64 {
            if self.is_nan() || self.is_infinite() { return f64::NAN; }
            let t = trig_parts(self);
            if t.2 % 2 == 0 { t.0 / t.1 } else { -t.1 / t.0 }
        }
        pub fn sin_cos(self) -> (f64, f64) { (self.sin(), self.cos()) }
        pub fn atan(self) -> f64 {
            let x = self;
            if x.is_nan() { return x; }
            let a = x.abs();
            if a == f64::INFINITY { return if x < 0.0 { -1.57079632679489661923132169163975144 } else { 1.57079632679489661923132169163975144 }; }
            let inv = a > 1.0;
            let mut y = if inv { 1.0 / a } else { a };
            y = y / (1.0 + (1.0 + y * y).sqrt());
            y = y / (1.0 + (1.0 + y * y).sqrt());
            let s = y * y;
            let mut term = y;
            let mut sum = y;
            let mut k = 1;
            while k <= 14 {
                term = -term * s;
                sum = sum + term / ((2 * k + 1) as f64);
                k += 1;
            }
            let mut r = 4.0 * sum;
            if inv { r = 1.57079632679489661923132169163975144 - r; }
            if x < 0.0 { -r } else { r }
        }
        pub fn atan2(self, x: f64) -> f64 {
            let y = self;
            let pi = 3.14159265358979323846264338327950288;
            if y.is_nan() || x.is_nan() { return f64::NAN; }
            if x == 0.0 {
                if y == 0.0 {
                    return if x.is_sign_negative() { if y.is_sign_negative() { -pi } else { pi } } else { y };
                }
                return if y > 0.0 { pi / 2.0 } else { -pi / 2.0 };
            }
            if x > 0.0 { return (y / x).atan(); }
            if y.is_sign_negative() { (y / x).atan() - pi } else { (y / x).atan() + pi }
        }
        pub fn asin(self) -> f64 {
            if self.abs() > 1.0 { return f64::NAN; }
            self.atan2((1.0 - self * self).sqrt())
        }
        pub fn acos(self) -> f64 {
            if self.abs() > 1.0 { return f64::NAN; }
            (1.0 - self * self).sqrt().atan2(self)
        }
        pub fn sinh(self) -> f64 { let e = self.exp(); (e - 1.0 / e) / 2.0 }
        pub fn cosh(self) -> f64 { let e = self.exp(); (e + 1.0 / e) / 2.0 }
        pub fn tanh(self) -> f64 {
            if self > 20.0 { return 1.0; }
            if self < -20.0 { return -1.0; }
            let e = (2.0 * self).exp();
            (e - 1.0) / (e + 1.0)
        }
        pub fn div_euclid(self, o: f64) -> f64 {
            let q = (self / o).trunc();
            if self % o < 0.0 { if o > 0.0 { q - 1.0 } else { q + 1.0 } } else { q }
        }
        pub fn rem_euclid(self, o: f64) -> f64 {
            let r = self % o;
            if r < 0.0 { r + o.abs() } else { r }
        }
    }
    impl<'a> ::core::ops::Neg for &'a f64 { type Output = f64; fn neg(self) -> f64 { -*self } }
    impl ::core::iter::Sum<f64> for f64 {
        fn sum<I: ::core::iter::Iterator<Item = f64>>(iter: I) -> f64 {
            let mut a = 0.0f64;
            let mut it = iter;
            while let Some(x) = it.next() { a = a + x; }
            a
        }
    }
    impl<'a> ::core::iter::Sum<&'a f64> for f64 {
        fn sum<I: ::core::iter::Iterator<Item = &'a f64>>(iter: I) -> f64 {
            let mut a = 0.0f64;
            let mut it = iter;
            while let Some(x) = it.next() { a = a + *x; }
            a
        }
    }
    impl<'a> ::core::iter::Product<&'a f64> for f64 {
        fn product<I: ::core::iter::Iterator<Item = &'a f64>>(iter: I) -> f64 {
            let mut a = 1.0f64;
            let mut it = iter;
            while let Some(x) = it.next() { a = a * *x; }
            a
        }
    }
    impl ::core::iter::Product<f64> for f64 {
        fn product<I: ::core::iter::Iterator<Item = f64>>(iter: I) -> f64 {
            let mut a = 1.0f64;
            let mut it = iter;
            while let Some(x) = it.next() { a = a * x; }
            a
        }
    }
}

// std::f64::consts and std::{i32, u32, ...}::MAX style paths
pub mod f64 {
    pub const MAX: f64 = 1.7976931348623157e308;
    pub const MIN: f64 = -1.7976931348623157e308;
    pub const EPSILON: f64 = 2.220446049250313e-16;
    pub const INFINITY: f64 = 1.0e308 * 10.0;
    pub const NEG_INFINITY: f64 = -1.0e308 * 10.0;
    pub const NAN: f64 = 0.0 / 0.0;
    pub mod consts {
        pub const PI: f64 = 3.14159265358979323846264338327950288;
        pub const TAU: f64 = 6.28318530717958647692528676655900577;
        pub const E: f64 = 2.71828182845904523536028747135266250;
        pub const SQRT_2: f64 = 1.41421356237309504880168872420969808;
        pub const LN_2: f64 = 0.693147180559945309417232121458176568;
        pub const LN_10: f64 = 2.30258509299404568401799145468436421;
        pub const FRAC_PI_2: f64 = 1.57079632679489661923132169163975144;
        pub const FRAC_PI_4: f64 = 0.785398163397448309615660845819875721;
    }
}

// ---------------------------------------------------------------- char

pub mod char {
    use ::core::option::Option::{self, Some, None};

    // UTF-8 encodes `c` into buf (at least 4 bytes) and returns the byte count
    pub fn encode_utf8(c: char, buf: &mut [u8]) -> usize {
        let u = c as u32;
        if u < 0x80 {
            buf[0] = u as u8;
            1
        } else if u < 0x800 {
            buf[0] = (0xC0 | (u >> 6)) as u8;
            buf[1] = (0x80 | (u & 0x3F)) as u8;
            2
        } else if u < 0x10000 {
            buf[0] = (0xE0 | (u >> 12)) as u8;
            buf[1] = (0x80 | ((u >> 6) & 0x3F)) as u8;
            buf[2] = (0x80 | (u & 0x3F)) as u8;
            3
        } else {
            buf[0] = (0xF0 | (u >> 18)) as u8;
            buf[1] = (0x80 | ((u >> 12) & 0x3F)) as u8;
            buf[2] = (0x80 | ((u >> 6) & 0x3F)) as u8;
            buf[3] = (0x80 | (u & 0x3F)) as u8;
            4
        }
    }
    pub fn from_u32(i: u32) -> Option<char> {
        if i > 0x10FFFF || (i >= 0xD800 && i <= 0xDFFF) { None }
        else { Some(::core::intrinsics::transmute::<u32, char>(i)) }
    }
    pub fn from_digit(num: u32, radix: u32) -> Option<char> {
        if num >= radix { None }
        else if num < 10 { Some((b'0' + num as u8) as char) }
        else { Some((b'a' + (num - 10) as u8) as char) }
    }

    // Simple case mapping for ASCII, Latin-1 Supplement, Latin Extended-A, Greek and Cyrillic.
    // Pairs ranges are (even = upper, odd = lower) unless `odd_up` is set.
    fn in_pairs(u: u32, lo: u32, hi: u32) -> bool { u >= lo && u <= hi }

    pub fn upper_u32(u: u32) -> u32 {
        if u < 128 { return if u >= 97 && u <= 122 { u - 32 } else { u }; }
        if u == 0xB5 { return 0x39C; }
        if u >= 0xE0 && u <= 0xFE && u != 0xF7 { return u - 32; }
        if u == 0xFF { return 0x178; }
        if u == 0x131 { return 73; }
        if u == 0x17F { return 83; }
        if (in_pairs(u, 0x100, 0x12F) || in_pairs(u, 0x132, 0x137) || in_pairs(u, 0x14A, 0x177)) { return if u % 2 == 1 { u - 1 } else { u }; }
        if (in_pairs(u, 0x139, 0x148) || in_pairs(u, 0x179, 0x17E)) { return if u % 2 == 0 { u - 1 } else { u }; }
        if u >= 0x3B1 && u <= 0x3CB && u != 0x3C2 { return u - 32; }
        if u == 0x3C2 { return 0x3A3; }
        if u == 0x3AC { return 0x386; }
        if u >= 0x3AD && u <= 0x3AF { return u - 37; }
        if u == 0x3CC { return 0x38C; }
        if u == 0x3CD || u == 0x3CE { return u - 63; }
        if u >= 0x430 && u <= 0x44F { return u - 32; }
        if u >= 0x450 && u <= 0x45F { return u - 80; }
        if (in_pairs(u, 0x460, 0x481) || in_pairs(u, 0x48A, 0x4BF) || in_pairs(u, 0x4D0, 0x52F)) { return if u % 2 == 1 { u - 1 } else { u }; }
        u
    }

    pub fn lower_u32(u: u32) -> u32 {
        if u < 128 { return if u >= 65 && u <= 90 { u + 32 } else { u }; }
        if u >= 0xC0 && u <= 0xDE && u != 0xD7 { return u + 32; }
        if u == 0x178 { return 0xFF; }
        if (in_pairs(u, 0x100, 0x12F) || in_pairs(u, 0x132, 0x137) || in_pairs(u, 0x14A, 0x177)) { return if u % 2 == 0 { u + 1 } else { u }; }
        if (in_pairs(u, 0x139, 0x148) || in_pairs(u, 0x179, 0x17E)) { return if u % 2 == 1 { u + 1 } else { u }; }
        if u >= 0x391 && u <= 0x3AB && u != 0x3A2 { return u + 32; }
        if u == 0x386 { return 0x3AC; }
        if u >= 0x388 && u <= 0x38A { return u + 37; }
        if u == 0x38C { return 0x3CC; }
        if u == 0x38E || u == 0x38F { return u + 63; }
        if u >= 0x410 && u <= 0x42F { return u + 32; }
        if u >= 0x400 && u <= 0x40F { return u + 80; }
        if (in_pairs(u, 0x460, 0x481) || in_pairs(u, 0x48A, 0x4BF) || in_pairs(u, 0x4D0, 0x52F)) { return if u % 2 == 0 { u + 1 } else { u }; }
        u
    }

    // the iterator returned by char::to_uppercase / to_lowercase (always one char here)
    pub struct ToCase { c: Option<char> }
    impl ::core::iter::Iterator for ToCase {
        type Item = char;
        fn next(&mut self) -> Option<char> { self.c.take() }
    }
    impl ::core::fmt::Display for ToCase {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
            match self.c { Some(c) => ::core::fmt::Display::fmt(&c, f), None => Ok(()) }
        }
    }

    impl char {
        pub const MAX: char = '\u{10FFFF}';
        pub fn from_u32(i: u32) -> Option<char> { from_u32(i) }
        pub fn from_digit(num: u32, radix: u32) -> Option<char> { from_digit(num, radix) }
        pub fn len_utf8(self) -> usize {
            let u = self as u32;
            if u < 0x80 { 1 } else if u < 0x800 { 2 } else if u < 0x10000 { 3 } else { 4 }
        }
        pub fn is_ascii(&self) -> bool { (*self as u32) < 128 }
        pub fn is_ascii_digit(&self) -> bool { *self >= '0' && *self <= '9' }
        pub fn is_ascii_uppercase(&self) -> bool { *self >= 'A' && *self <= 'Z' }
        pub fn is_ascii_lowercase(&self) -> bool { *self >= 'a' && *self <= 'z' }
        pub fn is_ascii_alphabetic(&self) -> bool { self.is_ascii_uppercase() || self.is_ascii_lowercase() }
        pub fn is_ascii_alphanumeric(&self) -> bool { self.is_ascii_alphabetic() || self.is_ascii_digit() }
        pub fn is_ascii_whitespace(&self) -> bool {
            *self == ' ' || *self == '\t' || *self == '\n' || *self == '\r' || *self == '\u{0C}'
        }
        pub fn is_ascii_punctuation(&self) -> bool {
            let u = *self as u32;
            (u >= 33 && u <= 47) || (u >= 58 && u <= 64) || (u >= 91 && u <= 96) || (u >= 123 && u <= 126)
        }
        pub fn is_alphabetic(self) -> bool { self.is_ascii_alphabetic() || (self as u32) > 127 }
        pub fn is_numeric(self) -> bool { self.is_ascii_digit() }
        pub fn is_alphanumeric(self) -> bool { self.is_alphabetic() || self.is_numeric() }
        pub fn is_whitespace(self) -> bool { self.is_ascii_whitespace() || self == '\u{0B}' }
        pub fn is_uppercase(self) -> bool { lower_u32(self as u32) != self as u32 }
        pub fn is_lowercase(self) -> bool { upper_u32(self as u32) != self as u32 || (self as u32) == 0xDF }
        pub fn to_uppercase(self) -> ToCase { ToCase { c: Some(::core::intrinsics::transmute::<u32, char>(upper_u32(self as u32))) } }
        pub fn to_lowercase(self) -> ToCase { ToCase { c: Some(::core::intrinsics::transmute::<u32, char>(lower_u32(self as u32))) } }
        pub fn is_control(self) -> bool { (self as u32) < 32 || self as u32 == 127 }
        pub fn is_digit(self, radix: u32) -> bool { self.to_digit(radix).is_some() }
        pub fn to_digit(self, radix: u32) -> Option<u32> {
            let u = self as u32;
            let d = if u >= 48 && u <= 57 { u - 48 }
                    else if u >= 97 && u <= 122 { u - 97 + 10 }
                    else if u >= 65 && u <= 90 { u - 65 + 10 }
                    else { return None; };
            if d < radix { Some(d) } else { None }
        }
        pub fn to_ascii_uppercase(&self) -> char {
            if self.is_ascii_lowercase() { ((*self as u8) - 32) as char } else { *self }
        }
        pub fn to_ascii_lowercase(&self) -> char {
            if self.is_ascii_uppercase() { ((*self as u8) + 32) as char } else { *self }
        }
        pub fn eq_ignore_ascii_case(&self, o: &char) -> bool { self.to_ascii_lowercase() == o.to_ascii_lowercase() }
    }
    impl ::core::iter::Step for char {
        fn succ(&self) -> char { ::core::intrinsics::transmute::<u32, char>(*self as u32 + 1) }
        fn pred(&self) -> char { ::core::intrinsics::transmute::<u32, char>(*self as u32 - 1) }
    }
    impl ::core::convert::From<char> for u32 { fn from(c: char) -> u32 { c as u32 } }
    impl ::core::convert::From<u8> for char { fn from(b: u8) -> char { b as char } }
}

// ---------------------------------------------------------------- str

pub mod str {
    use ::core::intrinsics::{elem_read, elem_offset, str_base, str_len, str_from_raw, slice_from_raw, slice_base};
    use ::core::option::Option::{self, Some, None};
    use ::core::result::Result;
    use ::core::string::String;
    use ::core::cmp::Ordering;

    pub trait FromStr: Sized {
        type Err;
        fn from_str(s: &str) -> Result<Self, Self::Err>;
    }

    // the first n bytes of a byte buffer as a str
    pub fn buf_str<'a>(b: &'a [u8], n: usize) -> &'a str { str_from_raw(slice_base::<u8>(b), n) }
    pub fn buf_range<'a>(b: &'a [u8], from: usize, to: usize) -> &'a str {
        str_from_raw(elem_offset::<u8>(slice_base::<u8>(b), from), to - from)
    }
    // bytes [start, end) ; end == usize::MAX means "to the end"
    pub fn sub_str<'a>(s: &'a str, start: usize, end: usize) -> &'a str {
        let len = str_len(s);
        let end = if end == usize::MAX { len } else { end };
        if start > end || end > len {
            ::core::panicking::panic_str("library/core/src/str/mod.rs", 1, 1, "byte index out of range of string slice");
        }
        str_from_raw(str_base(s) + start, end - start)
    }
    fn byte_at(base: usize, i: usize) -> u8 { elem_read::<u8>(base, i) }

    // (code point, byte length) of the character starting at byte `pos`
    fn decode_at(base: usize, pos: usize) -> (u32, usize) {
        let b0 = byte_at(base, pos) as u32;
        if b0 < 0x80 { (b0, 1) }
        else if b0 < 0xE0 { (((b0 & 0x1F) << 6) | (byte_at(base, pos + 1) as u32 & 0x3F), 2) }
        else if b0 < 0xF0 {
            (((b0 & 0x0F) << 12) | ((byte_at(base, pos + 1) as u32 & 0x3F) << 6) | (byte_at(base, pos + 2) as u32 & 0x3F), 3)
        } else {
            (((b0 & 0x07) << 18) | ((byte_at(base, pos + 1) as u32 & 0x3F) << 12)
                | ((byte_at(base, pos + 2) as u32 & 0x3F) << 6) | (byte_at(base, pos + 3) as u32 & 0x3F), 4)
        }
    }
    fn to_char(u: u32) -> char { ::core::intrinsics::transmute::<u32, char>(u) }
    fn is_ws(b: u8) -> bool { b == 32 || (b >= 9 && b <= 13) }

    pub fn str_eq(a: &str, b: &str) -> bool {
        let n = str_len(a);
        if n != str_len(b) { return false; }
        let pa = str_base(a);
        let pb = str_base(b);
        let mut i = 0usize;
        while i < n { if byte_at(pa, i) != byte_at(pb, i) { return false; } i += 1; }
        true
    }
    pub fn str_cmp(a: &str, b: &str) -> Ordering {
        let na = str_len(a);
        let nb = str_len(b);
        let pa = str_base(a);
        let pb = str_base(b);
        let n = if na < nb { na } else { nb };
        let mut i = 0usize;
        while i < n {
            let x = byte_at(pa, i);
            let y = byte_at(pb, i);
            if x < y { return Ordering::Less; }
            if x > y { return Ordering::Greater; }
            i += 1;
        }
        if na < nb { Ordering::Less } else if na == nb { Ordering::Equal } else { Ordering::Greater }
    }
    impl ::core::cmp::PartialEq for str { fn eq(&self, o: &str) -> bool { str_eq(self, o) } }
    impl ::core::cmp::Eq for str {}
    impl ::core::cmp::PartialOrd for str {
        fn partial_cmp(&self, o: &str) -> Option<Ordering> { Some(str_cmp(self, o)) }
    }
    impl ::core::cmp::Ord for str { fn cmp(&self, o: &str) -> Ordering { str_cmp(self, o) } }

    fn find_sub(hay: &str, needle: &str, from: usize) -> Option<(usize, usize)> {
        let n = str_len(hay);
        let m = str_len(needle);
        if m > n { return None; }
        let ph = str_base(hay);
        let pn = str_base(needle);
        let mut i = from;
        while i + m <= n {
            let mut j = 0usize;
            while j < m && byte_at(ph, i + j) == byte_at(pn, j) { j += 1; }
            if j == m { return Some((i, i + m)); }
            i += 1;
        }
        None
    }
    fn is_prefix(hay: &str, needle: &str) -> bool {
        let m = str_len(needle);
        if m > str_len(hay) { return false; }
        str_eq(str_from_raw(str_base(hay), m), needle)
    }
    fn is_suffix(hay: &str, needle: &str) -> bool {
        let m = str_len(needle);
        let n = str_len(hay);
        if m > n { return false; }
        str_eq(str_from_raw(str_base(hay) + (n - m), m), needle)
    }
    fn enc<'a>(c: char, b: &'a mut [u8; 4]) -> &'a str {
        let n = ::core::char::encode_utf8(c, b);
        buf_str(b, n)
    }

    // what str::find / split / contains / starts_with accept: char, &str, &String
    pub trait Pattern {
        fn find_in(&self, hay: &str, from: usize) -> Option<(usize, usize)>;
        fn prefix_len(&self, hay: &str) -> Option<usize>;
        fn suffix_len(&self, hay: &str) -> Option<usize>;
    }
    impl<'a> Pattern for &'a str {
        fn find_in(&self, hay: &str, from: usize) -> Option<(usize, usize)> { find_sub(hay, *self, from) }
        fn prefix_len(&self, hay: &str) -> Option<usize> { if is_prefix(hay, *self) { Some(str_len(*self)) } else { None } }
        fn suffix_len(&self, hay: &str) -> Option<usize> { if is_suffix(hay, *self) { Some(str_len(*self)) } else { None } }
    }
    impl<'a> Pattern for &'a String {
        fn find_in(&self, hay: &str, from: usize) -> Option<(usize, usize)> { find_sub(hay, self.as_str(), from) }
        fn prefix_len(&self, hay: &str) -> Option<usize> { if is_prefix(hay, self.as_str()) { Some(self.len()) } else { None } }
        fn suffix_len(&self, hay: &str) -> Option<usize> { if is_suffix(hay, self.as_str()) { Some(self.len()) } else { None } }
    }
    impl Pattern for char {
        fn find_in(&self, hay: &str, from: usize) -> Option<(usize, usize)> {
            let mut b = [0u8; 4];
            find_sub(hay, enc(*self, &mut b), from)
        }
        fn prefix_len(&self, hay: &str) -> Option<usize> {
            let mut b = [0u8; 4];
            let s = enc(*self, &mut b);
            if is_prefix(hay, s) { Some(str_len(s)) } else { None }
        }
        fn suffix_len(&self, hay: &str) -> Option<usize> {
            let mut b = [0u8; 4];
            let s = enc(*self, &mut b);
            if is_suffix(hay, s) { Some(str_len(s)) } else { None }
        }
    }

    pub struct Chars<'a> { s: &'a str, pos: usize, end: usize }
    impl<'a> ::core::iter::Iterator for Chars<'a> {
        type Item = char;
        fn next(&mut self) -> Option<char> {
            if self.pos >= self.end { return None; }
            let r = decode_at(str_base(self.s), self.pos);
            self.pos += r.1;
            Some(to_char(r.0))
        }
    }
    impl<'a> ::core::iter::DoubleEndedIterator for Chars<'a> {
        fn next_back(&mut self) -> Option<char> {
            if self.pos >= self.end { return None; }
            let base = str_base(self.s);
            let mut p = self.end - 1;
            while p > self.pos && (byte_at(base, p) & 0xC0) == 0x80 { p -= 1; }
            let r = decode_at(base, p);
            self.end = p;
            Some(to_char(r.0))
        }
    }
    impl<'a> Chars<'a> {
        pub fn as_str(&self) -> &'a str { sub_str(self.s, self.pos, self.end) }
    }
    impl<'a> ::core::clone::Clone for Chars<'a> {
        fn clone(&self) -> Chars<'a> { Chars { s: self.s, pos: self.pos, end: self.end } }
    }

    pub struct CharIndices<'a> { s: &'a str, pos: usize, end: usize }
    impl<'a> ::core::iter::Iterator for CharIndices<'a> {
        type Item = (usize, char);
        fn next(&mut self) -> Option<(usize, char)> {
            if self.pos >= self.end { return None; }
            let at = self.pos;
            let r = decode_at(str_base(self.s), at);
            self.pos += r.1;
            Some((at, to_char(r.0)))
        }
    }

    pub struct Bytes<'a> { s: &'a str, pos: usize, end: usize }
    impl<'a> ::core::iter::Iterator for Bytes<'a> {
        type Item = u8;
        fn next(&mut self) -> Option<u8> {
            if self.pos >= self.end { return None; }
            self.pos += 1;
            Some(byte_at(str_base(self.s), self.pos - 1))
        }
    }
    impl<'a> ::core::iter::DoubleEndedIterator for Bytes<'a> {
        fn next_back(&mut self) -> Option<u8> {
            if self.pos >= self.end { return None; }
            self.end -= 1;
            Some(byte_at(str_base(self.s), self.end))
        }
    }

    pub struct Split<'a, P: Pattern> { hay: &'a str, pos: usize, pat: P, done: bool }
    impl<'a, P: Pattern> ::core::iter::Iterator for Split<'a, P> {
        type Item = &'a str;
        fn next(&mut self) -> Option<&'a str> {
            if self.done { return None; }
            let hay: &'a str = self.hay;
            match self.pat.find_in(hay, self.pos) {
                Some(m) => {
                    let r = sub_str(hay, self.pos, m.0);
                    self.pos = m.1;
                    Some(r)
                }
                None => {
                    self.done = true;
                    Some(sub_str(hay, self.pos, usize::MAX))
                }
            }
        }
    }

    pub struct SplitWhitespace<'a> { hay: &'a str, pos: usize }
    impl<'a> ::core::iter::Iterator for SplitWhitespace<'a> {
        type Item = &'a str;
        fn next(&mut self) -> Option<&'a str> {
            let hay: &'a str = self.hay;
            let n = str_len(hay);
            let base = str_base(hay);
            while self.pos < n && is_ws(byte_at(base, self.pos)) { self.pos += 1; }
            if self.pos >= n { return None; }
            let start = self.pos;
            while self.pos < n && !is_ws(byte_at(base, self.pos)) { self.pos += 1; }
            Some(sub_str(hay, start, self.pos))
        }
    }

    pub struct Lines<'a> { hay: &'a str, pos: usize }
    impl<'a> ::core::iter::Iterator for Lines<'a> {
        type Item = &'a str;
        fn next(&mut self) -> Option<&'a str> {
            let hay: &'a str = self.hay;
            let n = str_len(hay);
            if self.pos >= n { return None; }
            let base = str_base(hay);
            let start = self.pos;
            let mut i = start;
            while i < n && byte_at(base, i) != 10 { i += 1; }
            self.pos = i + 1;
            let mut e = i;
            if e > start && byte_at(base, e - 1) == 13 { e -= 1; }
            Some(sub_str(hay, start, e))
        }
    }

    #[lang = "str"]
    impl str {
        pub fn len(&self) -> usize { str_len(self) }
        pub fn is_empty(&self) -> bool { str_len(self) == 0 }
        pub fn as_bytes(&self) -> &[u8] { slice_from_raw::<u8>(str_base(self), str_len(self)) }
        pub fn chars(&self) -> Chars<'_> { Chars { s: self, pos: 0, end: str_len(self) } }
        pub fn char_indices(&self) -> CharIndices<'_> { CharIndices { s: self, pos: 0, end: str_len(self) } }
        pub fn bytes(&self) -> Bytes<'_> { Bytes { s: self, pos: 0, end: str_len(self) } }
        pub fn lines(&self) -> Lines<'_> { Lines { hay: self, pos: 0 } }
        pub fn split_whitespace(&self) -> SplitWhitespace<'_> { SplitWhitespace { hay: self, pos: 0 } }
        pub fn split<P: Pattern>(&self, pat: P) -> Split<'_, P> { Split { hay: self, pos: 0, pat: pat, done: false } }
        pub fn is_char_boundary(&self, i: usize) -> bool {
            let n = str_len(self);
            if i == 0 || i == n { true } else if i > n { false } else { (byte_at(str_base(self), i) & 0xC0) != 0x80 }
        }
        pub fn contains<P: Pattern>(&self, pat: P) -> bool { pat.find_in(self, 0).is_some() }
        pub fn find<P: Pattern>(&self, pat: P) -> Option<usize> {
            match pat.find_in(self, 0) { Some(m) => Some(m.0), None => None }
        }
        pub fn starts_with<P: Pattern>(&self, pat: P) -> bool { pat.prefix_len(self).is_some() }
        pub fn ends_with<P: Pattern>(&self, pat: P) -> bool { pat.suffix_len(self).is_some() }
        pub fn strip_prefix<P: Pattern>(&self, pat: P) -> Option<&str> {
            match pat.prefix_len(self) { Some(n) => Some(sub_str(self, n, usize::MAX)), None => None }
        }
        pub fn strip_suffix<P: Pattern>(&self, pat: P) -> Option<&str> {
            match pat.suffix_len(self) { Some(n) => Some(sub_str(self, 0, str_len(self) - n)), None => None }
        }
        pub fn split_once<P: Pattern>(&self, pat: P) -> Option<(&str, &str)> {
            match pat.find_in(self, 0) {
                Some(m) => Some((sub_str(self, 0, m.0), sub_str(self, m.1, usize::MAX))),
                None => None,
            }
        }
        pub fn split_at(&self, mid: usize) -> (&str, &str) {
            (sub_str(self, 0, mid), sub_str(self, mid, usize::MAX))
        }
        pub fn trim_start(&self) -> &str {
            let n = str_len(self);
            let base = str_base(self);
            let mut i = 0usize;
            while i < n && is_ws(byte_at(base, i)) { i += 1; }
            sub_str(self, i, n)
        }
        pub fn trim_end(&self) -> &str {
            let base = str_base(self);
            let mut e = str_len(self);
            while e > 0 && is_ws(byte_at(base, e - 1)) { e -= 1; }
            sub_str(self, 0, e)
        }
        pub fn trim(&self) -> &str { self.trim_start().trim_end() }
        pub fn parse<F: FromStr>(&self) -> Result<F, F::Err> { F::from_str(self) }
        pub fn to_owned(&self) -> String { String::from(self) }
        pub fn to_ascii_uppercase(&self) -> String {
            let mut s = String::new();
            let mut it = self.chars();
            while let Some(c) = it.next() { s.push(c.to_ascii_uppercase()); }
            s
        }
        pub fn to_ascii_lowercase(&self) -> String {
            let mut s = String::new();
            let mut it = self.chars();
            while let Some(c) = it.next() { s.push(c.to_ascii_lowercase()); }
            s
        }
        pub fn to_uppercase(&self) -> String {
            let mut s = String::new();
            let mut it = self.chars();
            while let Some(c) = it.next() { s.push(::core::intrinsics::transmute::<u32, char>(::core::char::upper_u32(c as u32))); }
            s
        }
        pub fn to_lowercase(&self) -> String {
            let mut s = String::new();
            let mut it = self.chars();
            while let Some(c) = it.next() { s.push(::core::intrinsics::transmute::<u32, char>(::core::char::lower_u32(c as u32))); }
            s
        }
        pub fn eq_ignore_ascii_case(&self, o: &str) -> bool { str_eq(&self.to_ascii_lowercase(), &o.to_ascii_lowercase()) }
        pub fn repeat(&self, n: usize) -> String {
            let mut s = String::new();
            let mut i = 0usize;
            while i < n { s.push_str(self); i += 1; }
            s
        }
        pub fn replace(&self, from: &str, to: &str) -> String {
            let mut s = String::new();
            let mut pos = 0usize;
            let n = str_len(self);
            loop {
                match find_sub(self, from, pos) {
                    Some(m) => {
                        s.push_str(sub_str(self, pos, m.0));
                        s.push_str(to);
                        if m.1 == m.0 {
                            if m.0 >= n { break; }
                            let r = decode_at(str_base(self), m.0);
                            s.push_str(sub_str(self, m.0, m.0 + r.1));
                            pos = m.0 + r.1;
                        } else { pos = m.1; }
                    }
                    None => { s.push_str(sub_str(self, pos, n)); break; }
                }
            }
            s
        }
    }

    // strict UTF-8 validation (no overlongs, no surrogates, max U+10FFFF)
    pub fn is_valid_utf8(v: &[u8]) -> bool {
        let n = v.len();
        let mut i = 0usize;
        while i < n {
            let b = v[i] as u32;
            if b < 0x80 { i += 1; continue; }
            let need: usize;
            let mut lo = 0x80u32;
            let mut hi = 0xBFu32;
            if b >= 0xC2 && b <= 0xDF { need = 1; }
            else if b >= 0xE0 && b <= 0xEF { need = 2; if b == 0xE0 { lo = 0xA0; } if b == 0xED { hi = 0x9F; } }
            else if b >= 0xF0 && b <= 0xF4 { need = 3; if b == 0xF0 { lo = 0x90; } if b == 0xF4 { hi = 0x8F; } }
            else { return false; }
            if i + need >= n { return false; }
            let mut j = 1usize;
            while j <= need {
                let c = v[i + j] as u32;
                if j == 1 { if c < lo || c > hi { return false; } }
                else if c < 0x80 || c > 0xBF { return false; }
                j += 1;
            }
            i += need + 1;
        }
        true
    }
    pub fn from_utf8(v: &[u8]) -> Result<&str, ::core::str::Utf8Error> {
        if !is_valid_utf8(v) { return ::core::result::Result::Err(Utf8Error); }
        ::core::result::Result::Ok(str_from_raw(slice_base::<u8>(v), v.len()))
    }
    pub struct Utf8Error;
    impl ::core::fmt::Debug for Utf8Error {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result { f.write_str("Utf8Error") }
    }
}

// ---------------------------------------------------------------- string

pub mod string {
    use ::core::fmt;
    use ::core::intrinsics::{elem_read, elem_write, str_base, str_from_raw};
    use ::core::option::Option::{self, Some, None};
    use ::core::vec::Vec;

    // a Vec<u8> of UTF-8 bytes; layout (buffer byte address, capacity, length) per design-strings.md
    #[lang = "string"]
    pub struct String { pub vec: Vec<u8> }

    impl String {
        pub fn new() -> String { String { vec: Vec::new() } }
        pub fn with_capacity(n: usize) -> String { String { vec: Vec::with_capacity(n) } }
        pub fn len(&self) -> usize { self.vec.len }
        pub fn is_empty(&self) -> bool { self.vec.len == 0 }
        pub fn capacity(&self) -> usize { self.vec.cap }
        pub fn as_str(&self) -> &str { str_from_raw(self.vec.ptr, self.vec.len) }
        pub fn as_bytes(&self) -> &[u8] { self.vec.as_slice() }
        pub fn into_bytes(self) -> Vec<u8> { self.vec }
        pub fn reserve(&mut self, n: usize) { self.vec.reserve(n); }
        pub fn clear(&mut self) { self.vec.len = 0; }
        pub fn truncate(&mut self, n: usize) { if n < self.vec.len { self.vec.len = n; } }
        pub fn push_str(&mut self, s: &str) {
            let n = s.len();
            self.vec.reserve(n);
            let src = str_base(s);
            let dst = self.vec.ptr;
            let l = self.vec.len;
            let mut i = 0usize;
            while i < n {
                elem_write::<u8>(dst, l + i, elem_read::<u8>(src, i));
                i += 1;
            }
            self.vec.len = l + n;
        }
        pub fn push(&mut self, c: char) {
            let mut b = [0u8; 4];
            let n = ::core::char::encode_utf8(c, &mut b);
            let mut i = 0usize;
            while i < n { self.vec.push(b[i]); i += 1; }
        }
        pub fn pop(&mut self) -> Option<char> {
            let c = match self.as_str().chars().next_back() { Some(c) => c, None => return None };
            self.vec.len -= c.len_utf8();
            Some(c)
        }
        pub fn from_utf8(v: Vec<u8>) -> ::core::result::Result<String, ::core::str::Utf8Error> {
            if !::core::str::is_valid_utf8(&v) { return ::core::result::Result::Err(::core::str::Utf8Error); }
            ::core::result::Result::Ok(String { vec: v })
        }
        fn check_boundary(&self, idx: usize) {
            if !self.as_str().is_char_boundary(idx) {
                ::core::panicking::panic_str("library/alloc/src/string.rs", 1, 1, "assertion failed: self.is_char_boundary(idx)");
            }
        }
        pub fn insert_str(&mut self, idx: usize, s: &str) {
            self.check_boundary(idx);
            let n = s.len();
            let l = self.vec.len;
            self.vec.reserve(n);
            let p = self.vec.ptr;
            let mut i = l;
            while i > idx { i -= 1; elem_write::<u8>(p, i + n, elem_read::<u8>(p, i)); }
            let src = str_base(s);
            let mut j = 0usize;
            while j < n { elem_write::<u8>(p, idx + j, elem_read::<u8>(src, j)); j += 1; }
            self.vec.len = l + n;
        }
        pub fn insert(&mut self, idx: usize, c: char) {
            let mut b = [0u8; 4];
            let n = ::core::char::encode_utf8(c, &mut b);
            self.insert_str(idx, ::core::str::buf_range(&b, 0, n));
        }
        pub fn remove(&mut self, idx: usize) -> char {
            let c = match ::core::str::sub_str(self.as_str(), idx, usize::MAX).chars().next() {
                Some(c) => c,
                None => ::core::panicking::panic_str("library/alloc/src/string.rs", 1, 1, "cannot remove a char from the end of a string"),
            };
            let n = c.len_utf8();
            let p = self.vec.ptr;
            let l = self.vec.len;
            let mut i = idx + n;
            while i < l { elem_write::<u8>(p, i - n, elem_read::<u8>(p, i)); i += 1; }
            self.vec.len = l - n;
            c
        }
        pub fn retain<F: FnMut(char) -> bool>(&mut self, mut f: F) {
            let mut out = String::new();
            {
                let mut it = self.as_str().chars();
                while let Some(c) = it.next() { if f(c) { out.push(c); } }
            }
            *self = out;
        }
        pub fn split_off(&mut self, at: usize) -> String {
            self.check_boundary(at);
            let r = String::from(::core::str::sub_str(self.as_str(), at, usize::MAX));
            self.vec.len = at;
            r
        }
        pub fn drain<R: ::core::ops::RangeArg>(&mut self, r: R) -> Drain {
            let b = r.bounds();
            let l = self.vec.len;
            let end = if b.1 == usize::MAX { l } else { b.1 };
            if b.0 > end || end > l {
                ::core::panicking::panic_str("library/alloc/src/string.rs", 1, 1, "range end index out of range for slice");
            }
            self.check_boundary(b.0);
            self.check_boundary(end);
            let taken = String::from(::core::str::sub_str(self.as_str(), b.0, end));
            let p = self.vec.ptr;
            let mut i = end;
            while i < l { elem_write::<u8>(p, b.0 + (i - end), elem_read::<u8>(p, i)); i += 1; }
            self.vec.len = l - (end - b.0);
            Drain { s: taken, pos: 0 }
        }
        pub fn replace_range<R: ::core::ops::RangeArg>(&mut self, r: R, with: &str) {
            let b = r.bounds();
            let end = if b.1 == usize::MAX { self.vec.len } else { b.1 };
            let tail = self.split_off(end);
            self.truncate(b.0);
            self.push_str(with);
            self.push_str(tail.as_str());
        }
    }
    pub struct Drain { s: String, pos: usize }
    impl ::core::iter::Iterator for Drain {
        type Item = char;
        fn next(&mut self) -> Option<char> {
            let c = match ::core::str::sub_str(self.s.as_str(), self.pos, usize::MAX).chars().next() { Some(c) => c, None => return None };
            self.pos += c.len_utf8();
            Some(c)
        }
    }
    impl ::core::iter::DoubleEndedIterator for Drain {
        fn next_back(&mut self) -> Option<char> {
            if self.pos >= self.s.len() { return None; }
            self.s.pop()
        }
    }
    impl<'a> ::core::iter::FromIterator<&'a char> for String {
        fn from_iter<I: ::core::iter::IntoIterator<Item = &'a char>>(iter: I) -> String {
            let mut s = String::new();
            let mut it = iter.into_iter();
            while let Some(c) = it.next() { s.push(*c); }
            s
        }
    }
    impl<'a> ::core::iter::Extend<&'a char> for String {
        fn extend<I: ::core::iter::IntoIterator<Item = &'a char>>(&mut self, iter: I) {
            let mut it = iter.into_iter();
            while let Some(c) = it.next() { self.push(*c); }
        }
    }
    impl ::core::iter::Extend<String> for String {
        fn extend<I: ::core::iter::IntoIterator<Item = String>>(&mut self, iter: I) {
            let mut it = iter.into_iter();
            while let Some(c) = it.next() { self.push_str(c.as_str()); }
        }
    }
    impl ::core::ops::Deref for String {
        type Target = str;
        fn deref(&self) -> &str { self.as_str() }
    }
    impl ::core::clone::Clone for String {
        fn clone(&self) -> String { String { vec: self.vec.clone() } }
    }
    impl ::core::default::Default for String { fn default() -> String { String::new() } }
    impl fmt::Display for String {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { fmt::Display::fmt(self.as_str(), f) }
    }
    impl fmt::Debug for String {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { fmt::Debug::fmt(self.as_str(), f) }
    }
    impl fmt::Write for String {
        fn write_str(&mut self, s: &str) -> fmt::Result { self.push_str(s); ::core::result::Result::Ok(()) }
        fn write_char(&mut self, c: char) -> fmt::Result { self.push(c); ::core::result::Result::Ok(()) }
    }
    impl ::core::cmp::PartialEq for String { fn eq(&self, o: &String) -> bool { ::core::str::str_eq(self.as_str(), o.as_str()) } }
    impl ::core::cmp::PartialEq<str> for String { fn eq(&self, o: &str) -> bool { ::core::str::str_eq(self.as_str(), o) } }
    impl<'a> ::core::cmp::PartialEq<&'a str> for String { fn eq(&self, o: &&'a str) -> bool { ::core::str::str_eq(self.as_str(), *o) } }
    impl ::core::cmp::PartialEq<String> for str { fn eq(&self, o: &String) -> bool { ::core::str::str_eq(self, o.as_str()) } }
    impl<'a> ::core::cmp::PartialEq<String> for &'a str { fn eq(&self, o: &String) -> bool { ::core::str::str_eq(*self, o.as_str()) } }
    impl ::core::cmp::Eq for String {}
    impl ::core::cmp::PartialOrd for String {
        fn partial_cmp(&self, o: &String) -> Option<::core::cmp::Ordering> { Some(::core::str::str_cmp(self.as_str(), o.as_str())) }
    }
    impl ::core::cmp::Ord for String {
        fn cmp(&self, o: &String) -> ::core::cmp::Ordering { ::core::str::str_cmp(self.as_str(), o.as_str()) }
    }
    impl<'a> ::core::convert::From<&'a str> for String {
        fn from(s: &'a str) -> String { let mut r = String::with_capacity(s.len()); r.push_str(s); r }
    }
    impl<'a> ::core::convert::From<&'a String> for String {
        fn from(s: &'a String) -> String { s.clone() }
    }
    impl<'a> ::core::convert::From<&'a mut str> for String {
        fn from(s: &'a mut str) -> String { String::from(&*s) }
    }
    impl ::core::convert::From<char> for String {
        fn from(c: char) -> String { let mut r = String::new(); r.push(c); r }
    }
    impl<'a> ::core::ops::Add<&'a str> for String {
        type Output = String;
        fn add(mut self, o: &'a str) -> String { self.push_str(o); self }
    }
    impl<'a> ::core::ops::AddAssign<&'a str> for String {
        fn add_assign(&mut self, o: &'a str) { self.push_str(o); }
    }
    impl ::core::convert::AsRef<str> for String { fn as_ref(&self) -> &str { self.as_str() } }
    impl ::core::convert::AsRef<[u8]> for String { fn as_ref(&self) -> &[u8] { self.as_bytes() } }
    impl ::core::iter::FromIterator<char> for String {
        fn from_iter<I: ::core::iter::IntoIterator<Item = char>>(iter: I) -> String {
            let mut s = String::new();
            let mut it = iter.into_iter();
            while let Some(c) = it.next() { s.push(c); }
            s
        }
    }
    impl<'a> ::core::iter::FromIterator<&'a str> for String {
        fn from_iter<I: ::core::iter::IntoIterator<Item = &'a str>>(iter: I) -> String {
            let mut s = String::new();
            let mut it = iter.into_iter();
            while let Some(c) = it.next() { s.push_str(c); }
            s
        }
    }
    impl ::core::iter::FromIterator<String> for String {
        fn from_iter<I: ::core::iter::IntoIterator<Item = String>>(iter: I) -> String {
            let mut s = String::new();
            let mut it = iter.into_iter();
            while let Some(c) = it.next() { s.push_str(c.as_str()); }
            s
        }
    }

    pub trait ToString {
        fn to_string(&self) -> String;
    }
    impl<T: fmt::Display + ?Sized> ToString for T {
        fn to_string(&self) -> String {
            let mut s = String::new();
            {
                let mut f = fmt::Formatter::new(&mut s);
                let _ = fmt::Display::fmt(self, &mut f);
            }
            s
        }
    }
}

// ---------------------------------------------------------------- iter

pub mod iter {
    use ::core::cmp::Ordering;
    use ::core::marker::PhantomData;
    use ::core::option::Option::{self, Some, None};
    use ::core::result::Result::{self, Ok, Err};
    use ::core::string::String;
    use ::core::vec::Vec;

    // `for pat in expr { body }` lowers to
    //   { let mut __it = IntoIterator::into_iter(expr);
    //     loop { match Iterator::next(&mut __it) { Some(pat) => body, None => break } } }
    #[lang = "iterator"]
    pub trait Iterator {
        type Item;
        fn next(&mut self) -> Option<Self::Item>;

        fn map<B, F: FnMut(Self::Item) -> B>(self, f: F) -> Map<Self, F> where Self: Sized { Map { iter: self, f: f } }
        fn filter<P: FnMut(&Self::Item) -> bool>(self, p: P) -> Filter<Self, P> where Self: Sized { Filter { iter: self, p: p } }
        fn filter_map<B, F: FnMut(Self::Item) -> Option<B>>(self, f: F) -> FilterMap<Self, F> where Self: Sized {
            FilterMap { iter: self, f: f }
        }
        fn enumerate(self) -> Enumerate<Self> where Self: Sized { Enumerate { iter: self, count: 0 } }
        fn zip<U: IntoIterator>(self, other: U) -> Zip<Self, U::IntoIter> where Self: Sized {
            Zip { a: self, b: other.into_iter() }
        }
        fn chain<U: IntoIterator<Item = Self::Item>>(self, other: U) -> Chain<Self, U::IntoIter> where Self: Sized {
            Chain { a: self, b: other.into_iter(), a_done: false }
        }
        fn take(self, n: usize) -> Take<Self> where Self: Sized { Take { iter: self, n: n } }
        fn skip(self, n: usize) -> Skip<Self> where Self: Sized { Skip { iter: self, n: n } }
        fn step_by(self, step: usize) -> StepBy<Self> where Self: Sized { StepBy { iter: self, step: step, first: true } }
        fn take_while<P: FnMut(&Self::Item) -> bool>(self, p: P) -> TakeWhile<Self, P> where Self: Sized {
            TakeWhile { iter: self, p: p, done: false }
        }
        fn skip_while<P: FnMut(&Self::Item) -> bool>(self, p: P) -> SkipWhile<Self, P> where Self: Sized {
            SkipWhile { iter: self, p: p, started: false }
        }
        fn rev(self) -> Rev<Self> where Self: Sized + DoubleEndedIterator { Rev { iter: self } }
        fn peekable(self) -> Peekable<Self> where Self: Sized { Peekable { iter: self, peeked: None } }
        fn cloned<'a, T: Clone>(self) -> Cloned<Self> where Self: Sized + Iterator<Item = &'a T> { Cloned { iter: self } }
        fn copied<'a, T: Copy>(self) -> Copied<Self> where Self: Sized + Iterator<Item = &'a T> { Copied { iter: self } }
        fn by_ref(&mut self) -> &mut Self where Self: Sized { self }

        fn count(self) -> usize where Self: Sized {
            let mut it = self;
            let mut n = 0usize;
            while let Some(_) = it.next() { n += 1; }
            n
        }
        fn last(self) -> Option<Self::Item> where Self: Sized {
            let mut it = self;
            let mut r = None;
            while let Some(x) = it.next() { r = Some(x); }
            r
        }
        fn nth(&mut self, n: usize) -> Option<Self::Item> {
            let mut i = 0usize;
            while i < n {
                if self.next().is_none() { return None; }
                i += 1;
            }
            self.next()
        }
        fn fold<B, F: FnMut(B, Self::Item) -> B>(self, init: B, f: F) -> B where Self: Sized {
            let mut it = self;
            let mut g = f;
            let mut acc = init;
            while let Some(x) = it.next() { acc = g(acc, x); }
            acc
        }
        fn for_each<F: FnMut(Self::Item)>(self, f: F) where Self: Sized {
            let mut it = self;
            let mut g = f;
            while let Some(x) = it.next() { g(x); }
        }
        fn any<F: FnMut(Self::Item) -> bool>(&mut self, f: F) -> bool {
            let mut g = f;
            while let Some(x) = self.next() { if g(x) { return true; } }
            false
        }
        fn all<F: FnMut(Self::Item) -> bool>(&mut self, f: F) -> bool {
            let mut g = f;
            while let Some(x) = self.next() { if !g(x) { return false; } }
            true
        }
        fn find<P: FnMut(&Self::Item) -> bool>(&mut self, p: P) -> Option<Self::Item> {
            let mut g = p;
            while let Some(x) = self.next() { if g(&x) { return Some(x); } }
            None
        }
        fn find_map<B, F: FnMut(Self::Item) -> Option<B>>(&mut self, f: F) -> Option<B> {
            let mut g = f;
            while let Some(x) = self.next() {
                match g(x) { Some(y) => return Some(y), None => {} }
            }
            None
        }
        fn position<P: FnMut(Self::Item) -> bool>(&mut self, p: P) -> Option<usize> {
            let mut g = p;
            let mut i = 0usize;
            while let Some(x) = self.next() { if g(x) { return Some(i); } i += 1; }
            None
        }
        fn max(self) -> Option<Self::Item> where Self: Sized, Self::Item: Ord {
            let mut it = self;
            let mut best = match it.next() { Some(x) => x, None => return None };
            while let Some(x) = it.next() { if x.cmp(&best) != Ordering::Less { best = x; } }
            Some(best)
        }
        fn min(self) -> Option<Self::Item> where Self: Sized, Self::Item: Ord {
            let mut it = self;
            let mut best = match it.next() { Some(x) => x, None => return None };
            while let Some(x) = it.next() { if x.cmp(&best) == Ordering::Less { best = x; } }
            Some(best)
        }
        fn max_by_key<B: Ord, F: FnMut(&Self::Item) -> B>(self, f: F) -> Option<Self::Item> where Self: Sized {
            let mut it = self;
            let mut g = f;
            let mut best = match it.next() { Some(x) => x, None => return None };
            let mut bk = g(&best);
            while let Some(x) = it.next() {
                let k = g(&x);
                if k.cmp(&bk) != Ordering::Less { best = x; bk = k; }
            }
            Some(best)
        }
        fn min_by_key<B: Ord, F: FnMut(&Self::Item) -> B>(self, f: F) -> Option<Self::Item> where Self: Sized {
            let mut it = self;
            let mut g = f;
            let mut best = match it.next() { Some(x) => x, None => return None };
            let mut bk = g(&best);
            while let Some(x) = it.next() {
                let k = g(&x);
                if k.cmp(&bk) == Ordering::Less { best = x; bk = k; }
            }
            Some(best)
        }
        fn max_by<F: FnMut(&Self::Item, &Self::Item) -> Ordering>(self, f: F) -> Option<Self::Item> where Self: Sized {
            let mut it = self;
            let mut g = f;
            let mut best = match it.next() { Some(x) => x, None => return None };
            while let Some(x) = it.next() { if g(&x, &best) != Ordering::Less { best = x; } }
            Some(best)
        }
        fn min_by<F: FnMut(&Self::Item, &Self::Item) -> Ordering>(self, f: F) -> Option<Self::Item> where Self: Sized {
            let mut it = self;
            let mut g = f;
            let mut best = match it.next() { Some(x) => x, None => return None };
            while let Some(x) = it.next() { if g(&x, &best) == Ordering::Less { best = x; } }
            Some(best)
        }
        fn rposition<P: FnMut(Self::Item) -> bool>(&mut self, p: P) -> Option<usize> where Self: Sized {
            let mut g = p;
            let mut i = 0usize;
            let mut r: Option<usize> = None;
            while let Some(x) = self.next() { if g(x) { r = Some(i); } i += 1; }
            r
        }
        fn reduce<F: FnMut(Self::Item, Self::Item) -> Self::Item>(self, f: F) -> Option<Self::Item> where Self: Sized {
            let mut it = self;
            let mut g = f;
            let mut acc = match it.next() { Some(x) => x, None => return None };
            while let Some(x) = it.next() { acc = g(acc, x); }
            Some(acc)
        }
        fn inspect<F: FnMut(&Self::Item)>(self, f: F) -> Inspect<Self, F> where Self: Sized { Inspect { iter: self, f: f } }
        fn scan<St, B, F: FnMut(&mut St, Self::Item) -> Option<B>>(self, init: St, f: F) -> Scan<Self, St, F> where Self: Sized {
            Scan { iter: self, state: init, f: f }
        }
        fn flat_map<U: IntoIterator, F: FnMut(Self::Item) -> U>(self, f: F) -> FlatMap<Self, U, F> where Self: Sized {
            FlatMap { iter: self, f: f, cur: None }
        }
        fn flatten(self) -> Flatten<Self> where Self: Sized, Self::Item: IntoIterator {
            Flatten { iter: self, cur: None }
        }
        fn cycle(self) -> Cycle<Self> where Self: Sized + Clone { Cycle { orig: self.clone(), cur: self } }
        fn partition<B: Default + Extend<Self::Item>, F: FnMut(&Self::Item) -> bool>(self, f: F) -> (B, B) where Self: Sized {
            let mut a: B = Default::default();
            let mut b: B = Default::default();
            let mut it = self;
            let mut g = f;
            while let Some(x) = it.next() {
                if g(&x) { a.extend(once(x)); } else { b.extend(once(x)); }
            }
            (a, b)
        }
        fn unzip<A, B, FromA: Default + Extend<A>, FromB: Default + Extend<B>>(self) -> (FromA, FromB) where Self: Sized + Iterator<Item = (A, B)> {
            let mut a: FromA = Default::default();
            let mut b: FromB = Default::default();
            let mut it = self;
            while let Some(x) = it.next() {
                a.extend(once(x.0));
                b.extend(once(x.1));
            }
            (a, b)
        }
        fn sum<S: Sum<Self::Item>>(self) -> S where Self: Sized { S::sum(self) }
        fn product<P: Product<Self::Item>>(self) -> P where Self: Sized { P::product(self) }
        fn collect<B: FromIterator<Self::Item>>(self) -> B where Self: Sized { B::from_iter(self) }
    }

    #[lang = "double_ended_iterator"]
    pub trait DoubleEndedIterator: Iterator {
        fn next_back(&mut self) -> Option<Self::Item>;
    }

    // `for` loops and `.zip(..)` / `.chain(..)` call into_iter on their argument
    #[lang = "into_iterator"]
    pub trait IntoIterator {
        type Item;
        type IntoIter: Iterator<Item = Self::Item>;
        fn into_iter(self) -> Self::IntoIter;
    }
    impl<I: Iterator> IntoIterator for I {
        type Item = I::Item;
        type IntoIter = I;
        fn into_iter(self) -> I { self }
    }

    #[lang = "from_iterator"]
    pub trait FromIterator<A>: Sized {
        fn from_iter<I: IntoIterator<Item = A>>(iter: I) -> Self;
    }
    pub trait Extend<A> {
        fn extend<I: IntoIterator<Item = A>>(&mut self, iter: I);
    }
    pub trait Sum<A = Self>: Sized {
        fn sum<I: Iterator<Item = A>>(iter: I) -> Self;
    }
    pub trait Product<A = Self>: Sized {
        fn product<I: Iterator<Item = A>>(iter: I) -> Self;
    }
    // integer-like types usable in ranges
    pub trait Step: Clone + PartialOrd {
        fn succ(&self) -> Self;
        fn pred(&self) -> Self;
    }

    impl<'a, I: Iterator + ?Sized> Iterator for &'a mut I {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> { (**self).next() }
    }
    impl<'a, I: DoubleEndedIterator + ?Sized> DoubleEndedIterator for &'a mut I {
        fn next_back(&mut self) -> Option<I::Item> { (**self).next_back() }
    }

    impl<A: Step> Iterator for ::core::ops::Range<A> {
        type Item = A;
        fn next(&mut self) -> Option<A> {
            if self.start < self.end {
                let n = self.start.succ();
                Some(::core::mem::replace(&mut self.start, n))
            } else { None }
        }
    }
    impl<A: Step> DoubleEndedIterator for ::core::ops::Range<A> {
        fn next_back(&mut self) -> Option<A> {
            if self.start < self.end {
                self.end = self.end.pred();
                Some(self.end.clone())
            } else { None }
        }
    }
    impl<A: Step> Iterator for ::core::ops::RangeInclusive<A> {
        type Item = A;
        fn next(&mut self) -> Option<A> {
            if self.exhausted || !(self.start <= self.end) { None }
            else if self.start < self.end {
                let n = self.start.succ();
                Some(::core::mem::replace(&mut self.start, n))
            } else {
                self.exhausted = true;
                Some(self.start.clone())
            }
        }
    }
    impl<A: Step> DoubleEndedIterator for ::core::ops::RangeInclusive<A> {
        fn next_back(&mut self) -> Option<A> {
            if self.exhausted || !(self.start <= self.end) { None }
            else if self.start < self.end {
                let n = self.end.pred();
                Some(::core::mem::replace(&mut self.end, n))
            } else {
                self.exhausted = true;
                Some(self.end.clone())
            }
        }
    }
    impl<A: Step> Iterator for ::core::ops::RangeFrom<A> {
        type Item = A;
        fn next(&mut self) -> Option<A> {
            let n = self.start.succ();
            Some(::core::mem::replace(&mut self.start, n))
        }
    }

    pub struct Map<I, F> { iter: I, f: F }
    impl<B, I: Iterator, F: FnMut(I::Item) -> B> Iterator for Map<I, F> {
        type Item = B;
        fn next(&mut self) -> Option<B> {
            match self.iter.next() { Some(x) => Some((self.f)(x)), None => None }
        }
    }
    impl<B, I: DoubleEndedIterator, F: FnMut(I::Item) -> B> DoubleEndedIterator for Map<I, F> {
        fn next_back(&mut self) -> Option<B> {
            match self.iter.next_back() { Some(x) => Some((self.f)(x)), None => None }
        }
    }

    pub struct Filter<I, P> { iter: I, p: P }
    impl<I: Iterator, P: FnMut(&I::Item) -> bool> Iterator for Filter<I, P> {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> {
            loop {
                match self.iter.next() {
                    Some(x) => { if (self.p)(&x) { return Some(x); } }
                    None => return None,
                }
            }
        }
    }
    impl<I: DoubleEndedIterator, P: FnMut(&I::Item) -> bool> DoubleEndedIterator for Filter<I, P> {
        fn next_back(&mut self) -> Option<I::Item> {
            loop {
                match self.iter.next_back() {
                    Some(x) => { if (self.p)(&x) { return Some(x); } }
                    None => return None,
                }
            }
        }
    }

    pub struct FilterMap<I, F> { iter: I, f: F }
    impl<B, I: Iterator, F: FnMut(I::Item) -> Option<B>> Iterator for FilterMap<I, F> {
        type Item = B;
        fn next(&mut self) -> Option<B> {
            loop {
                match self.iter.next() {
                    Some(x) => { match (self.f)(x) { Some(y) => return Some(y), None => {} } }
                    None => return None,
                }
            }
        }
    }

    pub struct Enumerate<I> { iter: I, count: usize }
    impl<I: Iterator> Iterator for Enumerate<I> {
        type Item = (usize, I::Item);
        fn next(&mut self) -> Option<(usize, I::Item)> {
            match self.iter.next() {
                Some(x) => { let i = self.count; self.count += 1; Some((i, x)) }
                None => None,
            }
        }
    }

    pub struct Zip<A, B> { a: A, b: B }
    impl<A: Iterator, B: Iterator> Iterator for Zip<A, B> {
        type Item = (A::Item, B::Item);
        fn next(&mut self) -> Option<(A::Item, B::Item)> {
            match self.a.next() {
                Some(x) => match self.b.next() { Some(y) => Some((x, y)), None => None },
                None => None,
            }
        }
    }

    pub struct Chain<A, B> { a: A, b: B, a_done: bool }
    impl<A: Iterator, B: Iterator<Item = A::Item>> Iterator for Chain<A, B> {
        type Item = A::Item;
        fn next(&mut self) -> Option<A::Item> {
            if !self.a_done {
                match self.a.next() { Some(x) => return Some(x), None => { self.a_done = true; } }
            }
            self.b.next()
        }
    }
    impl<A: DoubleEndedIterator, B: DoubleEndedIterator<Item = A::Item>> DoubleEndedIterator for Chain<A, B> {
        fn next_back(&mut self) -> Option<A::Item> {
            match self.b.next_back() { Some(x) => return Some(x), None => {} }
            if self.a_done { None } else { self.a.next_back() }
        }
    }

    pub struct Take<I> { iter: I, n: usize }
    impl<I: Iterator> Iterator for Take<I> {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> {
            if self.n == 0 { None } else { self.n -= 1; self.iter.next() }
        }
    }

    pub struct Skip<I> { iter: I, n: usize }
    impl<I: Iterator> Iterator for Skip<I> {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> {
            while self.n > 0 {
                self.n -= 1;
                if self.iter.next().is_none() { return None; }
            }
            self.iter.next()
        }
    }

    pub struct StepBy<I> { iter: I, step: usize, first: bool }
    impl<I: Iterator> Iterator for StepBy<I> {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> {
            if self.first { self.first = false; self.iter.next() } else { self.iter.nth(self.step - 1) }
        }
    }

    pub struct TakeWhile<I, P> { iter: I, p: P, done: bool }
    impl<I: Iterator, P: FnMut(&I::Item) -> bool> Iterator for TakeWhile<I, P> {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> {
            if self.done { return None; }
            match self.iter.next() {
                Some(x) => { if (self.p)(&x) { Some(x) } else { self.done = true; None } }
                None => None,
            }
        }
    }

    pub struct SkipWhile<I, P> { iter: I, p: P, started: bool }
    impl<I: Iterator, P: FnMut(&I::Item) -> bool> Iterator for SkipWhile<I, P> {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> {
            loop {
                match self.iter.next() {
                    Some(x) => {
                        if self.started || !(self.p)(&x) { self.started = true; return Some(x); }
                    }
                    None => return None,
                }
            }
        }
    }

    pub struct Rev<I> { iter: I }
    impl<I: DoubleEndedIterator> Iterator for Rev<I> {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> { self.iter.next_back() }
    }
    impl<I: DoubleEndedIterator> DoubleEndedIterator for Rev<I> {
        fn next_back(&mut self) -> Option<I::Item> { self.iter.next() }
    }

    pub struct Peekable<I: Iterator> { iter: I, peeked: Option<Option<I::Item>> }
    impl<I: Iterator> Peekable<I> {
        pub fn peek(&mut self) -> Option<&I::Item> {
            if self.peeked.is_none() { self.peeked = Some(self.iter.next()); }
            match self.peeked { Some(Some(ref x)) => Some(x), _ => None }
        }
    }
    impl<I: Iterator> Iterator for Peekable<I> {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> {
            match self.peeked.take() { Some(v) => v, None => self.iter.next() }
        }
    }

    pub struct Cloned<I> { iter: I }
    impl<I: Clone> Clone for Cloned<I> { fn clone(&self) -> Cloned<I> { Cloned { iter: self.iter.clone() } } }
    impl<I: Clone> Clone for Copied<I> { fn clone(&self) -> Copied<I> { Copied { iter: self.iter.clone() } } }
    impl<I: Clone> Clone for Rev<I> { fn clone(&self) -> Rev<I> { Rev { iter: self.iter.clone() } } }
    impl<I: Clone, F: Clone> Clone for Map<I, F> { fn clone(&self) -> Map<I, F> { Map { iter: self.iter.clone(), f: self.f.clone() } } }
    impl<I: Clone> Clone for Enumerate<I> { fn clone(&self) -> Enumerate<I> { Enumerate { iter: self.iter.clone(), count: self.count } } }
    impl<I: Clone> Clone for Take<I> { fn clone(&self) -> Take<I> { Take { iter: self.iter.clone(), n: self.n } } }

    pub struct Inspect<I, F> { iter: I, f: F }
    impl<I: Iterator, F: FnMut(&I::Item)> Iterator for Inspect<I, F> {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> {
            match self.iter.next() { Some(x) => { (self.f)(&x); Some(x) } None => None }
        }
    }
    pub struct Scan<I, St, F> { iter: I, state: St, f: F }
    impl<B, I: Iterator, St, F: FnMut(&mut St, I::Item) -> Option<B>> Iterator for Scan<I, St, F> {
        type Item = B;
        fn next(&mut self) -> Option<B> {
            match self.iter.next() { Some(x) => (self.f)(&mut self.state, x), None => None }
        }
    }
    pub struct FlatMap<I, U: IntoIterator, F> { iter: I, f: F, cur: Option<U::IntoIter> }
    impl<I: Iterator, U: IntoIterator, F: FnMut(I::Item) -> U> Iterator for FlatMap<I, U, F> {
        type Item = U::Item;
        fn next(&mut self) -> Option<U::Item> {
            loop {
                match self.cur.as_mut() {
                    Some(c) => { match c.next() { Some(x) => return Some(x), None => {} } }
                    None => {}
                }
                match self.iter.next() {
                    Some(x) => { self.cur = Some((self.f)(x).into_iter()); }
                    None => return None,
                }
            }
        }
    }
    pub struct Flatten<I: Iterator> where I::Item: IntoIterator { iter: I, cur: Option<<I::Item as IntoIterator>::IntoIter> }
    impl<I: Iterator> Iterator for Flatten<I> where I::Item: IntoIterator {
        type Item = <I::Item as IntoIterator>::Item;
        fn next(&mut self) -> Option<<I::Item as IntoIterator>::Item> {
            loop {
                match self.cur.as_mut() {
                    Some(c) => { match c.next() { Some(x) => return Some(x), None => {} } }
                    None => {}
                }
                match self.iter.next() {
                    Some(x) => { self.cur = Some(x.into_iter()); }
                    None => return None,
                }
            }
        }
    }
    pub struct Cycle<I> { orig: I, cur: I }
    impl<I: Iterator + Clone> Iterator for Cycle<I> {
        type Item = I::Item;
        fn next(&mut self) -> Option<I::Item> {
            match self.cur.next() {
                Some(x) => Some(x),
                None => { self.cur = self.orig.clone(); self.cur.next() }
            }
        }
    }
    impl<T> IntoIterator for Option<T> {
        type Item = T;
        type IntoIter = Once<T>;
        fn into_iter(self) -> Once<T> { Once { v: self } }
    }
    impl<'a, T: Clone, I: Iterator<Item = &'a T>> Iterator for Cloned<I> {
        type Item = T;
        fn next(&mut self) -> Option<T> { match self.iter.next() { Some(x) => Some(x.clone()), None => None } }
    }
    impl<'a, T: Clone, I: DoubleEndedIterator<Item = &'a T>> DoubleEndedIterator for Cloned<I> {
        fn next_back(&mut self) -> Option<T> { match self.iter.next_back() { Some(x) => Some(x.clone()), None => None } }
    }
    pub struct Copied<I> { iter: I }
    impl<'a, T: Copy, I: Iterator<Item = &'a T>> Iterator for Copied<I> {
        type Item = T;
        fn next(&mut self) -> Option<T> { match self.iter.next() { Some(x) => Some(*x), None => None } }
    }
    impl<'a, T: Copy, I: DoubleEndedIterator<Item = &'a T>> DoubleEndedIterator for Copied<I> {
        fn next_back(&mut self) -> Option<T> { match self.iter.next_back() { Some(x) => Some(*x), None => None } }
    }

    pub struct Once<T> { v: Option<T> }
    pub fn once<T>(v: T) -> Once<T> { Once { v: Some(v) } }
    impl<T> Iterator for Once<T> {
        type Item = T;
        fn next(&mut self) -> Option<T> { self.v.take() }
    }
    pub struct Repeat<T> { v: T }
    pub fn repeat<T: Clone>(v: T) -> Repeat<T> { Repeat { v: v } }
    impl<T: Clone> Iterator for Repeat<T> {
        type Item = T;
        fn next(&mut self) -> Option<T> { Some(self.v.clone()) }
    }

    impl<T> FromIterator<T> for Vec<T> {
        fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> Vec<T> {
            let mut v = Vec::new();
            let mut it = iter.into_iter();
            while let Some(x) = it.next() { v.push(x); }
            v
        }
    }
    impl<A, E, V: FromIterator<A>> FromIterator<Result<A, E>> for Result<V, E> {
        fn from_iter<I: IntoIterator<Item = Result<A, E>>>(iter: I) -> Result<V, E> {
            let mut v: Vec<A> = Vec::new();
            let mut it = iter.into_iter();
            loop {
                match it.next() {
                    Some(Ok(x)) => v.push(x),
                    Some(Err(e)) => return Err(e),
                    None => break,
                }
            }
            Ok(V::from_iter(v))
        }
    }
    impl<A, V: FromIterator<A>> FromIterator<Option<A>> for Option<V> {
        fn from_iter<I: IntoIterator<Item = Option<A>>>(iter: I) -> Option<V> {
            let mut v: Vec<A> = Vec::new();
            let mut it = iter.into_iter();
            loop {
                match it.next() {
                    Some(Some(x)) => v.push(x),
                    Some(None) => return None,
                    None => break,
                }
            }
            Some(V::from_iter(v))
        }
    }
    impl<T> Extend<T> for Vec<T> {
        fn extend<I: IntoIterator<Item = T>>(&mut self, iter: I) {
            let mut it = iter.into_iter();
            while let Some(x) = it.next() { self.push(x); }
        }
    }
    impl<'a, T: Copy + 'a> Extend<&'a T> for Vec<T> {
        fn extend<I: IntoIterator<Item = &'a T>>(&mut self, iter: I) {
            let mut it = iter.into_iter();
            while let Some(x) = it.next() { self.push(*x); }
        }
    }
    impl Extend<char> for String {
        fn extend<I: IntoIterator<Item = char>>(&mut self, iter: I) {
            let mut it = iter.into_iter();
            while let Some(c) = it.next() { self.push(c); }
        }
    }
    impl<'a> Extend<&'a str> for String {
        fn extend<I: IntoIterator<Item = &'a str>>(&mut self, iter: I) {
            let mut it = iter.into_iter();
            while let Some(c) = it.next() { self.push_str(c); }
        }
    }
}

// ---------------------------------------------------------------- slice

pub mod slice {
    use ::core::cmp::Ordering;
    use ::core::intrinsics::{elem_alloc, elem_free, elem_offset, elem_read, elem_ref, elem_ref_mut, elem_write, slice_base, slice_from_raw, slice_from_raw_mut, slice_len};
    use ::core::marker::PhantomData;
    use ::core::option::Option::{self, Some, None};
    use ::core::string::String;
    use ::core::vec::Vec;

    // elements [start, end) ; end == usize::MAX means "to the end"
    pub fn sub_slice<'a, T>(s: &'a [T], start: usize, end: usize) -> &'a [T] {
        let len = slice_len::<T>(s);
        let end = if end == usize::MAX { len } else { end };
        if start > end || end > len {
            ::core::panicking::panic_str("library/core/src/slice/mod.rs", 1, 1, "slice index out of range");
        }
        slice_from_raw::<T>(elem_offset::<T>(slice_base::<T>(s), start), end - start)
    }
    pub fn sub_slice_mut<'a, T>(s: &'a mut [T], start: usize, end: usize) -> &'a mut [T] {
        let len = slice_len::<T>(s);
        let end = if end == usize::MAX { len } else { end };
        if start > end || end > len {
            ::core::panicking::panic_str("library/core/src/slice/mod.rs", 1, 1, "slice index out of range");
        }
        slice_from_raw_mut::<T>(elem_offset::<T>(slice_base::<T>(s), start), end - start)
    }

    pub struct Iter<'a, T> { base: usize, pos: usize, end: usize, m: PhantomData<&'a T> }
    impl<'a, T> ::core::iter::Iterator for Iter<'a, T> {
        type Item = &'a T;
        fn next(&mut self) -> Option<&'a T> {
            if self.pos < self.end { let r = elem_ref::<T>(self.base, self.pos); self.pos += 1; Some(r) } else { None }
        }
    }
    impl<'a, T> ::core::iter::DoubleEndedIterator for Iter<'a, T> {
        fn next_back(&mut self) -> Option<&'a T> {
            if self.pos < self.end { self.end -= 1; Some(elem_ref::<T>(self.base, self.end)) } else { None }
        }
    }
    impl<'a, T> ::core::clone::Clone for Iter<'a, T> {
        fn clone(&self) -> Iter<'a, T> { Iter { base: self.base, pos: self.pos, end: self.end, m: PhantomData } }
    }
    pub struct IterMut<'a, T> { base: usize, pos: usize, end: usize, m: PhantomData<&'a mut T> }
    impl<'a, T> ::core::iter::Iterator for IterMut<'a, T> {
        type Item = &'a mut T;
        fn next(&mut self) -> Option<&'a mut T> {
            if self.pos < self.end { let r = elem_ref_mut::<T>(self.base, self.pos); self.pos += 1; Some(r) } else { None }
        }
    }
    impl<'a, T> ::core::iter::DoubleEndedIterator for IterMut<'a, T> {
        fn next_back(&mut self) -> Option<&'a mut T> {
            if self.pos < self.end { self.end -= 1; Some(elem_ref_mut::<T>(self.base, self.end)) } else { None }
        }
    }

    pub struct Windows<'a, T> { s: &'a [T], size: usize, pos: usize }
    impl<'a, T> ::core::iter::Iterator for Windows<'a, T> {
        type Item = &'a [T];
        fn next(&mut self) -> Option<&'a [T]> {
            let s: &'a [T] = self.s;
            if self.pos + self.size > slice_len::<T>(s) { return None; }
            let r = sub_slice(s, self.pos, self.pos + self.size);
            self.pos += 1;
            Some(r)
        }
    }
    pub struct Chunks<'a, T> { s: &'a [T], size: usize, pos: usize }
    impl<'a, T> ::core::iter::Iterator for Chunks<'a, T> {
        type Item = &'a [T];
        fn next(&mut self) -> Option<&'a [T]> {
            let s: &'a [T] = self.s;
            let n = slice_len::<T>(s);
            if self.pos >= n { return None; }
            let e = if self.pos + self.size < n { self.pos + self.size } else { n };
            let r = sub_slice(s, self.pos, e);
            self.pos = e;
            Some(r)
        }
    }

    pub struct ChunksExact<'a, T> { s: &'a [T], size: usize, pos: usize }
    impl<'a, T> ::core::iter::Iterator for ChunksExact<'a, T> {
        type Item = &'a [T];
        fn next(&mut self) -> Option<&'a [T]> {
            let s: &'a [T] = self.s;
            if self.pos + self.size > slice_len::<T>(s) { return None; }
            let r = sub_slice(s, self.pos, self.pos + self.size);
            self.pos += self.size;
            Some(r)
        }
    }
    impl<'a, T> ChunksExact<'a, T> {
        pub fn remainder(&self) -> &'a [T] {
            let s: &'a [T] = self.s;
            let n = slice_len::<T>(s);
            sub_slice(s, n - n % self.size, n)
        }
    }

    // stable bottom-up merge sort moving elements bitwise between two buffers
    fn merge_sort<T, F: FnMut(&T, &T) -> Ordering>(base: usize, n: usize, mut compare: F) {
        if n < 2 { return; }
        let tmp = elem_alloc::<T>(n);
        let mut src = base;
        let mut dst = tmp;
        let mut width = 1usize;
        while width < n {
            let mut lo = 0usize;
            while lo < n {
                let mid = if lo + width < n { lo + width } else { n };
                let hi = if lo + 2 * width < n { lo + 2 * width } else { n };
                let mut i = lo;
                let mut j = mid;
                let mut k = lo;
                while i < mid && j < hi {
                    if compare(elem_ref::<T>(src, j), elem_ref::<T>(src, i)) == Ordering::Less {
                        elem_write::<T>(dst, k, elem_read::<T>(src, j));
                        j += 1;
                    } else {
                        elem_write::<T>(dst, k, elem_read::<T>(src, i));
                        i += 1;
                    }
                    k += 1;
                }
                while i < mid { elem_write::<T>(dst, k, elem_read::<T>(src, i)); i += 1; k += 1; }
                while j < hi { elem_write::<T>(dst, k, elem_read::<T>(src, j)); j += 1; k += 1; }
                lo += 2 * width;
            }
            let t = src;
            src = dst;
            dst = t;
            width = width * 2;
        }
        if src != base {
            let mut i = 0usize;
            while i < n { elem_write::<T>(base, i, elem_read::<T>(src, i)); i += 1; }
        }
        elem_free::<T>(tmp);
    }

    pub fn slice_eq<A: ::core::cmp::PartialEq<B>, B>(a: &[A], b: &[B]) -> bool {
        let n = slice_len::<A>(a);
        if n != slice_len::<B>(b) { return false; }
        let pa = slice_base::<A>(a);
        let pb = slice_base::<B>(b);
        let mut i = 0usize;
        while i < n {
            if !(*elem_ref::<A>(pa, i) == *elem_ref::<B>(pb, i)) { return false; }
            i += 1;
        }
        true
    }
    pub fn slice_cmp<T: ::core::cmp::Ord>(a: &[T], b: &[T]) -> Ordering {
        let na = slice_len::<T>(a);
        let nb = slice_len::<T>(b);
        let pa = slice_base::<T>(a);
        let pb = slice_base::<T>(b);
        let n = if na < nb { na } else { nb };
        let mut i = 0usize;
        while i < n {
            let c = elem_ref::<T>(pa, i).cmp(elem_ref::<T>(pb, i));
            if c != Ordering::Equal { return c; }
            i += 1;
        }
        na.cmp(&nb)
    }

    impl<T> [T] {
        pub fn len(&self) -> usize { slice_len::<T>(self) }
        pub fn is_empty(&self) -> bool { slice_len::<T>(self) == 0 }
        pub fn iter(&self) -> Iter<'_, T> { Iter { base: slice_base::<T>(self), pos: 0, end: slice_len::<T>(self), m: PhantomData } }
        pub fn iter_mut(&mut self) -> IterMut<'_, T> {
            IterMut { base: slice_base::<T>(self), pos: 0, end: slice_len::<T>(self), m: PhantomData }
        }
        pub fn get(&self, i: usize) -> Option<&T> {
            if i < slice_len::<T>(self) { Some(elem_ref::<T>(slice_base::<T>(self), i)) } else { None }
        }
        pub fn get_mut(&mut self, i: usize) -> Option<&mut T> {
            if i < slice_len::<T>(self) { Some(elem_ref_mut::<T>(slice_base::<T>(self), i)) } else { None }
        }
        pub fn first(&self) -> Option<&T> { self.get(0) }
        pub fn last(&self) -> Option<&T> {
            let n = slice_len::<T>(self);
            if n == 0 { None } else { self.get(n - 1) }
        }
        pub fn first_mut(&mut self) -> Option<&mut T> { self.get_mut(0) }
        pub fn last_mut(&mut self) -> Option<&mut T> {
            let n = slice_len::<T>(self);
            if n == 0 { None } else { self.get_mut(n - 1) }
        }
        pub fn contains(&self, x: &T) -> bool where T: ::core::cmp::PartialEq {
            let mut it = self.iter();
            while let Some(y) = it.next() { if *y == *x { return true; } }
            false
        }
        pub fn swap(&mut self, a: usize, b: usize) {
            let n = slice_len::<T>(self);
            if a >= n { ::core::panicking::panic_bounds_check("library/core/src/slice/mod.rs", 1, 1, a, n); }
            if b >= n { ::core::panicking::panic_bounds_check("library/core/src/slice/mod.rs", 1, 1, b, n); }
            let base = slice_base::<T>(self);
            let x = elem_read::<T>(base, a);
            let y = elem_read::<T>(base, b);
            elem_write::<T>(base, a, y);
            elem_write::<T>(base, b, x);
        }
        pub fn reverse(&mut self) {
            let n = slice_len::<T>(self);
            let mut i = 0usize;
            while i < n / 2 { self.swap(i, n - 1 - i); i += 1; }
        }
        pub fn windows(&self, size: usize) -> Windows<'_, T> { Windows { s: self, size: size, pos: 0 } }
        pub fn chunks(&self, size: usize) -> Chunks<'_, T> { Chunks { s: self, size: size, pos: 0 } }
        pub fn split_at(&self, mid: usize) -> (&[T], &[T]) { (sub_slice(self, 0, mid), sub_slice(self, mid, usize::MAX)) }
        pub fn split_at_mut(&mut self, mid: usize) -> (&mut [T], &mut [T]) {
            let n = slice_len::<T>(self);
            if mid > n {
                ::core::panicking::panic_str("library/core/src/slice/mod.rs", 1, 1, "mid > len");
            }
            let b = slice_base::<T>(self);
            (slice_from_raw_mut::<T>(b, mid), slice_from_raw_mut::<T>(elem_offset::<T>(b, mid), n - mid))
        }
        pub fn split_first(&self) -> Option<(&T, &[T])> {
            if slice_len::<T>(self) == 0 { None } else { Some((self.get(0).unwrap(), sub_slice(self, 1, usize::MAX))) }
        }
        pub fn split_last(&self) -> Option<(&T, &[T])> {
            let n = slice_len::<T>(self);
            if n == 0 { None } else { Some((self.get(n - 1).unwrap(), sub_slice(self, 0, n - 1))) }
        }
        pub fn rotate_left(&mut self, k: usize) {
            let n = slice_len::<T>(self);
            if k > n { ::core::panicking::panic_str("library/core/src/slice/mod.rs", 1, 1, "assertion failed: mid <= self.len()"); }
            sub_slice_mut(self, 0, k).reverse();
            sub_slice_mut(self, k, usize::MAX).reverse();
            self.reverse();
        }
        pub fn rotate_right(&mut self, k: usize) {
            let n = slice_len::<T>(self);
            if k > n { ::core::panicking::panic_str("library/core/src/slice/mod.rs", 1, 1, "assertion failed: k <= self.len()"); }
            self.rotate_left(n - k);
        }
        pub fn fill(&mut self, v: T) where T: ::core::clone::Clone {
            let n = slice_len::<T>(self);
            let b = slice_base::<T>(self);
            let mut i = 0usize;
            while i < n { *elem_ref_mut::<T>(b, i) = v.clone(); i += 1; }
        }
        pub fn chunks_exact(&self, size: usize) -> ChunksExact<'_, T> {
            if size == 0 { ::core::panicking::panic_str("library/core/src/slice/mod.rs", 1, 1, "chunk size must be non-zero"); }
            ChunksExact { s: self, size: size, pos: 0 }
        }
        pub fn starts_with(&self, p: &[T]) -> bool where T: ::core::cmp::PartialEq {
            let n = slice_len::<T>(p);
            n <= slice_len::<T>(self) && slice_eq::<T, T>(sub_slice(self, 0, n), p)
        }
        pub fn ends_with(&self, p: &[T]) -> bool where T: ::core::cmp::PartialEq {
            let n = slice_len::<T>(p);
            let l = slice_len::<T>(self);
            n <= l && slice_eq::<T, T>(sub_slice(self, l - n, l), p)
        }
        pub fn to_vec(&self) -> Vec<T> where T: ::core::clone::Clone {
            let mut v = Vec::with_capacity(slice_len::<T>(self));
            let mut it = self.iter();
            while let Some(x) = it.next() { v.push(x.clone()); }
            v
        }
        pub fn to_owned(&self) -> Vec<T> where T: ::core::clone::Clone { self.to_vec() }
        pub fn sort_by<F: FnMut(&T, &T) -> Ordering>(&mut self, compare: F) {
            merge_sort::<T, F>(slice_base::<T>(self), slice_len::<T>(self), compare);
        }
        pub fn sort(&mut self) where T: ::core::cmp::Ord { self.sort_by(|a, b| a.cmp(b)); }
        pub fn sort_unstable(&mut self) where T: ::core::cmp::Ord { self.sort_by(|a, b| a.cmp(b)); }
        pub fn sort_unstable_by<F: FnMut(&T, &T) -> Ordering>(&mut self, compare: F) { self.sort_by(compare); }
        pub fn sort_by_key<K: ::core::cmp::Ord, F: FnMut(&T) -> K>(&mut self, mut f: F) {
            self.sort_by(|a, b| f(a).cmp(&f(b)));
        }
        pub fn sort_unstable_by_key<K: ::core::cmp::Ord, F: FnMut(&T) -> K>(&mut self, mut f: F) {
            self.sort_by(|a, b| f(a).cmp(&f(b)));
        }
        pub fn binary_search(&self, x: &T) -> ::core::result::Result<usize, usize> where T: ::core::cmp::Ord {
            let mut lo = 0usize;
            let mut hi = slice_len::<T>(self);
            while lo < hi {
                let mid = lo + (hi - lo) / 2;
                match self.get(mid).unwrap().cmp(x) {
                    Ordering::Less => { lo = mid + 1; }
                    Ordering::Greater => { hi = mid; }
                    Ordering::Equal => return ::core::result::Result::Ok(mid),
                }
            }
            ::core::result::Result::Err(lo)
        }
    }
    impl<T: JoinItem> [T] {
        pub fn join(&self, sep: &<T as JoinItem>::Sep) -> <T as JoinItem>::Out { <T as JoinItem>::join_items(self, sep) }
        pub fn concat(&self) -> <T as JoinItem>::Out { <T as JoinItem>::concat_items(self) }
    }
    // element types that [T]::join / concat accept (Strings and Vecs)
    pub trait JoinItem: Sized {
        type Out;
        type Sep: ?Sized;
        fn join_items(s: &[Self], sep: &Self::Sep) -> Self::Out;
        fn concat_items(s: &[Self]) -> Self::Out;
    }
    fn join_strs<S: ::core::convert::AsRef<str>>(s: &[S], sep: &str) -> String {
        let mut r = String::new();
        let mut i = 0usize;
        let n = slice_len::<S>(s);
        while i < n {
            if i > 0 { r.push_str(sep); }
            r.push_str(s.get(i).unwrap().as_ref());
            i += 1;
        }
        r
    }
    impl JoinItem for String {
        type Out = String;
        type Sep = str;
        fn join_items(s: &[String], sep: &str) -> String { join_strs::<String>(s, sep) }
        fn concat_items(s: &[String]) -> String { join_strs::<String>(s, "") }
    }
    impl<'a> JoinItem for &'a str {
        type Out = String;
        type Sep = str;
        fn join_items(s: &[&'a str], sep: &str) -> String { join_strs::<&'a str>(s, sep) }
        fn concat_items(s: &[&'a str]) -> String { join_strs::<&'a str>(s, "") }
    }
    impl<'a> JoinItem for &'a String {
        type Out = String;
        type Sep = str;
        fn join_items(s: &[&'a String], sep: &str) -> String { join_strs::<&'a String>(s, sep) }
        fn concat_items(s: &[&'a String]) -> String { join_strs::<&'a String>(s, "") }
    }
    impl<U: ::core::clone::Clone> JoinItem for Vec<U> {
        type Out = Vec<U>;
        type Sep = U;
        fn join_items(s: &[Vec<U>], sep: &U) -> Vec<U> {
            let mut r: Vec<U> = Vec::new();
            let mut i = 0usize;
            let n = slice_len::<Vec<U>>(s);
            while i < n {
                if i > 0 { r.push(sep.clone()); }
                r.extend_from_slice(s.get(i).unwrap().as_slice());
                i += 1;
            }
            r
        }
        fn concat_items(s: &[Vec<U>]) -> Vec<U> {
            let mut r: Vec<U> = Vec::new();
            let mut i = 0usize;
            let n = slice_len::<Vec<U>>(s);
            while i < n { r.extend_from_slice(s.get(i).unwrap().as_slice()); i += 1; }
            r
        }
    }
    impl<'a, U: ::core::clone::Clone> JoinItem for &'a [U] {
        type Out = Vec<U>;
        type Sep = U;
        fn join_items(s: &[&'a [U]], sep: &U) -> Vec<U> {
            let mut r: Vec<U> = Vec::new();
            let mut i = 0usize;
            let n = slice_len::<&'a [U]>(s);
            while i < n {
                if i > 0 { r.push(sep.clone()); }
                r.extend_from_slice(*s.get(i).unwrap());
                i += 1;
            }
            r
        }
        fn concat_items(s: &[&'a [U]]) -> Vec<U> {
            let mut r: Vec<U> = Vec::new();
            let mut i = 0usize;
            let n = slice_len::<&'a [U]>(s);
            while i < n { r.extend_from_slice(*s.get(i).unwrap()); i += 1; }
            r
        }
    }

    impl<A: ::core::cmp::PartialEq<B>, B> ::core::cmp::PartialEq<[B]> for [A] {
        fn eq(&self, o: &[B]) -> bool { slice_eq(self, o) }
    }
    impl<T: ::core::cmp::Eq> ::core::cmp::Eq for [T] {}
    impl<T: ::core::cmp::PartialOrd> ::core::cmp::PartialOrd for [T] {
        fn partial_cmp(&self, o: &[T]) -> Option<Ordering> {
            let na = slice_len::<T>(self);
            let nb = slice_len::<T>(o);
            let n = if na < nb { na } else { nb };
            let mut i = 0usize;
            while i < n {
                match self.get(i).unwrap().partial_cmp(o.get(i).unwrap()) {
                    Some(Ordering::Equal) => {}
                    other => return other,
                }
                i += 1;
            }
            Some(na.cmp(&nb))
        }
    }
    impl<T: ::core::cmp::Ord> ::core::cmp::Ord for [T] {
        fn cmp(&self, o: &[T]) -> Ordering { slice_cmp(self, o) }
    }
    impl<T: ::core::fmt::Debug> ::core::fmt::Debug for [T] {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
            f.debug_list().entries(self.iter()).finish()
        }
    }
    impl<T, const N: usize> ::core::fmt::Debug for [T; N] where T: ::core::fmt::Debug {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
            let s: &[T] = self;
            ::core::fmt::Debug::fmt(s, f)
        }
    }
    impl<A: ::core::cmp::PartialEq<B>, B, const N: usize> ::core::cmp::PartialEq<[B; N]> for [A; N] {
        fn eq(&self, o: &[B; N]) -> bool { let a: &[A] = self; let b: &[B] = o; slice_eq(a, b) }
    }
    impl<T: ::core::cmp::Eq, const N: usize> ::core::cmp::Eq for [T; N] {}
    impl<T: ::core::cmp::PartialOrd, const N: usize> ::core::cmp::PartialOrd for [T; N] {
        fn partial_cmp(&self, o: &[T; N]) -> Option<Ordering> {
            let a: &[T] = self;
            let b: &[T] = o;
            ::core::cmp::PartialOrd::partial_cmp(a, b)
        }
    }
    impl<T: ::core::cmp::Ord, const N: usize> ::core::cmp::Ord for [T; N] {
        fn cmp(&self, o: &[T; N]) -> Ordering {
            let a: &[T] = self;
            let b: &[T] = o;
            slice_cmp(a, b)
        }
    }
    impl<T: ::core::default::Default + ::core::marker::Copy, const N: usize> ::core::default::Default for [T; N] {
        fn default() -> [T; N] { [T::default(); N] }
    }
    impl<'a, T> ::core::iter::IntoIterator for &'a [T] {
        type Item = &'a T;
        type IntoIter = Iter<'a, T>;
        fn into_iter(self) -> Iter<'a, T> { self.iter() }
    }
    impl<'a, T> ::core::iter::IntoIterator for &'a mut [T] {
        type Item = &'a mut T;
        type IntoIter = IterMut<'a, T>;
        fn into_iter(self) -> IterMut<'a, T> { self.iter_mut() }
    }
    impl<'a, T, const N: usize> ::core::iter::IntoIterator for &'a [T; N] {
        type Item = &'a T;
        type IntoIter = Iter<'a, T>;
        fn into_iter(self) -> Iter<'a, T> { let s: &[T] = self; s.iter() }
    }
    impl<'a, T, const N: usize> ::core::iter::IntoIterator for &'a mut [T; N] {
        type Item = &'a mut T;
        type IntoIter = IterMut<'a, T>;
        fn into_iter(self) -> IterMut<'a, T> { let s: &mut [T] = self; s.iter_mut() }
    }
    impl<T, const N: usize> ::core::iter::IntoIterator for [T; N] {
        type Item = T;
        type IntoIter = ::core::vec::IntoIter<T>;
        fn into_iter(self) -> ::core::vec::IntoIter<T> { Vec::from_array(self).into_iter() }
    }
}

// ---------------------------------------------------------------- vec

pub mod vec {
    use ::core::intrinsics::{elem_alloc, elem_free, elem_read, elem_realloc, elem_ref, elem_ref_mut, elem_write, slice_base, slice_from_raw, slice_from_raw_mut};
    use ::core::marker::PhantomData;
    use ::core::option::Option::{self, Some, None};

    // layout (element base, capacity, length); for Vec<u8> the base is a byte address
    #[lang = "vec"]
    pub struct Vec<T> { pub ptr: usize, pub cap: usize, pub len: usize }

    impl<T> Vec<T> {
        pub fn new() -> Vec<T> { Vec { ptr: 0, cap: 0, len: 0 } }
        pub fn with_capacity(n: usize) -> Vec<T> {
            if n == 0 { Vec { ptr: 0, cap: 0, len: 0 } } else { Vec { ptr: elem_alloc::<T>(n), cap: n, len: 0 } }
        }
        // vec![a, b, c]
        pub fn from_array<const N: usize>(a: [T; N]) -> Vec<T> {
            if N == 0 { ::core::mem::forget(a); return Vec { ptr: 0, cap: 0, len: 0 }; }
            let base = elem_alloc::<T>(N);
            let src: &[T] = &a;
            let sb = slice_base::<T>(src);
            let mut i = 0usize;
            while i < N { elem_write::<T>(base, i, elem_read::<T>(sb, i)); i += 1; }
            ::core::mem::forget(a);
            Vec { ptr: base, cap: N, len: N }
        }
        pub fn len(&self) -> usize { self.len }
        pub fn is_empty(&self) -> bool { self.len == 0 }
        pub fn capacity(&self) -> usize { self.cap }
        pub fn as_slice(&self) -> &[T] { slice_from_raw::<T>(self.ptr, self.len) }
        pub fn as_mut_slice(&mut self) -> &mut [T] { slice_from_raw_mut::<T>(self.ptr, self.len) }
        fn grow(&mut self, min: usize) {
            let mut nc = if self.cap == 0 { 4 } else { self.cap * 2 };
            if nc < min { nc = min; }
            self.ptr = if self.cap == 0 { elem_alloc::<T>(nc) } else { elem_realloc::<T>(self.ptr, self.cap, nc) };
            self.cap = nc;
        }
        pub fn reserve(&mut self, extra: usize) {
            if self.len + extra > self.cap { self.grow(self.len + extra); }
        }
        pub fn push(&mut self, v: T) {
            if self.len == self.cap { self.grow(self.len + 1); }
            elem_write::<T>(self.ptr, self.len, v);
            self.len += 1;
        }
        pub fn pop(&mut self) -> Option<T> {
            if self.len == 0 { None } else { self.len -= 1; Some(elem_read::<T>(self.ptr, self.len)) }
        }
        pub fn get(&self, i: usize) -> Option<&T> {
            if i < self.len { Some(elem_ref::<T>(self.ptr, i)) } else { None }
        }
        pub fn get_mut(&mut self, i: usize) -> Option<&mut T> {
            if i < self.len { Some(elem_ref_mut::<T>(self.ptr, i)) } else { None }
        }
        pub fn iter(&self) -> ::core::slice::Iter<'_, T> { self.as_slice().iter() }
        pub fn iter_mut(&mut self) -> ::core::slice::IterMut<'_, T> { self.as_mut_slice().iter_mut() }
        pub fn truncate(&mut self, n: usize) {
            while self.len > n { self.len -= 1; let _x = elem_read::<T>(self.ptr, self.len); }
        }
        pub fn clear(&mut self) { self.truncate(0); }
        pub fn insert(&mut self, idx: usize, v: T) {
            if idx > self.len {
                ::core::panicking::panic_str("library/alloc/src/vec/mod.rs", 1, 1, "insertion index is out of bounds");
            }
            if self.len == self.cap { self.grow(self.len + 1); }
            let mut i = self.len;
            while i > idx { elem_write::<T>(self.ptr, i, elem_read::<T>(self.ptr, i - 1)); i -= 1; }
            elem_write::<T>(self.ptr, idx, v);
            self.len += 1;
        }
        pub fn remove(&mut self, idx: usize) -> T {
            if idx >= self.len {
                ::core::panicking::panic_str("library/alloc/src/vec/mod.rs", 1, 1, "removal index is out of bounds");
            }
            let v = elem_read::<T>(self.ptr, idx);
            let mut i = idx;
            while i + 1 < self.len { elem_write::<T>(self.ptr, i, elem_read::<T>(self.ptr, i + 1)); i += 1; }
            self.len -= 1;
            v
        }
        pub fn swap_remove(&mut self, idx: usize) -> T {
            if idx >= self.len {
                ::core::panicking::panic_str("library/alloc/src/vec/mod.rs", 1, 1, "swap_remove index is out of bounds");
            }
            let v = elem_read::<T>(self.ptr, idx);
            self.len -= 1;
            if idx < self.len { elem_write::<T>(self.ptr, idx, elem_read::<T>(self.ptr, self.len)); }
            v
        }
        pub fn retain<F: FnMut(&T) -> bool>(&mut self, mut f: F) {
            let n = self.len;
            let mut r = 0usize;
            let mut w = 0usize;
            while r < n {
                if f(elem_ref::<T>(self.ptr, r)) {
                    if w != r { elem_write::<T>(self.ptr, w, elem_read::<T>(self.ptr, r)); }
                    w += 1;
                } else {
                    let _x = elem_read::<T>(self.ptr, r);
                }
                r += 1;
            }
            self.len = w;
        }
        pub fn append(&mut self, other: &mut Vec<T>) {
            self.reserve(other.len);
            let mut i = 0usize;
            while i < other.len { self.push(elem_read::<T>(other.ptr, i)); i += 1; }
            other.len = 0;
        }
        pub fn extend_from_slice(&mut self, s: &[T]) where T: ::core::clone::Clone {
            let mut it = s.iter();
            while let Some(x) = it.next() { self.push(x.clone()); }
        }
        pub fn resize(&mut self, n: usize, v: T) where T: ::core::clone::Clone {
            if n < self.len { self.truncate(n); }
            else { while self.len < n { self.push(v.clone()); } }
        }
        pub fn dedup(&mut self) where T: ::core::cmp::PartialEq {
            if self.len < 2 { return; }
            let n = self.len;
            let mut w = 1usize;
            let mut r = 1usize;
            while r < n {
                if *elem_ref::<T>(self.ptr, r) == *elem_ref::<T>(self.ptr, w - 1) {
                    let _x = elem_read::<T>(self.ptr, r);
                } else {
                    if w != r { elem_write::<T>(self.ptr, w, elem_read::<T>(self.ptr, r)); }
                    w += 1;
                }
                r += 1;
            }
            self.len = w;
        }
        pub fn dedup_by<F: FnMut(&mut T, &mut T) -> bool>(&mut self, mut same: F) {
            if self.len < 2 { return; }
            let n = self.len;
            let mut w = 1usize;
            let mut r = 1usize;
            while r < n {
                if same(elem_ref_mut::<T>(self.ptr, r), elem_ref_mut::<T>(self.ptr, w - 1)) {
                    let _x = elem_read::<T>(self.ptr, r);
                } else {
                    if w != r { elem_write::<T>(self.ptr, w, elem_read::<T>(self.ptr, r)); }
                    w += 1;
                }
                r += 1;
            }
            self.len = w;
        }
        pub fn dedup_by_key<K: ::core::cmp::PartialEq, F: FnMut(&mut T) -> K>(&mut self, mut key: F) {
            self.dedup_by(|a, b| key(a) == key(b));
        }
        pub fn split_off(&mut self, at: usize) -> Vec<T> {
            if at > self.len {
                ::core::panicking::panic_str("library/alloc/src/vec/mod.rs", 1, 1, "`at` split index out of bounds");
            }
            let mut r: Vec<T> = Vec::with_capacity(self.len - at);
            let mut i = at;
            while i < self.len { r.push(elem_read::<T>(self.ptr, i)); i += 1; }
            self.len = at;
            r
        }
        pub fn drain<R: ::core::ops::RangeArg>(&mut self, range: R) -> IntoIter<T> {
            let b = range.bounds();
            let l = self.len;
            let end = if b.1 == usize::MAX { l } else { b.1 };
            if b.0 > end || end > l {
                ::core::panicking::panic_str("library/alloc/src/vec/mod.rs", 1, 1, "range end index out of range for slice");
            }
            let mut taken: Vec<T> = Vec::with_capacity(end - b.0);
            let mut i = b.0;
            while i < end { taken.push(elem_read::<T>(self.ptr, i)); i += 1; }
            let mut j = end;
            while j < l { elem_write::<T>(self.ptr, b.0 + (j - end), elem_read::<T>(self.ptr, j)); j += 1; }
            self.len = l - (end - b.0);
            taken.into_iter()
        }
    }

    // vec![x; n]
    pub fn from_elem<T: ::core::clone::Clone>(x: T, n: usize) -> Vec<T> {
        let mut v = Vec::with_capacity(n);
        if n > 0 {
            let mut i = 1usize;
            while i < n { v.push(x.clone()); i += 1; }
            v.push(x);
        }
        v
    }

    impl<T> ::core::ops::Drop for Vec<T> {
        fn drop(&mut self) {
            let mut i = 0usize;
            while i < self.len { let _x = elem_read::<T>(self.ptr, i); i += 1; }
            if self.cap > 0 { elem_free::<T>(self.ptr); }
        }
    }
    impl<T> ::core::ops::Deref for Vec<T> {
        type Target = [T];
        fn deref(&self) -> &[T] { slice_from_raw::<T>(self.ptr, self.len) }
    }
    impl<T> ::core::ops::DerefMut for Vec<T> {
        fn deref_mut(&mut self) -> &mut [T] { slice_from_raw_mut::<T>(self.ptr, self.len) }
    }
    impl<T> ::core::ops::Index<usize> for Vec<T> {
        type Output = T;
        fn index(&self, i: usize) -> &T {
            if i >= self.len { ::core::panicking::panic_bounds_check("library/alloc/src/vec/mod.rs", 1, 1, i, self.len); }
            elem_ref::<T>(self.ptr, i)
        }
    }
    impl<T> ::core::ops::IndexMut<usize> for Vec<T> {
        fn index_mut(&mut self, i: usize) -> &mut T {
            if i >= self.len { ::core::panicking::panic_bounds_check("library/alloc/src/vec/mod.rs", 1, 1, i, self.len); }
            elem_ref_mut::<T>(self.ptr, i)
        }
    }
    impl<T: ::core::clone::Clone> ::core::clone::Clone for Vec<T> {
        fn clone(&self) -> Vec<T> {
            let mut v = Vec::with_capacity(self.len);
            let mut i = 0usize;
            while i < self.len { v.push(elem_ref::<T>(self.ptr, i).clone()); i += 1; }
            v
        }
    }
    impl<T> ::core::default::Default for Vec<T> { fn default() -> Vec<T> { Vec::new() } }
    impl<T: ::core::fmt::Debug> ::core::fmt::Debug for Vec<T> {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result { ::core::fmt::Debug::fmt(self.as_slice(), f) }
    }
    impl<A: ::core::cmp::PartialEq<B>, B> ::core::cmp::PartialEq<Vec<B>> for Vec<A> {
        fn eq(&self, o: &Vec<B>) -> bool { ::core::slice::slice_eq(self.as_slice(), o.as_slice()) }
    }
    impl<A: ::core::cmp::PartialEq<B>, B, const N: usize> ::core::cmp::PartialEq<[B; N]> for Vec<A> {
        fn eq(&self, o: &[B; N]) -> bool { let b: &[B] = o; ::core::slice::slice_eq(self.as_slice(), b) }
    }
    impl<'a, A: ::core::cmp::PartialEq<B>, B> ::core::cmp::PartialEq<&'a [B]> for Vec<A> {
        fn eq(&self, o: &&'a [B]) -> bool { ::core::slice::slice_eq(self.as_slice(), *o) }
    }
    impl<T: ::core::cmp::Eq> ::core::cmp::Eq for Vec<T> {}
    impl<T: ::core::cmp::PartialOrd> ::core::cmp::PartialOrd for Vec<T> {
        fn partial_cmp(&self, o: &Vec<T>) -> Option<::core::cmp::Ordering> { self.as_slice().partial_cmp(o.as_slice()) }
    }
    impl<T: ::core::cmp::Ord> ::core::cmp::Ord for Vec<T> {
        fn cmp(&self, o: &Vec<T>) -> ::core::cmp::Ordering { ::core::slice::slice_cmp(self.as_slice(), o.as_slice()) }
    }
    impl<T> ::core::convert::AsRef<[T]> for Vec<T> { fn as_ref(&self) -> &[T] { self.as_slice() } }
    impl<'a, T: ::core::clone::Clone> ::core::convert::From<&'a [T]> for Vec<T> {
        fn from(s: &'a [T]) -> Vec<T> { s.to_vec() }
    }
    impl<'a> ::core::convert::From<&'a str> for Vec<u8> {
        fn from(s: &'a str) -> Vec<u8> { s.as_bytes().to_vec() }
    }
    impl<T, const N: usize> ::core::convert::From<[T; N]> for Vec<T> {
        fn from(a: [T; N]) -> Vec<T> { Vec::from_array(a) }
    }

    pub struct IntoIter<T> { buf: usize, cap: usize, pos: usize, end: usize }
    impl<T> ::core::iter::Iterator for IntoIter<T> {
        type Item = T;
        fn next(&mut self) -> Option<T> {
            if self.pos < self.end { let v = elem_read::<T>(self.buf, self.pos); self.pos += 1; Some(v) } else { None }
        }
    }
    impl<T> ::core::iter::DoubleEndedIterator for IntoIter<T> {
        fn next_back(&mut self) -> Option<T> {
            if self.pos < self.end { self.end -= 1; Some(elem_read::<T>(self.buf, self.end)) } else { None }
        }
    }
    impl<T> ::core::ops::Drop for IntoIter<T> {
        fn drop(&mut self) {
            while self.pos < self.end { let _x = elem_read::<T>(self.buf, self.pos); self.pos += 1; }
            if self.cap > 0 { elem_free::<T>(self.buf); }
        }
    }
    impl<T> ::core::iter::IntoIterator for Vec<T> {
        type Item = T;
        type IntoIter = IntoIter<T>;
        fn into_iter(self) -> IntoIter<T> {
            let it = IntoIter { buf: self.ptr, cap: self.cap, pos: 0, end: self.len };
            ::core::mem::forget(self);
            it
        }
    }
    impl<'a, T> ::core::iter::IntoIterator for &'a Vec<T> {
        type Item = &'a T;
        type IntoIter = ::core::slice::Iter<'a, T>;
        fn into_iter(self) -> ::core::slice::Iter<'a, T> { self.as_slice().iter() }
    }
    impl<'a, T> ::core::iter::IntoIterator for &'a mut Vec<T> {
        type Item = &'a mut T;
        type IntoIter = ::core::slice::IterMut<'a, T>;
        fn into_iter(self) -> ::core::slice::IterMut<'a, T> { self.as_mut_slice().iter_mut() }
    }
}

// ---------------------------------------------------------------- boxed, error

pub mod boxed {
    use ::core::intrinsics::{box_deref, box_deref_mut, box_drop, box_new};

    // The layout of Box<T> is chosen by the code generator (one word, the heap address,
    // for sized T; address plus vtable or length for dyn / slice / str); `ptr` is a placeholder.
    // Deref, DerefMut, drop and `*b` move-out are built on the box_* intrinsics.
    #[lang = "owned_box"]
    pub struct Box<T: ?Sized> { ptr: usize }

    impl<T> Box<T> {
        pub fn new(x: T) -> Box<T> { box_new::<T>(x) }
    }
    impl<T: ?Sized> ::core::convert::AsRef<T> for Box<T> {
        fn as_ref(&self) -> &T { box_deref::<T>(self) }
    }
    impl<T: ?Sized> ::core::convert::AsMut<T> for Box<T> {
        fn as_mut(&mut self) -> &mut T { box_deref_mut::<T>(self) }
    }
    impl<T: ?Sized> ::core::ops::Deref for Box<T> {
        type Target = T;
        fn deref(&self) -> &T { box_deref::<T>(self) }
    }
    impl<T: ?Sized> ::core::ops::DerefMut for Box<T> {
        fn deref_mut(&mut self) -> &mut T { box_deref_mut::<T>(self) }
    }
    impl<T: ?Sized> ::core::ops::Drop for Box<T> {
        fn drop(&mut self) { box_drop::<T>(self) }
    }
    impl<T: ::core::clone::Clone> ::core::clone::Clone for Box<T> {
        fn clone(&self) -> Box<T> { Box::new((**self).clone()) }
    }
    impl<T: ::core::default::Default> ::core::default::Default for Box<T> {
        fn default() -> Box<T> { Box::new(T::default()) }
    }
    impl<T> ::core::convert::From<T> for Box<T> { fn from(x: T) -> Box<T> { Box::new(x) } }
    impl<T: ::core::fmt::Display + ?Sized> ::core::fmt::Display for Box<T> {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result { ::core::fmt::Display::fmt(&**self, f) }
    }
    impl<T: ::core::fmt::Debug + ?Sized> ::core::fmt::Debug for Box<T> {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result { ::core::fmt::Debug::fmt(&**self, f) }
    }
    impl<T: ::core::cmp::PartialEq + ?Sized> ::core::cmp::PartialEq for Box<T> {
        fn eq(&self, o: &Box<T>) -> bool { **self == **o }
    }
    impl<T: ::core::cmp::Eq + ?Sized> ::core::cmp::Eq for Box<T> {}
    impl<T: ::core::cmp::PartialOrd + ?Sized> ::core::cmp::PartialOrd for Box<T> {
        fn partial_cmp(&self, o: &Box<T>) -> ::core::option::Option<::core::cmp::Ordering> { (**self).partial_cmp(&**o) }
    }
    impl<T: ::core::cmp::Ord + ?Sized> ::core::cmp::Ord for Box<T> {
        fn cmp(&self, o: &Box<T>) -> ::core::cmp::Ordering { (**self).cmp(&**o) }
    }
    impl<I: ::core::iter::Iterator + ?Sized> ::core::iter::Iterator for Box<I> {
        type Item = I::Item;
        fn next(&mut self) -> ::core::option::Option<I::Item> { (**self).next() }
    }
    // Box<dyn Fn(A) -> R> and Box<dyn FnMut..> / FnOnce are callable: the code
    // generator calls through the box; no impl is needed here.
}

pub mod error {
    use ::core::boxed::Box;
    use ::core::fmt::{Debug, Display};
    use ::core::string::String;

    pub trait Error: Debug + Display {}
    struct StringError(String);
    impl Debug for StringError {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result { Debug::fmt(&self.0, f) }
    }
    impl Display for StringError {
        fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result { Display::fmt(&self.0, f) }
    }
    impl Error for StringError {}
    impl Error for ::core::fmt::Error {}
    impl Error for ::core::num::ParseIntError {}
    impl Error for ::core::num::ParseFloatError {}
    impl Error for ::core::io::Error {}
    impl<'a> ::core::convert::From<&'a str> for Box<dyn Error> {
        fn from(s: &'a str) -> Box<dyn Error> { Box::new(StringError(String::from(s))) }
    }
    impl ::core::convert::From<String> for Box<dyn Error> {
        fn from(s: String) -> Box<dyn Error> { Box::new(StringError(s)) }
    }
    impl<E: Error> ::core::convert::From<E> for Box<dyn Error> {
        fn from(e: E) -> Box<dyn Error> { Box::new(e) }
    }
}

// ---------------------------------------------------------------- prelude

// Names visible in every module without a `use` (the resolver treats this module's
// public items as the outermost scope, the "prelude scope").
pub mod prelude {
    pub use ::core::marker::{Copy, Send, Sized, Sync};
    pub use ::core::ops::{Drop, Fn, FnMut, FnOnce};
    pub use ::core::mem::drop;
    pub use ::core::boxed::Box;
    pub use ::core::borrow::ToOwned;
    pub use ::core::clone::Clone;
    pub use ::core::cmp::{Eq, Ord, PartialEq, PartialOrd};
    pub use ::core::convert::{AsMut, AsRef, From, Into};
    pub use ::core::default::Default;
    pub use ::core::iter::{DoubleEndedIterator, Extend, FromIterator, IntoIterator, Iterator};
    pub use ::core::option::Option::{self, None, Some};
    pub use ::core::result::Result::{self, Err, Ok};
    pub use ::core::fmt::WriteTarget;   // write!/writeln! expansion (a hidden method name)
    pub use ::core::string::{String, ToString};
    pub use ::core::vec::Vec;
}
