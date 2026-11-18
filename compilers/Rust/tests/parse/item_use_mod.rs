use a;
use a::b;
use a::b::c as d;
use a::b as _;
use ::a::b;
use a::*;
use a::b::*;
use ::*;
use *;
use a::{b, c};
use a::{b, c,};
use a::{self, b::c, d::{e, f as g}, h::*};
use {a, b};
use ::{a, b};
use self::a;
use super::a;
use super::super::a::b;
use crate::a::b;
use crate::{a, b};
use a::{};
use a::{self as me};
pub use a::b;
pub(crate) use a::b as c;
extern crate foo;
extern crate foo as bar;
extern crate self as me;
extern crate foo as _;
pub extern crate baz;
mod empty {}
mod outer { mod inner { fn f() {} } pub fn g() {} }
mod declared;
pub mod public;
pub(crate) mod in_crate { use super::*; }
mod with_attrs {
    #![allow(dead_code)]
    //! inner doc
    struct S;
}
unsafe mod u {}
