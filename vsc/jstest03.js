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
////////////////////////////////////////////////////////////////////////
////    01/25/2022 - Begin
////////////////////////////////////////////////////////////////////////
function fm101_20(nn, ff) {
    var vv;
    if (nn & 1) vv = 2; else vv = 3;
    let pp = ff(nn + vv);
    return (pp);
}
let bb101_20 = fm101_20(4,
    function (mm) {
        function ss(ww, uu, vv) {
            var tt = 1;
            if (ww & 2) tt = tt + uu; else tt = tt + vv;
            const zz = tt * 2;
            return (zz + 1);
        }
        var xx = mm + 4, tt = 1;
        let ii = 0;
        while (ii < xx) {
            const jj = 10, kk = 55 + ii * 2;
            tt = tt + ss(ii, jj, kk);
            ii = ii + 1;
        }
        return (tt);
    });
console.log("bb101_20=", bb101_20); // bb101_20= 902
////////////////////////////////////////////////////////////////////////
////    01/25/2022 - End
////////////////////////////////////////////////////////////////////////
const name = {
    first: 'Bob',
    last: 'Smith'
};
console.log("name=", name);   // Expecting: name= {first: 'Bob', last: 'Smith'}
////////////////////////////////////////////////////////////////////////
////    02/09/2022 - Begin
////////////////////////////////////////////////////////////////////////
const util = require('util');

const aa = [11, ['aa', 'bb', 'cc'], 33];
const ss = aa.toString();

console.log("aa=", aa);         // Expecting: aa= (3) [11, Array(3), 33]
let bb = util.format("aa=", aa);
console.log("bb=", bb);         // Expecting: aa= [ 11, [ 'aa', 'bb', 'cc' ], 33 ]
console.log("ss=", ss);     // Expecting: ss= 11,aa,bb,cc,33
////////////////////////////////////////////////////////////////////////
////    02/09/2022 - End
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////    02/14/2022 - Begin
////////////////////////////////////////////////////////////////////////
let ii = 123, ss = 'One two three';
const aa = [11, ['aa', 'bb', 'cc'], 33];
const util = require('util');

console.log("ii=%d, aa[1][1]=%s", ii, aa[1][1]);    // Expected: ii=123, aa[1][1]=bb
console.log("ii=%d", ii, ss);                       // Expected: ii=123 One two three
console.log("aa=", aa);                             // Expected: aa= (3) [11, Array(3), 33]
console.log("aa.toString()=", aa.toString());       // Expected: aa.toString()= 11,aa,bb,cc,33
let ff1 = util.format("aa:%o", aa);
console.log("ff1=%s", ff1);                         // Expected: ff1=aa:[ 11, [ 'aa', 'bb', 'cc', [length]: 3 ], 33, [length]: 3 ]
let ff2 = util.format("aa:%O", aa);
console.log("ff2=%s", ff2);                         // Expected: ff2=aa:[ 11, [ 'aa', 'bb', 'cc' ], 33 ]
let ff3 = util.format("aa:%j", aa);
console.log("ff3=%s", ff3);                         // Expected: ff3=aa:[11,["aa","bb","cc"],33]
let ff4 = util.format("aa:%s", aa);
console.log("ff4=%s", ff4);                         // Expected: ff4=aa:[11,["aa","bb","cc"],33]
let ff5 = util.format("aa:", aa);
console.log("ff5=%s", ff5);                         // Expected: ff4=aa:[ 11, [Array], 33 ]
console.log("ii=%04d ii=%-4d", ii, ii);             // Expected: ii=%04d ii=%-4d 123 123
console.log("aa=%s", aa);                           // Expected: Array(3)
console.log("ii=%s", ii);                           // Expected: ii=123
console.log("ss=%d", ss);                           // Expected: ss=NaN
//  ii=123, aa[1][1]=bb
//  ii=123 One two three
//  aa= (3) [11, Array(3), 33]
//  aa.toString()= 11,aa,bb,cc,33
//  ff1=aa:[ 11, [ 'aa', 'bb', 'cc', [length]: 3 ], 33, [length]: 3 ]
//  ff2=aa:[ 11, [ 'aa', 'bb', 'cc' ], 33 ]
//  ff3=aa:[11,["aa","bb","cc"],33]
//  ff4=aa:[ 11, [Array], 33 ]
//  ff5=aa: [ 11, [ 'aa', 'bb', 'cc' ], 33 ]
//  ii=%04d ii=%-4d 123 123
//  aa=Array(3)
//  ii=123
//  ss=NaN
////////////////////////////////////////////////////////////////////////
////    02/14/2022 - End
////////////////////////////////////////////////////////////////////////
function format_all(vv) {
    console.log("log    vv    =", vv);
    console.log("log    vv(%%d)=%d", vv);
    console.log("log    vv(%%f)=%f", vv);
    console.log("log    vv(%%i)=%i", vv);
    console.log("log    vv(%%j)=%j", vv);
    console.log("log    vv(%%o)=%o", vv);
    console.log("log    vv(%%O)=%O", vv);
    console.log("log    vv(%%s)=%s", vv);
    console.log("log    vv(%%v)=%v-", vv);
    console.log("log    vv.toString()=%s", vv.toString());

    var ff;
    ff = util.format("vv    =", vv);
    console.log("format %s", ff);
    ff = util.format("vv(%%d)=%d", vv);
    console.log("format %s", ff);
    ff = util.format("vv(%%f)=%f", vv);
    console.log("format %s", ff);
    ff = util.format("vv(%%i)=%i", vv);
    console.log("format %s", ff);
    ff = util.format("vv(%%j)=%j", vv);
    console.log("format %s", ff);
    ff = util.format("vv(%%o)=%o", vv);
    console.log("format %s", ff);
    ff = util.format("vv(%%O)=%O", vv);
    console.log("format %s", ff);
    ff = util.format("vv(%%s)=%s", vv);
    console.log("format %s", ff);
    ff = util.format("vv(%%v)=%v-", vv);
    console.log("format %s", ff);
    ff = util.format("vv.toString()=%s", vv.toString());
    console.log("format %s", ff);

    //'debug show stacktrace';
}
//format_all(123);
format_all('Abc');
//const aa = [11, ['aa', 'bb', 'cc'], 33];
//format_all(aa);
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////    02/18/2022 - Begin
////////////////////////////////////////////////////////////////////////
////////////////////////////////
const util = require('util');

