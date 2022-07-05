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
let aa = ['pigs', 'goats'];
let ss = aa;
//console.log("ss=", ss);
'debug show allvars';
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////

//var aa = "01Alpha23".substring;
//console.log(aa(2, 7));                    <-- Fails
//console.log("ABCDEFGHIJ".aa(2, 7));       <-- Fails
//let aa = ff(['one', ['two', 'beta'], 'three']);
