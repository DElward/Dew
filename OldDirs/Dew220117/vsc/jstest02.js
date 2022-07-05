// JavaScript source code
// jstest02.js

'use strict';

////////////////////////////////////////////////////////////////////////
try {
    console.log("Test 101_18 try");
    throw "try throw";
    console.log("Test 101_18 try - ERROR");
} catch (err101_18a) {
    console.log("Test 101_18 catch");
    throw "catch throw";
    console.log("Test 101_18 catch - ERROR");
} finally {
    console.log("Test 101_18 finally");
    'debug show runstack';
    //throw "finally throw";
    //console.log("Test 101_18 finally - ERROR");
}
console.log("Test 101_18 done");

////////////////////////////////////////////////////////////////////////
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
