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
var ss = "Alpha";
String.prototype.cat = 16;
String.cat = 32;
String.prototype.prototype = 64;
//  ss.beta = 4;        ERROR!
//  "Abc".gamma = 8;    ERROR!

//console.log("String.prototype=", String.prototype);         // Exp: String.prototype= String ('')
console.log("String.prototype.cat=", String.prototype.cat); // Exp: String.prototype.cat= 16
console.log("String.cat=", String.cat);                     // Exp: String.cat= 32
console.log("String.car=", String.car);                     // Exp: String.car= undefined
console.log("String.length=", String.length);               // Exp: String.length= 1
console.log("String.prototype.prototype=", String.prototype.prototype); // Exp: String.prototype.prototype= 64
console.log("--------");
console.log('ss.prototype=', ss.prototype);                 // Exp: ss.prototype= 64
console.log('ss.prototype.cat=', ss.prototype.cat);         // Exp: ss.prototype.cat= undefined
console.log('ss.cat=', ss.cat);                             // Exp: ss.cat= 16
console.log('ss.car=', ss.car);                             // Exp: ss.car= undefined
console.log('ss.length=', ss.length);                       // Exp: ss.length= 5
console.log('ss.prototype=', ss.prototype);                 // Exp: ss.prototype= 64
console.log("--------");
console.log('"Abc".prototype=', "Abc".prototype);           // Exp: "Abc".prototype= 64
console.log('"Abc".prototype.cat=', "Abc".prototype.cat);   // Exp: "Abc".prototype.cat= undefined
console.log('"Abc".cat=', "Abc".cat);                       // Exp: "Abc".cat= 16
console.log('"Abc".car=', "Abc".car);                       // Exp: "Abc".car= undefined
console.log('"Abc".length=', "Abc".length);                 // Exp: "Abc".length= 3
console.log('"Abc".prototype=', "Abc".prototype);           // Exp: "Abc".prototype= 64
////////////////////////////////////////////////////////////////////////
//  if (nerrs == 0) console.log("All", ngood, "tests successful.");
//  else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
//////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
