// xeqfil-Tests101-Tests.js
//
////////////////////////////////////////////////////////////////
//  07/22/2021

////////////////////////////////////////////////////////////////////////
//////// Good tests ////////
//  12/28/2021  Verified against node.js

//console.log("-- Enter xeqfil-Tests101-Tests.js");
'use strict';

////////
var ngood = 0;
var nerrs = 0;

////////////////////////////////////////////////////////////////////////
function inc(nn) { return nn + 1; }
function dec(nn) { return nn - 1; }
function add(mm, nn) { if (nn == 0) return mm; else { return add(inc(mm), dec(nn)); } }
function mul(mm, nn) { if (mm == 0 || nn == 0) return 0; else { if (nn == 1) return mm; else return add(mm, mul(mm, dec(nn))); } }
function pow(mm, nn) { if (nn == 0) return 1; else return mul(mm, pow(mm, dec(nn))); }
function fact(nn) { if (nn == 0) { return 1; } else return mul(nn, fact(dec(nn))); }

if (pow(2, 3) == 8) ngood = ngood + 1; else { console.log("Test 101-01 failed"); nerrs = nerrs + 1; }
if (fact(4) == 24) ngood = ngood + 1; else { console.log("Test 101-02 failed"); nerrs = nerrs + 1; }
if (pow(fact(2), pow(2, 2)) == 16) ngood = ngood + 1; else { console.log("Test 101-03 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
function f101_4a(nn, f1) {
    var xx4 = nn + 1;

    return f1(xx4 * 2);
}
var xx4 = 100;
function f101_4b(mm) {

    var xx4 = 200;
    return f101_4a(mm, function (kk) { var yy = xx4 + kk; return yy; });
}
var aa101_4a = f101_4b(1);
if (aa101_4a == 204) ngood = ngood + 1; else { console.log("Test 101-04 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
function f101_5a(nn, f1) {
    var xx5 = nn + 1;

    return f1(xx5 * 2);
}
var xx5 = 100;
function f101_5b(mm) {
    function hh(kk) {
        var yy = xx5 + kk;
        return yy;
    }

    var xx5 = 200;
    return f101_5a(mm, hh);
}
var aa101_5a = f101_5b(1);
if (aa101_5a == 204) ngood = ngood + 1; else { console.log("Test 101-05 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
function f101_6(nn) {
    var yy = nn + 3;
    return yy;
}
var aa101_6 = 9;
{
    let bb101_6 = 2;
    let aa101_6 = f101_6(bb101_6);

    if (aa101_6 == 5) ngood = ngood + 1; else { console.log("Test 101-06a failed"); nerrs = nerrs + 1; }
    if (bb101_6 == 2) ngood = ngood + 1; else { console.log("Test 101-06c failed"); nerrs = nerrs + 1; }
}
if (aa101_6 == 9) ngood = ngood + 1; else { console.log("Test 101-06c failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
const aa101_10 = [11, 22, 33];
const bb101_10 = [11, 22, 33];
const cc101_10 = bb101_10;

if (aa101_10 == bb101_10) {
    console.log("Test 101-10a failed");
    nerrs = nerrs + 1;
} else {
    ngood = ngood + 1;
}
if (aa101_10 == cc101_10) {
    console.log("Test 101-10b failed");
    nerrs = nerrs + 1;
} else {
    ngood = ngood + 1;
}
if (bb101_10 == cc101_10) {
    ngood = ngood + 1;
} else {
    console.log("Test 101-10c failed");
    nerrs = nerrs + 1;
}
////////////////////////////////////////////////////////////////////////
// 11/24/2021
function getRectArea101_11(width, height) {
    if (isNaN(width) || isNaN(height)) {
        throw 'getRectArea101_11() Parameter is not a number!';
    }
}

try {
    getRectArea101_11(3, 'A');
    console.log("Test 101_11b failed");
    nerrs = nerrs + 1;
} catch (err101_11) {
    if (err101_11.toString() == "getRectArea101_11() Parameter is not a number!") {
        ngood = ngood + 1;
    } else {
        console.log(err101_11.toString());
        console.log("Test 101_11a failed");
        nerrs = nerrs + 1;
    }
}
////////////////////////////////////////////////////////////////////////
// 11/24/2021
function getRectArea101_12b(width, height) {
    if (isNaN(width) || isNaN(height)) {
        throw 'getRectArea101_12b() Parameter is not a number!';
    }
    console.log("Test 101_12d failed");
    nerrs = nerrs + 1;
}

function getRectArea101_12a(width, height) {
    getRectArea101_12b(width, height);
    console.log("Test 101_12c failed");
    nerrs = nerrs + 1;
}

try {
    {
        getRectArea101_12a(3, 'A');
        console.log("Test 101_12b failed");
        nerrs = nerrs + 1;
    }
    console.log("Test 101_12e failed");
    nerrs = nerrs + 1;
} catch (err101_12) {
    if (err101_12.toString() == "getRectArea101_12b() Parameter is not a number!") {
        ngood = ngood + 1;
    } else {
        console.log(err101_12.toString());
        console.log("Test 101_12a failed");
        nerrs = nerrs + 1;
    }
}
////////////////////////////////////////////////////////////////////////
// Test 101_13  - 01/04/2022
function getRectArea101_13a(width) {
    nn101_13 = nn101_13 + width;
}
let nn101_13 = 0;
nn101_13 = nn101_13 + 1;
getRectArea101_13a(2);
nn101_13 = nn101_13 + 4;
if (nn101_13 == 7) ngood = ngood + 1; else { console.log("Test 101-13 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_14  - 01/04/2022
let nn101_14 = 1000;
if (nn101_14 < 500) {
    var aab101_14 = 20;
    function getRectArea101_14a(width) {
        nn101_14 = nn101_14 + width + aab101_14;
    }
    getRectArea101_14a(2);
} else {
    var aab101_14 = 100;
    function getRectArea101_14a(width) {
        nn101_14 = nn101_14 + width + aab101_14 + 200;
    }
    getRectArea101_14a(2);
}
nn101_14 = nn101_14 + 1;
if (nn101_14 == 1303) ngood = ngood + 1; else { console.log("Test 101-14 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
if (nerrs == 0) console.log("All", ngood, "tests successful.");
else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
console.log("--end xeqfil-Tests101-Tests tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
