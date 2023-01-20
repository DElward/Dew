// JavaScript source code
// jstest02.js
//
//  Create l-val:       jvar_store_lval()
//  Assign to l-val:    jexp_binop_assign() -> jvar_assign_jvarvalue() -> jvar_store_jvarvalue()
//  Dereference l-val:  jexp_eval_rval()
//  'debug show allvars';
//  'debug show allclasses';
//  'debug show allclassmethods';
//  console.log("ss=", ss, "typeof ss=", typeof ss);

'use strict';

////////
//  var ngood = 0;
//  var nerrs = 0;
//  var printf = false;

////////////////////////////////////////////////////////////////////////
//  String.prototype.v101_36 = 16;
String.v101_36 = 32;
//var aa = String.v101_36;
//console.log("aa=", aa);
//  if (String.prototype.v101_36 == 16) ngood = ngood + 1; else { console.log("Test 101_36a failed"); nerrs = nerrs + 1; }
//  console.log("String.prototype.v101_36=", String.prototype.v101_36); // Exp: String.prototype.v101_36= 16
console.log("String.v101_36=", String.v101_36); // Exp: String.v101_36= 32
////////////////////////////////////////////////////////////////////////
//  if (nerrs == 0) console.log("All", ngood, "tests successful.");
//  else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
