// JavaScript source code
// jstest02.js

'use strict';

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

//////// End tests ////////