////////////////
////////////////
var ngood = 0;
var nerrs = 0;

////////////////////////////////////////////////////////////////////////
// Test 101_26 - until.format() array elements and others
var ff101_26, ee101_26;
//const aa101_26 = [ [0, 2, 2], [3, 4], [ [5, 6], 'yy', 'zz'], 33];
// Test 101_26i     - No format
ff101_26 = util.format("aa    =", aa101_26);
ee101_26 = "aa    = [ [ 0, 2, 2 ], [ 3, 4 ], [ [ 5, 6 ], 'yy', 'zz' ], 33 ]";
if (ff101_26 == ee101_26) ngood = ngood + 1; else { console.log("Test 101_26i failed"); nerrs = nerrs + 1; }
// Test 101_26i     - No format aa101_26[2]
ff101_26 = util.format("aa    =", aa101_26[2]);
ee101_26 = "aa    = [ [ 5, 6 ], 'yy', 'zz' ]";
if (ff101_26 == ee101_26) ngood = ngood + 1; else { console.log("Test 101_26i failed"); nerrs = nerrs + 1; }
////
console.log("ff <%s>", ff101_26);
console.log("ee <%s>", ee101_26);
////////////////////////////////////////////////////////////////////////
if (nerrs == 0) console.log("All", ngood, "tests successful.");
else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
////    02/18/2022 - End
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////    02/21/2022 - Begin
////////////////////////////////////////////////////////////////////////
////////////////
const util = require('util');
var ngood = 0;
var nerrs = 0;

