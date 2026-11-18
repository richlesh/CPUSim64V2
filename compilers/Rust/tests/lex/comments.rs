#!/usr/bin/env rustc-script --flag
// line comment
/* block */ a /* nested /* inner */ still comment */ b
/// outer line doc
//// four slashes is a plain comment
//! inner line doc
/** outer block doc
 * second line */
/*! inner block doc */
/**/ c /***/ d /*** triple star is plain ***/
fn /* between */ f() {} // trailing
/// doc with unicode é 😀 and CRLF-free text
x /*/ not closed here */ y
/* spans
   several
   lines */ z
