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

var ss = { firstName: "John", lastName: "Jones", age: 29 };
ss.model = 'Eagle';
console.log("ss=", ss); // Exp: ss= {firstName: 'John', lastName: 'Jones', age: 29, model: 'Eagle'}
//  var tt = ss.make;
//  console.log("tt=", tt);             // Exp: tt= undefined
//  console.log("ss.year=", ss.year);   // Exp: ss.year= undefined

////////////////////////////////////////////////////////////////////////
//  function Car(make, model, year) {
//      console.log("Enter Car()");
//      this.make = make;
//      //  this.model = model;
//      //  this.year = year;
//      //  var make = "BMW";
//      //  
//      //  make = make + "Ford";
//      //  this.make = this.make + "Honda";
//      //  
//      //  console.log("make=", make);             // Exp: make= BMWFord
//      //  console.log("this.make=", this.make);   // Exp: this.make= EagleHonda
//      //  console.log("make=", make, "model=", model, "year=", year);
//  }
//  
//  const car1 = new Car('Eagle', 'Talon TSi', 1993);
//  console.log("null=", null);
//  //const car1 = new Car('Eagle', 'Talon TSi', 1993);
//  //  console.log("car1.make=", car1.make);       // Exp: car1.make= EagleHonda
////////////////////////////////////////////////////////////////////////
//  if (nerrs == 0) console.log("All", ngood, "tests successful.");
//  else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