////////////////////////////////////////////////////////////////////////
// Test 101_26 - until.format() array elements and others
var ff101_26, ee101_26;
const aa101_26 = [ [0, 2, 'alpha'], [3, 4], [ [5, 6], 'yy', 'zz'], 33];
// Test 101_26i     - No format
ff101_26 = util.format("aa    =", aa101_26);
ee101_26 = "aa    = [ [ 0, 2, 'alpha' ], [ 3, 4 ], [ [ 5, 6 ], 'yy', 'zz' ], 33 ]";
if (ff101_26 == ee101_26) ngood = ngood + 1; else { console.log("Test 101_26i failed"); nerrs = nerrs + 1; }
// Test 101_26ii    - No format aa101_26[2]
ff101_26 = util.format("aa    =", aa101_26[2]);
ee101_26 = "aa    = [ [ 5, 6 ], 'yy', 'zz' ]";
if (ff101_26 == ee101_26) ngood = ngood + 1; else { console.log("Test 101_26ii failed"); nerrs = nerrs + 1; }
// Test 101_26iii   - No format aa101_26[aa101_26[0][1]]
ff101_26 = util.format("aa    =", aa101_26[aa101_26[0][1]]);
ee101_26 = "aa    = [ [ 5, 6 ], 'yy', 'zz' ]";
if (ff101_26 == ee101_26) ngood = ngood + 1; else { console.log("Test 101_26iii failed"); nerrs = nerrs + 1; }
// Test 101_26iv    - No format aa101_26[aa101_26[1][aa101_26[0][0]] - aa101_26[0][1]]
ff101_26 = util.format("aa    =%j.", aa101_26[aa101_26[1][aa101_26[0][0]] - aa101_26[0][1]]);
ee101_26 = "aa    =[3,4].";
if (ff101_26 == ee101_26) ngood = ngood + 1; else { console.log("Test 101_26iv failed"); nerrs = nerrs + 1; }
// Test 101_26v     - No format aa101_26[aa101_26[aa101_26[1][0] - aa101_26[0][1]][0] - aa101_26[0][1]][1]
ff101_26 = util.format("aa    =%d.", aa101_26[aa101_26[aa101_26[1][0] - aa101_26[0][1]][0] - aa101_26[0][1]][1]);
ee101_26 = "aa    =4.";
if (ff101_26 == ee101_26) ngood = ngood + 1; else { console.log("Test 101_26iv failed"); nerrs = nerrs + 1; }
////
console.log("ff <%s>", ff101_26);
console.log("ee <%s>", ee101_26);
////////////////////////////////////////////////////////////////////////
if (nerrs == 0) console.log("All", ngood, "tests successful.");
else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
console.log("--end jstest02.js tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
////    02/21/2022 - End
////////////////////////////////////////////////////////////////////////
jj = ii++ * 2;
console.log("ii=", ii, "jj=", jj);  // Exp: ii= 3 jj= 4
////////////////////////////////////////////////////////////////////////
////    03/03/2022 - Begin
////////////////////////////////////////////////////////////////////////
let yy = { first: "Taurus", last: "Software" };
console.log("yy=", yy);             // Exp: yy= {first: 'Taurus', last: 'Software'}
let aa = 11;
let bb = 'two';
let cc = [44, 55];
let ww = {aa, bb, cc};
console.log("ww=", ww);             // Exp: ww= {aa: 11, bb: 'two', cc: Array(2)}
bb = 'two three';
cc[2] = 66;
console.log("ww.cc=", ww.cc);       // Exp: ww.cc= (3) [44, 55, 66]
////////////////////////////////////////////////////////////////////////
////    03/03/2022 - End
////////////////////////////////////////////////////////////////////////
const util = require('util');
let yy = { first: "Taurus", last: "Software", list : ['one','two'] };
let ss = util.format("yy=%j", yy);
console.log("ss=", ss);     // Exp: ss= yy={"first":"Taurus","last":"Software","list":["one","two"]}
console.log("yy.first=", yy.first);

function ff(xx) {
    return xx * 2;
}
function gg(yy, nn) {
    'debug show allvars';
    return yy(nn) + 1;
}
let mm = gg(ff, 4);
@@
////    03/11/2022 - Begin

////////
var ngood = 0;
var nerrs = 0;
////////////////

//function fxparent_101_28(vv) { return (vv); }

function ff(varr) {
    var aa = varr[1].toString();
    'debug show allvars';
}
ff(['one', ['two', 'beta'], 'three']);

let yy = { first: "Taurus", last: "Software" };
//console.log("yy=", yy);             // Exp: yy= {first: 'Taurus', last: 'Software'}
let ff = yy;
//let ll = yy.last;
'debug show allvars';
//console.log("ff=", ff, "ll=", ll);

////////////////////////////////////////////////////////////////////////
if (nerrs == 0) console.log("All", ngood, "tests successful.");
else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);

////////////////////////////////////////////////////////////////////////
////    08/01/2022
////////////////////////////////////////////////////////////////////////
var book = new Object( );
book.title = "JavaScript: The Definitive Guide";
book.chapter1 = new Object();
book.chapter1.title = "Introduction to JavaScript";
book.chapter1.pages = 19;
book.chapter2 = { title: "Lexical Structure", pages: 6 };
console.log(book);
////////////////////////////////////////////////////////////////////////
var ss = new Number(23);
var ii = ss;
ss = ss + 2;
console.log("ii=", ii, "typeof ii=", typeof ii);
console.log("ss=", ss, "typeof ss=", typeof ss);
////////////////////////////////////////////////////////////////////////
////    08/24/2022
////////////////////////////////////////////////////////////////////////
function Car(make, model, year) {
    function Engine(typ, num) {
        console.log("Enter Engine()");
        this.engType = typ;
        this.engNum = num;
    }
    console.log("Enter Car()");
    new Engine('piston', 6);
    this.make = make;
    this.model = model;
    this.year = year;
    var make = "BMW";

    make = make + "Ford";
    this.make = this.make + "Honda";

    console.log("make=", make);             // Exp: make= BMWFord
    console.log("this.make=", this.make);   // Exp: this.make= EagleHonda
}

