// JavaScript source code
// jstest02.js
function inchestometers(inches) {
    if (inches < 0)
        return -1;
    else {
        var meters = inches / 39.37;
        return meters;
    }
}

var inches = 12;
var meters = inchestometers(inches);
console.log("the value in meters is " + meters);
