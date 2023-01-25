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
String.prototype.v101_36a = 16;
var ss = "Alpha".v101_36a;
console.log('"Alpha".v101_36a=', ss); // Exp: ss.v101_36a= 16
////////////////////////////////////////////////////////////////////////
//  if (nerrs == 0) console.log("All", ngood, "tests successful.");
//  else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
