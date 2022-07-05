// JavaScript source code
// jstest02.js

'use strict';

function ff(nn) {
    return nn + 7;
}
console.log("ff(4)=", ff(4));
@@
////////////////////////////////////////////////////////////////////////
function f101_4a(nn, f1) {
    return f1((nn + 1) * 2);
}
function f101_4b(mm) {
    return f101_4a(mm, function (kk) { function ff(nn) { return nn + 7; } let yy = ff(200) + kk; return yy; });
}
var aa101_4a = f101_4b(1);
console.log("aa101_4a=", aa101_4a); // aa101_4a= 211
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
////////////////////////////////////////////////////////////////////////
