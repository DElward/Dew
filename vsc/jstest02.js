// JavaScript source code
// jstest02.js
//
//  Create l-val:       jvar_store_lval()
//  Assign to l-val:    jexp_binop_assign() -> jvar_assign_jvarvalue() -> jvar_store_jvarvalue()
//  Dereference l-val:  jexp_eval_rval()
//  'debug show allvars';
//  console.log("ss=", ss);

'use strict';

////////
//  var ngood = 0;
//  var nerrs = 0;
//  var printf = false;

//  function f101_31a(arg1) { return arg1 + "DEF"; }
//  let s101_31l = `xy${f101_31a("ABC")}z`;
//  //  let s101_31l = `${f101_31a("ABC")}`;
//  console.log("s101_31l=", s101_31l);
////////////////////////////////////////////////////////////////////////
let v101_31a = "abcdefghiKlmnopqrsT";
var v101_31e = `xy${v101_31a}z`;
function f101_31a(arg1) { return arg1 + "DEF"; }
let s101_31l = `xy${f101_31a("ABC")}z`;
console.log("s101_31l=", s101_31l);         // Exp: s101_31l= xyABCDEFz
let s101_31m = `xy${f101_31a(v101_31e)}z`;
console.log("s101_31m=", s101_31m);         // Exp: s101_31m= xyxyabcdefghiKlmnopqrsTzDEFz
let s101_31n = `xy${f101_31a(`xy${v101_31a}z`)}z`;
console.log("s101_31n=", s101_31n);         // Exp: s101_31n= xyxyabcdefghiKlmnopqrsTzDEFz
////////////////////////////////////////////////////////////////////////
//  if (nerrs == 0) console.log("All", ngood, "tests successful.");
//  else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////