const car1 = new Car('Eagle', 'Talon TSi', 1993);
console.log("car1.make=", car1.make);       // Exp: car1.make= EagleHonda
////////////////////////////////////////////////////////////////////////
////    10/03/2022
////////////////////////////////////////////////////////////////////////
var ss = "Alpha";
let tt = ss.concat("Beta", ";");
String.prototype.concat = function (str, delim) {
    var out;
    if (delim == undefined) {
        out = this + str;
    } else {
        out = this + delim;
        out = out + str;
    }
    return (out);
}
let uu = tt.concat("Gamma", ",");
console.log("ss=", ss, "tt=", tt, "uu=", uu);   // Exp: ss= Alpha tt= AlphaBeta; uu= AlphaBeta;,Gamma
////////////////////////////////////////////////////////////////////////
String.prototype.cat = 16;
var tt = "Alpha";

console.log("String.prototype.cat=", String.prototype.cat); // Exp: String.prototype.cat= 16
console.log("String.cat=", String.cat);                     // Exp: String.cat= undefined
console.log("tt.cat=", tt.cat);                             // Exp: tt.cat= 16
console.log("'abc'.cat=", 'abc'.cat);                       // Exp: 'abc'.cat= 16
////////////////////////////////////////////////////////////////////////
//  var tt = "Alpha";
//  console.log("tt.cat=", tt.cat);                             // Exp: tt.cat= 16
//  console.log("'abc'.cat=", 'abc'.cat);                       // Exp: 'abc'.cat= 16
////////////////////////////////
//  String.prototype.cat = function (str, delim) {
//      var out;
//      out = this + delim;
//      out = out + str;
//  
//      return (out);
//  }
//  var ss = "Alpha";
//  let tt = ss.cat("Beta", ";");
//  console.log("ss=", ss, "tt=", tt);  // Exp: ss= Alpha tt= Alpha;Beta
////////////////////////////////////////////////////////////////////////
////    01/16/2023
////////////////////////////////////////////////////////////////////////
var ss = "Alpha";
String.prototype.cat = 16;
String.cat = 32;
String.prototype.prototype = 64;
//  ss.beta = 4;        ERROR!
//  "Abc".gamma = 8;    ERROR!

//console.log("String.prototype=", String.prototype);         // Exp: String.prototype= String ('')
//101_36a//console.log("String.prototype.cat=", String.prototype.cat); // Exp: String.prototype.cat= 16
console.log("String.cat=", String.cat);                     // Exp: String.cat= 32
console.log("String.car=", String.car);                     // Exp: String.car= undefined
console.log("String.length=", String.length);               // Exp: String.length= 1
console.log("String.prototype.prototype=", String.prototype.prototype); // Exp: String.prototype.prototype= 64
console.log("--------");
console.log('ss.prototype=', ss.prototype);                 // Exp: ss.prototype= 64
console.log('ss.prototype.cat=', ss.prototype.cat);         // Exp: ss.prototype.cat= undefined
console.log('ss.cat=', ss.cat);                             // Exp: ss.cat= 16
console.log('ss.car=', ss.car);                             // Exp: ss.car= undefined
console.log('ss.length=', ss.length);                       // Exp: ss.length= 5
console.log('ss.prototype=', ss.prototype);                 // Exp: ss.prototype= 64
console.log("--------");
console.log('"Abc".prototype=', "Abc".prototype);           // Exp: "Abc".prototype= 64
console.log('"Abc".prototype.cat=', "Abc".prototype.cat);   // Exp: "Abc".prototype.cat= undefined
console.log('"Abc".cat=', "Abc".cat);                       // Exp: "Abc".cat= 16
console.log('"Abc".car=', "Abc".car);                       // Exp: "Abc".car= undefined
console.log('"Abc".length=', "Abc".length);                 // Exp: "Abc".length= 3
console.log('"Abc".prototype=', "Abc".prototype);           // Exp: "Abc".prototype= 64
////////////////////////////////////////////////////////////////////////
