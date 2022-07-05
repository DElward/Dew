// JavaScript source code
// jstest02.js

'use strict';

function inc1(nn, nn) {
    return nn + 1;
}

console.log("inc1(4)=", inc1(4));
console.log("--end function tests--");
@@

function inc1(nn) {
    return nn + 1;
}

var inc2 = function (nn) {
    return nn + 1;
}
var nn, vv1, vv2;
nn = 4;
vv1 = inc1(nn);
console.log("vv1=", vv1);

nn = 44;
vv2 = inc2(nn);
console.log("vv2=", vv2);
console.log("--end function tests--");

//////// End tests ////////
