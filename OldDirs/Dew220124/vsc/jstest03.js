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
////////////////////////////////////////////////////////////////////////
////    01/06/2022 - Begin
////////////////////////////////////////////////////////////////////////
var ngood = 0;
var nerrs = 0;
////////

////////////////////////////////////////////////////////////////////////
let nn101_16a = 0;
let nn101_16b = 0;
let nn101_16c = 0;
let nn101_16d = 0;
var ii101_16 = 0;

while (ii101_16 <= 1) {
    let jj = 0;
    while (jj <= 1) {
        let kk = 0;
        while (kk <= 1) {
            let ll = 0;
            while (ll <= 1) {
                let mm = 0;
                while (mm <= 1) {
                    let nn = 0;
                    while (nn <= 1) {
                        if (ii101_16 == 1 || jj == 1 || kk == 1 || ll == 1 || mm == 1 || nn == 1) {
                            nn101_16a = nn101_16a + 1;
                        }
                        if (ii101_16 == 1 && jj == 1 && kk == 1 && ll == 1 && mm == 1 && nn == 1) {
                            nn101_16b = nn101_16b + 1;
                        }
                        if (ii101_16 == 1 || jj == 1 && kk == 1 && ll == 1 && mm == 1 || nn == 1) {
                            nn101_16c = nn101_16c + 1;
                        }
                        if (ii101_16 == 1 && jj == 1 || kk == 1 || ll == 1 || mm == 1 && nn == 1) {
                            nn101_16d = nn101_16d + 1;
                        }
                        nn = nn + 1;
                    }
                    mm = mm + 1;
                }
                ll = ll + 1;
            }
            kk = kk + 1;
        }
        jj = jj + 1;
    }
    ii101_16 = ii101_16 + 1;
}
console.log("nn101_16a=", nn101_16a); // Expected 63
console.log("nn101_16b=", nn101_16b); // Expected 1
console.log("nn101_16c=", nn101_16c); // Expected 49
console.log("nn101_16d=", nn101_16d); // Expected 55
if (nn101_16a == 63) ngood = ngood + 1; else { console.log("Test 101_16a failed"); nerrs = nerrs + 1; }
if (nn101_16b == 1) ngood = ngood + 1; else { console.log("Test 101_16b failed"); nerrs = nerrs + 1; }
if (nn101_16c == 49) ngood = ngood + 1; else { console.log("Test 101_16c failed"); nerrs = nerrs + 1; }
if (nn101_16d == 55) ngood = ngood + 1; else { console.log("Test 101_16d failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
if (nerrs == 0) console.log("All", ngood, "tests successful.");
else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);

////////////////////////////////////////////////////////////////////////
////    01/06/2022 - End
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
var ngood = 0;
var nerrs = 0;
////////

////////////////////////////////////////////////////////////////////////
let nn101_16a = 0;
let nn101_16b = 0;
let nn101_16c = 0;
let nn101_16d = 0;
var ii101_16 = 0;

while (ii101_16 <= 1) {
    let jj = 0;
    while (jj <= 1) {
        let kk = 0;
        while (kk <= 1) {
            let ll = 0;
            while (ll <= 1) {
                let mm = 0;
                while (mm <= 1) {
                    let nn = 0;
                    while (nn <= 1) {
                        if (ii101_16 == 1 && jj == 1 || kk == 1 || ll == 1) {
                            nn101_16d = nn101_16d + 1;
                            console.log(nn101_16d, "ii101_16=", ii101_16, "jj=", jj, "kk=", kk, "ll=", ll);
                        }
                        nn = nn + 1;
                    }
                    mm = mm + 1;
                }
                ll = ll + 1;
            }
            kk = kk + 1;
        }
        jj = jj + 1;
    }
    ii101_16 = ii101_16 + 1;
}
console.log("nn101_16d=", nn101_16d); // Expected 55
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////    01/11/2022 - Begin
////////////////////////////////////////////////////////////////////////
var ngood = 0;
var nerrs = 0;
////////

////////////////////////////////////////////////////////////////////////
let nn101_16a = 0;
let nn101_16b = 0;
let nn101_16c = 0;
let nn101_16d = 0;
let nn101_16e = 0;
let nn101_16f = 0;
let nn101_16g = 0;
var ii101_16 = 0;

while (ii101_16 <= 1) {
    let jj = 0;
    while (jj <= 1) {
        let kk = 0;
        while (kk <= 1) {
            let ll = 0;
            while (ll <= 1) {
                let mm = 0;
                while (mm <= 1) {
                    let nn = 0;
                    while (nn <= 1) {
                        let oo = 0;
                        while (oo <= 1) {
                            let pp = 0;
                            while (pp <= 1) {
                                if (ii101_16 == 1 || jj == 1 || kk == 1 || ll == 1 || mm == 1 || nn == 1 || oo == 1 || pp == 1) {
                                    nn101_16a = nn101_16a + 1;
                                }
                                if (ii101_16 == 1 && jj == 1 && kk == 1 && ll == 1 && mm == 1 && nn == 1 || oo == 1 && pp == 1) {
                                    nn101_16b = nn101_16b + 1;
                                }
                                if (ii101_16 == 1 || jj == 1 && kk == 1 && ll == 1 && mm == 1 || nn == 1 && oo == 1 || pp == 1) {
                                    nn101_16c = nn101_16c + 1;
                                }
                                if (ii101_16 == 1 && jj == 1 || kk == 1 || ll == 1 && mm == 1 && nn == 1 || oo == 1 && pp == 1) {
                                    nn101_16d = nn101_16d + 1;
                                }
                                if (ii101_16 == 1 && (jj == 1 && kk == 1 || ll == 1) || (mm == 1 || nn == 1 && oo == 1) || pp == 1) {
                                    nn101_16e = nn101_16e + 1;
                                }
                                //if ((ii101_16 == 1 && jj == 1 || (kk == 1 || ll == 1 && mm == 1 && nn == 1) || oo == 1) && pp == 1) {
                                //    nn101_16f = nn101_16f + 1;
                                //}
                                if (ii101_16 == 1 && (jj == 1 && kk == 1 || ll == 1) && (mm == 1 || nn == 1 && oo == 1) && pp == 1) {
                                    nn101_16g = nn101_16g + 1;
                                }
                                pp = pp + 1;
                            }
                            oo = oo + 1;
                        }
                        nn = nn + 1;
                    }
                    mm = mm + 1;
                }
                ll = ll + 1;
            }
            kk = kk + 1;
        }
        jj = jj + 1;
    }
    ii101_16 = ii101_16 + 1;
}
console.log("nn101_16a=", nn101_16a);
console.log("nn101_16b=", nn101_16b);
console.log("nn101_16c=", nn101_16c);
console.log("nn101_16d=", nn101_16d);
console.log("nn101_16e=", nn101_16e);
console.log("nn101_16f=", nn101_16f);
console.log("nn101_16g=", nn101_16g);
if (nn101_16a == 255) ngood = ngood + 1; else { console.log("Test 101_16a failed"); nerrs = nerrs + 1; }
if (nn101_16b ==  67) ngood = ngood + 1; else { console.log("Test 101_16b failed"); nerrs = nerrs + 1; }
if (nn101_16c == 211) ngood = ngood + 1; else { console.log("Test 101_16c failed"); nerrs = nerrs + 1; }
if (nn101_16d == 193) ngood = ngood + 1; else { console.log("Test 101_16d failed"); nerrs = nerrs + 1; }
if (nn101_16e == 223) ngood = ngood + 1; else { console.log("Test 101_16e failed"); nerrs = nerrs + 1; }
if (nn101_16f == 107) ngood = ngood + 1; else { console.log("Test 101_16f failed"); nerrs = nerrs + 1; }
if (nn101_16g ==  25) ngood = ngood + 1; else { console.log("Test 101_16g failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
if (nerrs == 0) console.log("All", ngood, "tests successful.");
else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
////////////////////////////////////////////////////////////////////////
////    01/11/2022 - End
////////////////////////////////////////////////////////////////////////
let ii101_16 = 0, jj = 0, kk = 0, ll = 0, mm = 0, nn = 0, oo = 0, pp = 0;

if ((ii101_16 == 1 && jj == 1 || (kk == 1 || ll == 1 && mm == 1 && nn == 1) || oo == 1) && pp == 1) {
    console.log("is true");
} else {
    console.log("is false");
}
////////////////////////////////////////////////////////////////////////
////    01/11/2022 - Begin
////////////////////////////////////////////////////////////////////////
let nn101_18 = 0;

function getRectArea101_18a(width, height) {
    try {
        throw "getRectArea101_18b(width, height)";
        console.log("Test 101_18a failed");
        nerrs = nerrs + 1;
    } catch (err101_18a) {
        nn101_18 = nn101_18 + 1;
        throw err101_18a;
        console.log("Test 101_18b failed");
        nerrs = nerrs + 1;
    } finally {
        //  'debug show allvars';
        //console.log("Test 101_18 finally 1");
        nn101_18 = nn101_18 + 2;
    }
    console.log("Test 101_18c failed");
    nerrs = nerrs + 1;
}
try {
    getRectArea101_18a(3, 'A');
} catch (err101_18a) {
    //console.log("Test 101_18c failed, err=", err101_18a.toString());
    if (err101_18a.toString() == "getRectArea101_18b(width, height)") ngood = ngood + 1;
    else { console.log("Test 101_18d failed"); nerrs = nerrs + 1; }
    nn101_18 = nn101_18 + 4;
} finally {
    //console.log("nn101_18=", nn101_18);
    nn101_18 = nn101_18 + 8;
}
console.log("nn101_18=", nn101_18);
if (nn101_18 == 15) ngood = ngood + 1; else { console.log("Test101_18c failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
////    01/11/2022 - End
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////    01/15/2022 - Begin
////////////////////////////////////////////////////////////////////////
let m101_18 = 0, t101_18 = 0;
while (m101_18 < 8) {
    console.log("m101_18=", m101_18);
    try {
        try {
            t101_18 = t101_18 + 1;
            //console.log("Test 101_18 try");
            if (m101_18 & 1) throw "try throw";
            //console.log("Test 101_18 try - ERROR");
            t101_18 = t101_18 + 9;
        } catch (err101_18a) {
            t101_18 = t101_18 + 100;
            //console.log("Test 101_18 catch");
            if (m101_18 & 2) throw "catch throw";
            t101_18 = t101_18 + 900;
        } finally {
            t101_18 = t101_18 + 10000;
            //console.log("Test 101_18 finally");
            if (m101_18 & 4) throw "finally throw";
            t101_18 = t101_18 + 90000;
        }
    } catch (e101_18) {
        t101_18 = t101_18 + 1000000;
    } finally {
        t101_18 = t101_18 + 9000000;
    }

    m101_18 = m101_18 + 1;
}
console.log("t101_18=", t101_18);   // Expecting 78440444
console.log("Test 101_18 done");
////////////////////////////////////////////////////////////////////////
////    01/15/2022 - End
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////    01/18/2022 - Begin
////////////////////////////////////////////////////////////////////////
function fm101_19(nn) {
    let m101_19 = 0, t101_19 = 0;
    while (m101_19 < nn) {
        console.log("m101_19=", m101_19);
        try {
            try {
                t101_19 = t101_19 + 1;
                //console.log("Test 101_19 try");
                if (m101_19 & 1) throw "try throw";
                //console.log("Test 101_19 try - ERROR");
                t101_19 = t101_19 + 9;
            } catch (err101_19a) {
                t101_19 = t101_19 + 100;
                //console.log("Test 101_19 catch");
                if (m101_19 & 2) throw "catch throw";
                t101_19 = t101_19 + 900;
            } finally {
                t101_19 = t101_19 + 10000;
                //console.log("Test 101_19 finally");
                if (m101_19 & 4) throw "finally throw";
                t101_19 = t101_19 + 90000;
            }
        } catch (e101_19) {
            t101_19 = t101_19 + 1000000;
        } finally {
            t101_19 = t101_19 + 9000000;
        }

        m101_19 = m101_19 + 1;
    }
    return (t101_19);
}
let nn101_19 = fm101_19(8);
console.log("nn101_19=", nn101_19);   // Expecting 78440444
//if (nn101_19 == 77442244) ngood = ngood + 1; else { console.log("Test 101_19 failed"); nerrs = nerrs + 1; }
console.log("Test 101_19 done");
////////////////////////////////////////////////////////////////////////
////    01/18/2022 - End
////////////////////////////////////////////////////////////////////////
