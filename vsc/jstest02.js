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

////////////////////////////////////////////////////////////////////////
// Test 101_30 - 07/19/2022
////////////////////////////////////////////////////////////////////////
function func_101_30b(tvers, vob1) {
    return vob1.co;
}
const obj101b_29b = { fn: "Jack", ln: "Long", co: { na: "Taurus", ci: "Belmont", ea: ["Dave", "Andy"] } };
let ss = func_101_30b('a', obj101b_29b);
console.log(ss);

//if (func_101_30b(obj101b_29b).ci == "Belmont") { console.log("Good."); }

////////////////////////////////////////////////////////////////////////
//  if (nerrs == 0) console.log("All", ngood, "tests successful.");
//  else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
