// JavaScript source code
// jstest02.js

'use strict';

var ii = 2;
function inc(nn) {
    console.log("enter inc(", nn, ")");
    'debug show stacktrace';
    'debug show allvars';
    return nn + 1;
}

console.log("calling inc(ii)=", inc(ii));
@@
////////////////////////////////////////////////////////////////

function inc(nn) {
    console.log("enter inc(", nn, ")");
    'debug show stacktrace';
    'debug show allvars';
    return nn + 1;
}
function increment(nn) {
    console.log("enter increment(", nn, ")");
    'debug show stacktrace';
    'debug show allvars';
    return inc(nn + 0);
}

console.log("calling increment(2)=", increment(2));
@@
////////////////////////////////////////////////////////////////

function inc(nn)
{
    console.log("enter inc(", nn, ")");
    'debug show stacktrace';
    'debug show allvars';
    return nn + 1;
}
function dec(nn)
{
    console.log("enter dec(", nn, ")");
    'debug show stacktrace';
    'debug show allvars';
    return nn - 1;
}
function add(mm, nn)
{
    console.log("enter add(", mm, ",", nn, ")");
    'debug show stacktrace';
    'debug show allvars';
    if (nn == 0) return mm;
    else {
        return add(inc(mm), dec(nn));
    }
}

console.log("calling add(2,1)=", add(2, 1));

///////////////////////////

//////// End tests ////////

if (nerrs == 0) console.log("All", ngood, "tests successful.");
else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
//  @@

// JavaScript source code
// jstest02.js

'use strict';

console.log("-- testing --");
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
////////////////////////////////////////////////////////////////////////
function inc(nn) {
    return nn + 1;
}
function dec(nn) {
    return nn - 1;
}
function add(mm, nn) {
    if (nn == 0) return mm;
    else { return add(inc(mm), dec(nn)); }
}
function mul(mm, nn) {
    if (mm == 0 || nn == 0) return 0;
    else { if (nn == 1) return mm; else return add(mm, mul(mm, dec(nn))); }
}
function pow(mm, nn) {
    if (nn == 0) return 1;
    else return mul(mm, pow(mm, dec(nn)));
}
function fact(nn) {
    if (nn == 0) return 1;
    else return mul(nn, fact(dec(nn)));
}

