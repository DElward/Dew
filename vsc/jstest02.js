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
//  var v101_33;
//  v101_33 = Boolean(2);
//  console.log("v101_33=", v101_33, "typeof v101_33=", typeof v101_33);
console.log("Boolean(0)         =", Boolean(0));
console.log("Boolean(2)         =", Boolean(2));
console.log("Boolean('true')    =", Boolean('true'));
console.log("Boolean('false')   =", Boolean('false'));
console.log("Boolean('yes')     =", Boolean('yes'));
console.log("Boolean('no')      =", Boolean('no'));
console.log("Boolean('0')       =", Boolean('0'));
console.log("Boolean('0.00')    =", Boolean('0.00'));
console.log("Boolean(' ')       =", Boolean(' '));
console.log("Boolean('')        =", Boolean(''));
////////////////////////////////////////////////////////////////////////
//  if (nerrs == 0) console.log("All", ngood, "tests successful.");
//  else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
