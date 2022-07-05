// JavaScript source code
// jstest02.js

'use strict';

var var01;
function func0() {
    return "func0";
}
function func1() {
    return "func1";
}
var01=func0() + func1();
console.log("-- main() var01=", var01);
@@
////////////////////////////////

var var01 = 'one', var02;
var02 = 'two';
console.log("-- testing --", var01 + var02);
'debug show allvars';
@@

var var01 = "var01", var02 = 'var02';

function func0(alpha, beta) {
    console.log("-- func0() var01=", var01, "var02=", var02);
    if (false) {
        var var01 = 'two';
        console.log("-- func0() var01=", var01, "var02=", var02);
    }
}
func0();
console.log("-- main() var01=", var01, "var02=", var02);
@@
////////////////////////////////
var var01 = 'one', var02 = 'two';
function func0() {
    var fvar03= 'three', fvar04 = 'four';
    console.log("-- func0() --");
    func1();
}
function func1() {
    var fvar05= 'five', fvar06 = 'six';
    console.log("-- func1() --");
    console.log("-- fvar06=", fvar06);
    'debug show allvars';
}
func0();
console.log("-- main() --");
@@
////////////////////////////////

