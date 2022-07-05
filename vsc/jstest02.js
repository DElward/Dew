// JavaScript source code
// jstest02.js
//
//  Create l-val:       jvar_store_lval()
//  Assign to l-val:    jexp_binop_assign() -> jvar_assign_jvarvalue() -> jvar_store_jvarvalue()
//  Dereference l-val:  jexp_eval_rval()
//  'debug show allvars';
//  console.log("ss=", ss);

'use strict';

const person = { firstName: "John", lastName: "Doe", age: 50, eyeColor: "blue", company: { coname: "Taurus Software", cocity : "San Carlos" } };
let ss = person.company.cocity;
//let ss = { first: "Taurus", last: "Software" }.first;
console.log("ss=", ss);

////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
