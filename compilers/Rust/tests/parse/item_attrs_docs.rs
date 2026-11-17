//! crate doc line 1
//! crate doc line 2
#![allow(unused)]
#![feature(a, b)]
#![doc = "inner eq"]

/// outer doc
#[derive(Debug, Clone)]
#[repr(C)]
struct S { x: i32 }

/** block doc */
#[inline(always)]
fn f() {}

#[cfg(test)]
mod tests {}

#[doc = "string"]
#[path = "other.rs"]
mod m;

#[rustfmt::skip]
#[clippy::cognitive_complexity = "10"]
fn g() {}

#[cfg_attr(feature = "x", derive(Debug))]
#[allow(clippy::all, unused_variables)]
#[a(b(c, d), [e], {f})]
#[a[b]]
#[a{b}]
#[empty()]
enum E {
    /// variant doc
    #[default]
    A,
    #[allow(x)]
    /// after attr
    B(#[serde(skip)] i32),
}

/// first
/// second
pub fn documented() {}

#[must_use]
pub(crate) const fn attr_vis() -> i32 { 1 }

#[macro_use]
extern crate foo;

#[derive(Clone)]
#[derive(Copy)]
struct Two;

fn genr<#[may_dangle] T, #[a] 'a>() {}
fn params(#[allow(unused)] x: i32, #[b] y: i32) {}
