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
let ss = "01Alpha23".substring(2, 7);
console.log("ss=", ss);
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////

//var aa = "01Alpha23".substring;
//console.log(aa(2, 7));                    <-- Fails
//console.log("ABCDEFGHIJ".aa(2, 7));       <-- Fails