console.log("calling pow(2,3)=", pow(2, 3));
console.log("calling fact(4)=", fact(4));
//  @@
///////////////////////////
var tools = require('./xeqfil-Tests101-Tests.js');
console.log("-- main() tools=", tools);
tools.test_101();
@@
////////////////////////////////////////////////////////////////////////
//function plus3(vv) {
//    return vv + 3;
//}
//var plus5 = function (nn) {
//    'debug show stacktrace';
//    return nn + 5;
//};
//var aa = function (vv) {
//    //console.log("-- ff() aa(4)=", aa(4));
//    return vv + 3;
//}(4);
///////////////////////////
function f0(nn) {
    var xx = nn + 1;
    'debug show allvars';
    function f1(mm) {
        var yy = xx + mm;
        return yy;
    }
    return f1(xx * 2);
}
var aa = f0(1);
console.log("  f0(1)=", aa);
console.log("--end main tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
///////////////////////////
function ff(aa) {
    var bb = 2;
    'debug show allvars';
    function gg(cc) {
        var dd = 4;
        aa = aa + 10;
        bb = bb + 10;
        cc = cc + 10;
        dd = dd + 10;
        'debug show allvars';
        console.log("  aa=", aa, "bb=", bb, "cc=", cc, "dd=", dd);
    }
    gg(3);
}
ff(1);
console.log("--end main tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////

///////////////////////////
function f0(nn) {
    var xx = nn + 1;
    console.log("    (1)xx=", xx);
    function f1(mm) {
        console.log("    (5)xx=", xx);
        'debug show allvars';
        console.log("    (2)xx=", xx, "mm=", mm);
        var yy = xx + mm;
        console.log("    (3)xx=", xx, "yy=", yy);
        return yy;
    }
    console.log("    (4)xx=", xx);
    while (false);
    return f1(xx * 2);
}
var aa = f0(1);
console.log("  f0(1)=", aa);
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
////////////////////////////////
function f0(nn, f1) {
    var xx = nn + 1;
    console.log("  in f1()");
    
    return f1(xx * 2);
}
var xx = 100;
function gg(mm)
{
    var yy = xx + mm;
    'debug show allvars';
    return yy;
}
var aa = f0(1, gg);
console.log("  f0(1)=", aa);    // Expect: f0(1)= 104 - jcx_outer_jcx
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
////////////////////////////////
function f0(nn, f1) {
    var xx = nn + 1;
    console.log("  in f1()");

    return f1(xx * 2);
}
var xx = 100;
function gg(mm) {
    var yy = xx + mm;
    'debug show allvars';
    return yy;
}
var aa = f0(1, gg);
console.log("  f0(1)=", aa);    // Expect: f0(1)= 104 - jcx_outer_jcx
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
////////////////////////////////
var xx = 100;
function gg(mm) {
    var yy = xx + mm;
    'debug show allvars';
    return yy;
}
var aa = gg(1);
console.log("  f0(1)=", aa);    // Expect: f0(1)= 104 - jcx_outer_jcx
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
function f0(nn) {
    var xx = nn * 10;

    return xx;
}
var aa = f0(2);
console.log("  f0(1)=", aa);
console.log("--end main tests--");
@@
////////////////////////////////
function f0(nn, f1) {
    var xx = nn + 1;
    console.log("  in f1()");

    return f1(xx * 2);
}
var xx = 100;
function gg(mm) {
    var yy = xx + mm;
    'debug show allvars';
    return yy;
}
var aa = f0(1, gg);
console.log("  f0(1)=", aa);    // Expect: f0(1)= 104 - jcx_outer_jcx
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
function f0(nn, f1) {
    var xx = nn + 1;

    return f1(xx * 2);
}
var xx = 100;
function gg(mm) {
    function hh(kk) {
        var yy = xx + kk;
        return yy;
    }
    var xx = 200;
    return f0(mm, hh);
}
var aa = gg(1);
console.log("  gg(1)=", aa);    // Pass[09/13/21] Expect: gg(1)= 204 - jcx_outer_jcx
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
function fact(nn) {
    if (nn == 0) return 1;
    else return nn * fact(nn - 1);
    //'debug show allvars';
    //return nn * nn;
}
var aa = fact(4);
console.log("  fact(4)=", aa);    // Expect: f0(1)= 104 - jcx_outer_jcx
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
function f0(nn, f1) {
    var xx = nn + 1;

    return f1(xx * 2);
}
var xx = 100;
function gg(mm) {
    
    var xx = 200;
    return f0(mm, function (kk) { var yy = xx + kk; 'debug show allvars'; return yy; });
}
var aa = gg(1);
console.log("  gg(1)=", aa);    // Expect: gg(1)= 204 - jcx_outer_jcx
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
function fact(nn) {
    function mul(mm, nn) {
        //'debug show allvars';
        return mm *nn;
    }
    //'debug show allvars';
    if (nn == 0) return 1;
    else return mul(nn, fact(nn - 1));
}
var aa = fact(7);
console.log("  fact(0)=", aa);
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
let aa = 'apple';
let bb = 'banana';
var cc = bb = aa;
'debug show allvars';

console.log("aa=", aa, "bb=", bb, "cc=", cc);
//console.log("fruits.length=", fruits.length);
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
@@
////////////////////////////////
let aa = 'apple';
let bb = 'banana';
var cc = [bb, aa];
'debug show allvars';

console.log("aa=", aa, "bb=", bb, "cc=", cc);
//console.log("fruits.length=", fruits.length);
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
function ff(msg, lp)
{
    lp(msg);
}
let ml=console.log;
ff("Testing 1...2...3...", ml);
//
//'debug show allvars';
console.log("--end main tests--");
////////////////////////////////
function f101_4a(nn, f1) {
    var xx4 = nn + 1;

    return f1(xx4 * 2);
}
var xx4 = 100;
var bb = f101_4a(1, function (kk) { var yy = xx4 + kk; return yy; });
console.log("bb=", bb); // exp 104
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
function ff(msg, lp) {
    'debug show allvars';
    lp(msg);
}
let ml = console.log;
ff("Testing 1...2...3...", ml);
const gg = function (kk) { var yy = xx4 + kk; return yy; };
//
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
var abc = "Abcdefghijklmn";
//  var len = abc.length;
//  console.log("len=", len); // exp 14
var subs = abc.substring;
var bb=subs(3, 8);
console.log("subs=", subs); // exp defgh
console.log("bb=", bb);
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
let aa = 'apple';
let bb = 'banana';
var cc = [aa + " pie", bb + " pie"];
//'debug show allvars';

console.log("aa=", aa, "bb=", bb, "cc=", cc);
//console.log("cc.length=", cc.length);
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
@@

////////////////////////////////
var cc = { fruit: "apple", size: "medium" };
//'debug show allvars';

console.log("cc=", cc); // exp: {fruit: 'apple', size: 'medium'}
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
function getRectArea(width, height) {
    if (isNaN(width) || isNaN(height)) {
        throw 'Parameter is not a number!';
        //let msg = 'Parameter is not a number!';
        //console.log(msg);
    }
}

try {
    getRectArea(3, 'A');
} catch (err) {
    console.log("in   catch 1 msg:", err.toString());   // , err.toString()
    //console.error(e);
}
////////////////////////////////////////////////////////////////////////
////    11/30/2021
////////////////////////////////////////////////////////////////////////
let nn101_13 = 0;

function getRectArea101_13a(width, height) {
    try {
        throw "getRectArea101_13b(width, height)";
        console.log("Test 101_13b failed");
        nerrs = nerrs + 1;
    } catch (err101_13a) {
        nn101_13 = nn101_13 + 1;
        throw err101_13a;
        console.log("Test 101_13c failed");
        nerrs = nerrs + 1;
    } finally {
        'debug show allvars';
        console.log("Test 101_13 finally 1");
        nn101_13 = nn101_13 + 1;
    }
    console.log("Test 101_13d failed");
    nerrs = nerrs + 1;
}

try {
    getRectArea101_13a(3, 'A');
    console.log("Test 101_13e failed");
    nerrs = nerrs + 1;
    console.log("Test 101_13e failed");
    nerrs = nerrs + 1;
} catch (err101_13b) {
    if (err101_13b.toString() == "getRectArea101_13b() Parameter is not a number!") {
        nn101_13 = nn101_13 + 1;
    } else {
        console.log(err101_13.toString());
        console.log("Test 101_13f failed");
        nerrs = nerrs + 1;
    }
} finally {
    nn101_13 = nn101_13 + 1;
    console.log("Test 101_13 finally 2", "nn101_13=", nn101_13);
}
console.log("nn101_13=", nn101_13);
if (nn101_13 == 4) {
    ngood = ngood + 1;
} else {
    console.log("Test 101_13g failed");
    nerrs = nerrs + 1;
}
@@
////////////////////////////////////////////////////////////////////////
let nn101_13 = 0;

function getRectArea101_13b(width, height) {
    if (isNaN(width) || isNaN(height)) {
        throw 'getRectArea101_13b() Parameter is not a number!';
    }
    console.log("Test 101_13a failed");
    nerrs = nerrs + 1;
}

function getRectArea101_13a(width, height) {
    try {
        //  getRectArea101_13b(width, height);
        throw "getRectArea101_13b(width, height)";
        console.log("Test 101_13b failed");
        nerrs = nerrs + 1;
    } catch (err101_13a) {
        nn101_13 = nn101_13 + 1;
        throw err101_13a;
        console.log("Test 101_13c failed");
        nerrs = nerrs + 1;
    } finally {
        nn101_13 = nn101_13 + 1;
    }
    console.log("Test 101_13d failed");
    nerrs = nerrs + 1;
}

try {
    {
        getRectArea101_13a(3, 'A');
        console.log("Test 101_13e failed");
        nerrs = nerrs + 1;
    }
    console.log("Test 101_13e failed");
    nerrs = nerrs + 1;
} catch (err101_13b) {
    if (err101_13b.toString() == "getRectArea101_13b() Parameter is not a number!") {
        nn101_13 = nn101_13 + 1;
    } else {
        console.log(err101_13.toString());
        console.log("Test 101_13f failed");
        nerrs = nerrs + 1;
    }
} finally {
    nn101_13 = nn101_13 + 1;
}
console.log("nn101_13=", nn101_13);
if (nn101_13 == 4) {
    ngood = ngood + 1;
} else {
    console.log("Test 101_13g failed");
    nerrs = nerrs + 1;
}

////////////////////////////////////////////////////////////////////////
if (nerrs == 0) console.log("All", ngood, "tests successful.");
else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
////////////////////////////////////////////////////////////////////////
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////

//  C:\Program Files\nodejs\node.exe .\jstest02.js
//  nn101_13= 4
//  All 1 tests successful.
//  --end main tests--

////////////////////////////////////////////////////////////////////////
////////
var ngood = 0;
var nerrs = 0;

////////////////////////////////////////////////////////////////////////
let nn101_13 = 0;

function getRectArea101_13a(width, height) {
    try {
        throw "getRectArea101_13b(width, height)";
        console.log("Test 101_13b failed");
        nerrs = nerrs + 1;
    } catch (err101_13a) {
        nn101_13 = nn101_13 + 1;
        throw err101_13a;
        console.log("Test 101_13c failed");
        nerrs = nerrs + 1;
    } finally {
        //  'debug show allvars';
        console.log("Test 101_13 finally 1");
        nn101_13 = nn101_13 + 1;
    }
    console.log("Test 101_13d failed");
    nerrs = nerrs + 1;
}
try {
    getRectArea101_13a(3, 'A');
} catch (err101_13a) {
    console.log("Test 101_13c failed, err=");
} finally {
    console.log("nn101_13=", nn101_13);
}
////////////////////////////////////////////////////////////////////////
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
////    12/13/2021
////////////////////////////////////////////////////////////////////////

function getRectArea101_13a(width, height) {
    //'debug show allvars';
    nn101_13 = nn101_13 + width;
}
let nn101_13 = 0;
'debug show allvars';
nn101_13 = nn101_13 + 1;
getRectArea101_13a(2, 'A');
nn101_13 = nn101_13 + 4;
console.log("nn101_13=", nn101_13);
////////////////////////////////////////////////////////////////////////
console.log("--end main tests--");
////////////////////////////////////////////////////////////////////////
////    12/27/2021 - Begin
////////////////////////////////////////////////////////////////////////
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
// 'debug show allvars';
nn101_14 = nn101_14 + 1;
console.log("nn101_14=", nn101_14); // Expected 1303
////////////////////////////////////////////////////////////////////////
////    12/27/2021 - End
////////////////////////////////////////////////////////////////////////
