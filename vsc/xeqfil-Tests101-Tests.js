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
var printf = false;
//printf = true;

////////////////////////////////////////////////////////////////////////
if (printf) console.log("Starting test 101-1...");
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
if (printf) console.log("Starting test 101-4...");
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
if (printf) console.log("Starting test 101-5...");
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
if (printf) console.log("Starting test 101-6...");
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
if (printf) console.log("Starting test 101-10...");
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
if (printf) console.log("Starting test 101-11...");
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
if (printf) console.log("Starting test 101-12...");
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
if (printf) console.log("Starting test 101-13...");
function getRectArea101_13a(width) {
    nn101_13 = nn101_13 + width;
}
let nn101_13 = 8;
nn101_13 = nn101_13 + 1;
getRectArea101_13a(2);
nn101_13 = nn101_13 + 4;
if (nn101_13 == 15) ngood = ngood + 1; else { console.log("Test 101-13 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_14  - 01/04/2022
if (printf) console.log("Starting test 101-14...");
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
if (printf) console.log("Starting test 101-15...");
let nn101_15 = 0;
var ii101_15 = 0;

while (ii101_15 < 8) {
    if (ii101_15 == 1 || ii101_15 == 3 || ii101_15 == 5 || ii101_15 == 7) {
        function ff101_15() {
            nn101_15 = nn101_15 + 1000 * ii101_15;
        }
        ff101_15();
    } else {
        function ff101_15() {
            nn101_15 = nn101_15 + ii101_15;
        }
        ff101_15();
    }
    ii101_15 = ii101_15 + 1;
}
if (nn101_15 == 16012) ngood = ngood + 1; else { console.log("Test 101-15 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_16  - 01/11/2022
if (printf) console.log("Starting test 101-16...");
let nn101_16a = 0;
let nn101_16b = 0;
let nn101_16c = 0;
let nn101_16d = 0;
let nn101_16e = 0;
let nn101_16f = 0;
let nn101_16g = 0;
let nn101_16h = 0;
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
                                if ((ii101_16 == 1 && jj == 1 || (kk == 1 || ll == 1 && mm == 1 && nn == 1) || oo == 1) && pp == 1) {
                                    nn101_16f = nn101_16f + 1;
                                }
                                if (ii101_16 == 1 && (jj == 1 && kk == 1 || ll == 1) && (mm == 1 || nn == 1 && oo == 1) && pp == 1) {
                                    nn101_16g = nn101_16g + 1;
                                }
                                if (((ii101_16 == 1 || jj == 1 && kk == 1) || ll == 1 && mm == 1 || nn == 1) && oo == 1 || pp == 1) {
                                    nn101_16h = nn101_16h + 1;
                                }
                                ////
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
if (nn101_16a == 255) ngood = ngood + 1; else { console.log("Test 101_16a failed"); nerrs = nerrs + 1; }
if (nn101_16b ==  67) ngood = ngood + 1; else { console.log("Test 101_16b failed"); nerrs = nerrs + 1; }
if (nn101_16c == 211) ngood = ngood + 1; else { console.log("Test 101_16c failed"); nerrs = nerrs + 1; }
if (nn101_16d == 193) ngood = ngood + 1; else { console.log("Test 101_16d failed"); nerrs = nerrs + 1; }
if (nn101_16e == 223) ngood = ngood + 1; else { console.log("Test 101_16e failed"); nerrs = nerrs + 1; }
if (nn101_16f == 107) ngood = ngood + 1; else { console.log("Test 101_16f failed"); nerrs = nerrs + 1; }
if (nn101_16g ==  25) ngood = ngood + 1; else { console.log("Test 101_16g failed"); nerrs = nerrs + 1; }
if (nn101_16h == 183) ngood = ngood + 1; else { console.log("Test 101_16h failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_17  - 01/13/2022
if (printf) console.log("Starting test 101-17...");
let nn101_17 = 0;

try {
    throw "getRectArea101_17b(width, height)";
    console.log("Test 101_17b failed");
    nerrs = nerrs + 1;
} catch (err101_17a) {
    nn101_17 = nn101_17 + 1;
} finally {
    nn101_17 = nn101_17 + 2;
}
if (nn101_17 == 3) ngood = ngood + 1; else { console.log("Test 101_17 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_18  - 01/18/2022
if (printf) console.log("Starting test 101-18...");
let m101_18 = 0, t101_18 = 0;
while (m101_18 < 8) {
    try {
        try {
            t101_18 = t101_18 + 1;
            if (m101_18 & 1) throw "try throw";
            t101_18 = t101_18 + 9;
        } catch (err101_18a) {
            t101_18 = t101_18 + 100;
            if (m101_18 & 2) throw "catch throw";
            t101_18 = t101_18 + 900;
        } finally {
            t101_18 = t101_18 + 10000;
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
if (t101_18 == 77442244) ngood = ngood + 1; else { console.log("Test 101_18 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_19  - 01/19/2022
if (printf) console.log("Starting test 101-19...");
function fm101_19a(mm) {
    let tt = 0;
    try {
        tt = tt + 1;
        if (mm & 1) throw "try throw";
        tt = tt + 9;
    } catch (err101_19a) {
        tt = tt + 100;
        if (mm & 2) throw "catch throw";
        tt = tt + 900;
    } finally {
        tt = tt + 10000;
        if (mm & 4) throw "finally throw";
        tt = tt + 90000;
    }
    tt = tt + 12345;

    return (tt);
}
function fm101_19b(nn) {
    let mm = 0, tt = 0;
    while (mm < nn) {
        try {
            tt = tt + fm101_19a(mm);
        } catch (e101_19) {
            tt = tt + 1000000;
        } finally {
            tt = tt + 9000000;
        }

        mm = mm + 1;
    }
    return (tt);
}
let nn101_19 = fm101_19b(8);
if (nn101_19 == 77338056) ngood = ngood + 1; else { console.log("Test 101_19 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_20  - 01/29/2022
if (printf) console.log("Starting test 101-20...");
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
if (bb101_20 == 902) ngood = ngood + 1; else { console.log("Test 101_20 failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_21 - 101_25  - 02/17/2022
////////////////////////////////////////////////////////////////////////
const util = require('util');
////////////////////////////////
////////////////////////////////////////////////////////////////////////
// Test 101_21 - until.format() array with nested array
if (printf) console.log("Starting test 101-21...");
var ff101_21, ee101_21;
const aa101_21 = [11, ['aa', 'bb', 'cc'], 33];
// Test 101_21i     - No format
ff101_21 = util.format("aa    =", aa101_21);
ee101_21 = "aa    = [ 11, [ 'aa', 'bb', 'cc' ], 33 ]";
if (ff101_21 == ee101_21) ngood = ngood + 1; else { console.log("Test 101_21i failed"); nerrs = nerrs + 1; }
// Test 101_21ii    - format %j
ff101_21 = util.format("aa    =%j.", aa101_21);
ee101_21 = 'aa    =[11,["aa","bb","cc"],33].';
if (ff101_21 == ee101_21) ngood = ngood + 1; else { console.log("Test 101_21ii failed"); nerrs = nerrs + 1; }
// Test 101_21iii   - format %o
ff101_21 = util.format("aa    =%o.", aa101_21);
ee101_21 = "aa    =[ 11, [ 'aa', 'bb', 'cc', [length]: 3 ], 33, [length]: 3 ].";
if (ff101_21 == ee101_21) ngood = ngood + 1; else { console.log("Test 101_21iii failed"); nerrs = nerrs + 1; }
// Test 101_21iv    - format %O
ff101_21 = util.format("aa    =%O.", aa101_21);
ee101_21 = "aa    =[ 11, [ 'aa', 'bb', 'cc' ], 33 ].";
if (ff101_21 == ee101_21) ngood = ngood + 1; else { console.log("Test 101_21iv failed"); nerrs = nerrs + 1; }
// Test 101_21v     - format %s
ff101_21 = util.format("aa    =%s.", aa101_21);
ee101_21 = "aa    =[ 11, [Array], 33 ].";
if (ff101_21 == ee101_21) ngood = ngood + 1; else { console.log("Test 101_21v failed"); nerrs = nerrs + 1; }
// Test 101_21vi    - format %d
ff101_21 = util.format("aa    =%d.", aa101_21);
ee101_21 = "aa    =NaN.";
if (ff101_21 == ee101_21) ngood = ngood + 1; else { console.log("Test 101_21vi failed"); nerrs = nerrs + 1; }
// Test 101_21vii   - toString()
ff101_21 = util.format("aa    =%s.", aa101_21.toString());
ee101_21 = "aa    =11,aa,bb,cc,33.";
if (ff101_21 == ee101_21) ngood = ngood + 1; else { console.log("Test 101_21vii failed"); nerrs = nerrs + 1; }
// Test 101_21viii  - format %i
ff101_21 = util.format("aa    =%i.", aa101_21);
ee101_21 = 'aa    =11.';
if (ff101_21 == ee101_21) ngood = ngood + 1; else { console.log("Test 101_21viii failed"); nerrs = nerrs + 1; }
// Test 101_21ix    - format %f
ff101_21 = util.format("aa    =%f.", aa101_21);
ee101_21 = 'aa    =11.';
if (ff101_21 == ee101_21) ngood = ngood + 1; else { console.log("Test 101_21ix failed"); nerrs = nerrs + 1; }
///////
//console.log("ff <%s>", ff101_21);
//console.log("ee <%s>", ee101_21);
//  ////////////////////////////////////////////////////////////////////////
// Test 101_22 - until.format() array with one integer element
if (printf) console.log("Starting test 101-22...");
var ff101_22, ee101_22;
const aa101_22 = [6789];
// Test 101_22i     - No format
ff101_22 = util.format("aa    =", aa101_22);
ee101_22 = "aa    = [ 6789 ]";
if (ff101_22 == ee101_22) ngood = ngood + 1; else { console.log("Test 101_22i failed"); nerrs = nerrs + 1; }
// Test 101_22ii    - format %j
ff101_22 = util.format("aa    =%j.", aa101_22);
ee101_22 = 'aa    =[6789].';
if (ff101_22 == ee101_22) ngood = ngood + 1; else { console.log("Test 101_22ii failed"); nerrs = nerrs + 1; }
// Test 101_22iii   - format %o
ff101_22 = util.format("aa    =%o.", aa101_22);
ee101_22 = "aa    =[ 6789, [length]: 1 ].";
if (ff101_22 == ee101_22) ngood = ngood + 1; else { console.log("Test 101_22iii failed"); nerrs = nerrs + 1; }
// Test 101_22iv    - format %O
ff101_22 = util.format("aa    =%O.", aa101_22);
ee101_22 = "aa    =[ 6789 ].";
if (ff101_22 == ee101_22) ngood = ngood + 1; else { console.log("Test 101_22iv failed"); nerrs = nerrs + 1; }
// Test 101_22v     - format %s
ff101_22 = util.format("aa    =%s.", aa101_22);
ee101_22 = "aa    =[ 6789 ].";
if (ff101_22 == ee101_22) ngood = ngood + 1; else { console.log("Test 101_22v failed"); nerrs = nerrs + 1; }
// Test 101_22vi    - format %d
ff101_22 = util.format("aa    =%d.", aa101_22);
ee101_22 = 'aa    =6789.';
if (ff101_22 == ee101_22) ngood = ngood + 1; else { console.log("Test 101_22vi failed"); nerrs = nerrs + 1; }
// Test 101_22vii   - toString()
ff101_22 = util.format("aa    =%s.", aa101_22.toString());
ee101_22 = 'aa    =6789.';
if (ff101_22 == ee101_22) ngood = ngood + 1; else { console.log("Test 101_22vii failed"); nerrs = nerrs + 1; }
// Test 101_22viii  - format %i
ff101_22 = util.format("aa    =%i.", aa101_22);
ee101_22 = 'aa    =6789.';
if (ff101_22 == ee101_22) ngood = ngood + 1; else { console.log("Test 101_22viii failed"); nerrs = nerrs + 1; }
// Test 101_22ix    - format %f
ff101_22 = util.format("aa    =%f.", aa101_22);
ee101_22 = 'aa    =6789.';
if (ff101_22 == ee101_22) ngood = ngood + 1; else { console.log("Test 101_22ix failed"); nerrs = nerrs + 1; }
///////
//console.log("ff <%s>", ff101_22);
//console.log("ee <%s>", ee101_22);
////////////////////////////////////////////////////////////////////////
// Test 101_23 - until.format() array with one string element
if (printf) console.log("Starting test 101-23...");
var ff101_23, ee101_23;
const aa101_23 = ['Alpha'];
// Test 101_23i     - No format
ff101_23 = util.format("aa    =", aa101_23);
ee101_23 = "aa    = [ 'Alpha' ]";
if (ff101_23 == ee101_23) ngood = ngood + 1; else { console.log("Test 101_23i failed"); nerrs = nerrs + 1; }
// Test 101_23ii    - format %j
ff101_23 = util.format("aa    =%j.", aa101_23);
ee101_23 = 'aa    =["Alpha"].';
if (ff101_23 == ee101_23) ngood = ngood + 1; else { console.log("Test 101_23ii failed"); nerrs = nerrs + 1; }
// Test 101_23iii   - format %o
ff101_23 = util.format("aa    =%o.", aa101_23);
ee101_23 = "aa    =[ 'Alpha', [length]: 1 ].";
if (ff101_23 == ee101_23) ngood = ngood + 1; else { console.log("Test 101_23iii failed"); nerrs = nerrs + 1; }
// Test 101_23iv    - format %O
ff101_23 = util.format("aa    =%O.", aa101_23);
ee101_23 = "aa    =[ 'Alpha' ].";
if (ff101_23 == ee101_23) ngood = ngood + 1; else { console.log("Test 101_23iv failed"); nerrs = nerrs + 1; }
// Test 101_23vi    - format %d
ff101_23 = util.format("aa    =%d.", aa101_23);
ee101_23 = 'aa    =NaN.';
if (ff101_23 == ee101_23) ngood = ngood + 1; else { console.log("Test 101_23vi failed"); nerrs = nerrs + 1; }
// Test 101_23vii   - toString()
ff101_23 = util.format("aa    =%s.", aa101_23.toString());
ee101_23 = "aa    =Alpha.";
if (ff101_23 == ee101_23) ngood = ngood + 1; else { console.log("Test 101_23vii failed"); nerrs = nerrs + 1; }
// Test 101_23viii  - format %i
ff101_23 = util.format("aa    =%i.", aa101_23);
ee101_23 = 'aa    =NaN.';
if (ff101_23 == ee101_23) ngood = ngood + 1; else { console.log("Test 101_23viii failed"); nerrs = nerrs + 1; }
// Test 101_23ix    - format %f
ff101_23 = util.format("aa    =%f.", aa101_23);
ee101_23 = 'aa    =NaN.';
if (ff101_23 == ee101_23) ngood = ngood + 1; else { console.log("Test 101_23ix failed"); nerrs = nerrs + 1; }
////
//console.log("ff <%s>", ff101_23);
//console.log("ee <%s>", ee101_23);
////////////////////////////////////////////////////////////////////////
// Test 101_24 - until.format() array with zero elements
if (printf) console.log("Starting test 101-24...");
var ff101_24, ee101_24;
const aa101_24 = [];
// Test 101_24i     - No format
ff101_24 = util.format("aa    =", aa101_24);
ee101_24 = "aa    = []";
if (ff101_24 == ee101_24) ngood = ngood + 1; else { console.log("Test 101_24i failed"); nerrs = nerrs + 1; }
// Test 101_24ii    - format %j
ff101_24 = util.format("aa    =%j.", aa101_24);
ee101_24 = 'aa    =[].';
if (ff101_24 == ee101_24) ngood = ngood + 1; else { console.log("Test 101_24ii failed"); nerrs = nerrs + 1; }
// Test 101_24iii   - format %o
ff101_24 = util.format("aa    =%o.", aa101_24);
ee101_24 = "aa    =[ [length]: 0 ].";
if (ff101_24 == ee101_24) ngood = ngood + 1; else { console.log("Test 101_24iii failed"); nerrs = nerrs + 1; }
// Test 101_24iv    - format %O
ff101_24 = util.format("aa    =%O.", aa101_24);
ee101_24 = 'aa    =[].';
if (ff101_24 == ee101_24) ngood = ngood + 1; else { console.log("Test 101_24iv failed"); nerrs = nerrs + 1; }
// Test 101_24v     - format %s
ff101_24 = util.format("aa    =%s.", aa101_24);
ee101_24 = 'aa    =[].';
if (ff101_24 == ee101_24) ngood = ngood + 1; else { console.log("Test 101_24v failed"); nerrs = nerrs + 1; }
// Test 101_24vi    - format %d
ff101_24 = util.format("aa    =%d.", aa101_24);
ee101_24 = 'aa    =0.';
if (ff101_24 == ee101_24) ngood = ngood + 1; else { console.log("Test 101_24vi failed"); nerrs = nerrs + 1; }
// Test 101_24vii   - toString()
ff101_24 = util.format("aa    =%s.", aa101_24.toString());
ee101_24 = 'aa    =.';
if (ff101_24 == ee101_24) ngood = ngood + 1; else { console.log("Test 101_24vii failed"); nerrs = nerrs + 1; }
// Test 101_24viii  - format %i
ff101_24 = util.format("aa    =%i.", aa101_24);
ee101_24 = 'aa    =NaN.';
if (ff101_24 == ee101_24) ngood = ngood + 1; else { console.log("Test 101_24viii failed"); nerrs = nerrs + 1; }
// Test 101_24ix    - format %f
ff101_24 = util.format("aa    =%f.", aa101_24);
ee101_24 = 'aa    =NaN.';
if (ff101_24 == ee101_24) ngood = ngood + 1; else { console.log("Test 101_24ix failed"); nerrs = nerrs + 1; }
////
//console.log("ff <%s>", ff101_24);
//console.log("ee <%s>", ee101_24);
////////////////////////////////////////////////////////////////////////
// Test 101_25 - until.format() array with some undefined elements
if (printf) console.log("Starting test 101-25...");
var ff101_25, ee101_25;
let aa101_25 = [[4567, 'bb'], 33];
aa101_25.length = 6;
aa101_25[5] = 'Ab';
// Test 101_25i     - No format
ff101_25 = util.format("aa    =", aa101_25);
ee101_25 = "aa    = [ [ 4567, 'bb' ], 33, <3 empty items>, 'Ab' ]";
if (ff101_25 == ee101_25) ngood = ngood + 1; else { console.log("Test 101_25i failed"); nerrs = nerrs + 1; }
// Test 101_25ii    - format %j
ff101_25 = util.format("aa    =%j.", aa101_25);
ee101_25 = 'aa    =[[4567,"bb"],33,null,null,null,"Ab"].';
if (ff101_25 == ee101_25) ngood = ngood + 1; else { console.log("Test 101_25ii failed"); nerrs = nerrs + 1; }
// Test 101_25iii   - format %o
ff101_25 = util.format("aa    =%o.", aa101_25);
ee101_25 = "aa    =[ [ 4567, 'bb', [length]: 2 ], 33, <3 empty items>, 'Ab', [length]: 6 ].";
if (ff101_25 == ee101_25) ngood = ngood + 1; else { console.log("Test 101_25iii failed"); nerrs = nerrs + 1; }
// Test 101_25iv    - format %O
ff101_25 = util.format("aa    =%O.", aa101_25);
ee101_25 = "aa    =[ [ 4567, 'bb' ], 33, <3 empty items>, 'Ab' ].";
if (ff101_25 == ee101_25) ngood = ngood + 1; else { console.log("Test 101_25iv failed"); nerrs = nerrs + 1; }
// Test 101_25v     - format %s
ff101_25 = util.format("aa    =%s.", aa101_25);
ee101_25 = "aa    =[ [Array], 33, <3 empty items>, 'Ab' ].";
if (ff101_25 == ee101_25) ngood = ngood + 1; else { console.log("Test 101_25v failed"); nerrs = nerrs + 1; }
// Test 101_25vi    - format %d
ff101_25 = util.format("aa    =%d.", aa101_25);
ee101_25 = "aa    =NaN.";
if (ff101_25 == ee101_25) ngood = ngood + 1; else { console.log("Test 101_25vi failed"); nerrs = nerrs + 1; }
// Test 101_25vii   - toString()
ff101_25 = util.format("aa    =%s.", aa101_25.toString());
ee101_25 = "aa    =4567,bb,33,,,,Ab.";
if (ff101_25 == ee101_25) ngood = ngood + 1; else { console.log("Test 101_25vii failed"); nerrs = nerrs + 1; }
// Test 101_25viii  - format %i
ff101_25 = util.format("aa    =%i.", aa101_25);
ee101_25 = 'aa    =4567.';
if (ff101_25 == ee101_25) ngood = ngood + 1; else { console.log("Test 101_25viii failed"); nerrs = nerrs + 1; }
// Test 101_25ix    - format %f
ff101_25 = util.format("aa    =%f.", aa101_25);
ee101_25 = 'aa    =4567.';
if (ff101_25 == ee101_25) ngood = ngood + 1; else { console.log("Test 101_25ix failed"); nerrs = nerrs + 1; }
////
//console.log("ff <%s>", ff101_25);
//console.log("ee <%s>", ee101_25);
////////////////////////////////////////////////////////////////////////
// Test 101_26 - until.format() array elements and others
if (printf) console.log("Starting test 101-26...");
var ff101_26, ee101_26;
const aa101_26 = [[0, 2, 'alpha'], [3, 4], [[5, 6], 'yy', 'zz'], 33];
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
if (ff101_26 == ee101_26) ngood = ngood + 1; else { console.log("Test 101_26v failed"); nerrs = nerrs + 1; }
// Test 101_26vi    - %j format aa101_26[aa101_26[0][1]][aa101_26[0][0]]
ff101_26 = util.format("aa    =%j.", aa101_26[aa101_26[0][1]][aa101_26[0][0]]);
ee101_26 = "aa    =[5,6].";
if (ff101_26 == ee101_26) ngood = ngood + 1; else { console.log("Test 101_26vi failed"); nerrs = nerrs + 1; }
// Test 101_26vii   - %d format aa101_26[aa101_26[aa101_26[0][0]][1]][aa101_26[0][0]][1 + aa101_26[0][0]]
ff101_26 = util.format("aa    =%d.", aa101_26[aa101_26[aa101_26[0][0]][1]][aa101_26[0][0]][1 + aa101_26[0][0]]);
ee101_26 = "aa    =6.";
if (ff101_26 == ee101_26) ngood = ngood + 1; else { console.log("Test 101_26vii failed"); nerrs = nerrs + 1; }
////
//console.log("ff <%s>", ff101_26);
//console.log("ee <%s>", ee101_26);
////////////////////////////////////////////////////////////////////////
// Test 101_27 - 02/23/2022
////////////////////////////////////////////////////////////////////////
if (printf) console.log("Starting test 101-27...");
function ff101_27(two, three) {
    if (two ** (three * (two + 1)) - 500 == -(two ** three) + 20) ngood = ngood + 1; else { console.log("Test ff101_27i failed"); nerrs = nerrs + 1; }
    if (8 + three * two ** three ** two * two - 80 == 3000) ngood = ngood + 1; else { console.log("Test ff101_27ii failed"); nerrs = nerrs + 1; }
    if (1 + two * three * 4 + 5 == 50 - 4 * 5) ngood = ngood + 1; else { console.log("Test ff101_27iii failed"); nerrs = nerrs + 1; }
    var ii = -two - three * 4 ** (-5 + 2 * 4) + 200;
    if (ii == 6) ngood = ngood + 1; else { console.log("Test ff101_27iv failed"); nerrs = nerrs + 1; }
    if (++ii * 5 == 35) ngood = ngood + 1; else { console.log("Test ff101_27v failed"); nerrs = nerrs + 1; }
    if (++ii * 4 == 32) ngood = ngood + 1; else { console.log("Test ff101_27vi failed"); nerrs = nerrs + 1; }
    if (ii++ * 3 == 24) ngood = ngood + 1; else { console.log("Test ff101_27vii failed"); nerrs = nerrs + 1; }
    if (ii++ * 2 == 18) ngood = ngood + 1; else { console.log("Test ff101_27viii failed"); nerrs = nerrs + 1; }
    ii = two;
    ii = 2 + ++ii ** 3 * 4;
    if (++ii * two == two * 100 + two * 10 + two) ngood = ngood + 1; else { console.log("Test ff101_27ix failed"); nerrs = nerrs + 1; }
    let aa = [0, function (nn) { return 1 + nn * (nn - 1); }, 2];
    if (aa[1](three) == 7) ngood = ngood + 1; else { console.log("Test ff101_27x failed"); nerrs = nerrs + 1; }
    if (aa[aa[1](0)](two + three) == 21) ngood = ngood + 1; else { console.log("Test ff101_27xi failed"); nerrs = nerrs + 1; }
}
ff101_27(2, 3);
////////////////////////////////////////////////////////////////////////
// Test 101_28 - 03/10/2022
////////////////////////////////////////////////////////////////////////
if (printf) console.log("Starting test 101-28...");
function fxparent_101_28(vv) {
    return (vv);
}
function flev2_101_28(tvers, vint, vstr, vfunc, varr, vel) {
    if (vint == 10128) ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "1 failed"); nerrs = nerrs + 1; }
    if (vstr == 'Alpha') ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "2 failed"); nerrs = nerrs + 1; }
    if (vfunc(7) == 15) ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "3 failed"); nerrs = nerrs + 1; }
    if (varr[0] == 1) ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "4 failed"); nerrs = nerrs + 1; }
    if (varr[2] == 'three') ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "5 failed"); nerrs = nerrs + 1; }
    if (vel == 'five') ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "6 failed"); nerrs = nerrs + 1; }
    if (varr[1].toString() == '77,88') ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "7 failed"); nerrs = nerrs + 1; }

    if (fxparent_101_28(vint) == 10128) ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "11 failed"); nerrs = nerrs + 1; }
    if (fxparent_101_28(vstr) == 'Alpha') ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "12 failed"); nerrs = nerrs + 1; }
    if (fxparent_101_28(vfunc(7)) == 15) ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "13 failed"); nerrs = nerrs + 1; }

    if (fxparent_101_28(varr[0]) == 1) ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "14 failed"); nerrs = nerrs + 1; }
    if (fxparent_101_28(varr[2]) == 'three') ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "15 failed"); nerrs = nerrs + 1; }
    if (fxparent_101_28(vel) == 'five') ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "16 failed"); nerrs = nerrs + 1; }
    if (fxparent_101_28(varr)[1].toString() == '77,88') ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "17 failed"); nerrs = nerrs + 1; }
    if (fxparent_101_28(varr[1]).toString() == '77,88') ngood = ngood + 1; else { console.log("Test 101_28" + tvers + "18 failed"); nerrs = nerrs + 1; }

    if (tvers == 'a') {
        flev2_101_28('b', vint, vstr, vfunc, varr, vel);
    }
    if (tvers == 'c') {
        flev2_101_28('d', vint, vstr, vfunc, varr, vel);
    }
}
function func_101_28(xx) { return 1 + 2 * xx; }
flev2_101_28('a', 10128, "Alpha", function (xx) { return 1 + 2 * xx; }, [1, [77, 88], 'three'], [44, 'five'][1]);
flev2_101_28('c', 6 * (88 + 40 ** 2), "01Alpha23".substring(2, 7),
    func_101_28, ['beta', ["X".length, [77, 88], 'three']]["X".length], ['one', [44, "fi" + "ve"]][1][1]);
////////////////////////////////////////////////////////////////////////
// Test 101_29 - 03/22/2022
////////////////////////////////////////////////////////////////////////
if (printf) console.log("Starting test 101-29...");
function func_101_29(arr1, vv) {
    arr1.push(vv);
}
let arr101_29 = ['pigs', 'goats'];

func_101_29(arr101_29, 'cows');
func_101_29(arr101_29, ['cats', 'dogs']);
if (arr101_29[0] == 'pigs') ngood = ngood + 1; else { console.log("Test 101_29a failed"); nerrs = nerrs + 1; }
if (arr101_29[2] == 'cows') ngood = ngood + 1; else { console.log("Test 101_29b failed"); nerrs = nerrs + 1; }
if (arr101_29[3][0] == 'cats') ngood = ngood + 1; else { console.log("Test 101_29c failed"); nerrs = nerrs + 1; }
if (arr101_29[3][1] == 'dogs') ngood = ngood + 1; else { console.log("Test 101_29d failed"); nerrs = nerrs + 1; }
if (arr101_29.length == 4) ngood = ngood + 1; else { console.log("Test 101_29e failed"); nerrs = nerrs + 1; }
if (arr101_29[3].length == 2) ngood = ngood + 1; else { console.log("Test 101_29f failed"); nerrs = nerrs + 1; }
const v1101_29a = 'alpha';
const v1101_29b = 'beta';
const v1101_29c = 'gamma';
arr101_29 = [v1101_29a, v1101_29b];
func_101_29(arr101_29, v1101_29c);
if (arr101_29[0] == 'alpha') ngood = ngood + 1; else { console.log("Test 101_29g failed"); nerrs = nerrs + 1; }
if (arr101_29[2] == 'gamma') ngood = ngood + 1; else { console.log("Test 101_29h failed"); nerrs = nerrs + 1; }
if (arr101_29.length == 3) ngood = ngood + 1; else { console.log("Test 101_29i failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_30 - 07/19/2022 - Simple object tests
////////////////////////////////////////////////////////////////////////
if (printf) console.log("Starting test 101-30...");
function func_101_30(tvers, vob1) {
    return vob1.lastName;
}
function func_101_30b(tvers, vob1) {
    return vob1.co;
}
let obj101_30 = { firstName: "John", lastName: "Jones" };
const obj101b_30 = { firstName: "John", lastName: "Doe", age: 50, eyeColor: "blue", company: { coname: "Taurus Software", cocity: "San Carlos" } };

let ss101_30 = func_101_30('a', { firstName: "John", lastName: "Doe" });
if (ss101_30 == 'Doe') ngood = ngood + 1; else { console.log("Test 101_30a failed"); nerrs = nerrs + 1; }
ss101_30 = func_101_30('b', obj101_30);
if (ss101_30 == 'Jones') ngood = ngood + 1; else { console.log("Test 101_30b failed"); nerrs = nerrs + 1; }
ss101_30 = obj101b_30.company.cocity;
if (ss101_30 == 'San Carlos') ngood = ngood + 1; else { console.log("Test 101_30c failed"); nerrs = nerrs + 1; }
if (obj101b_30.company.coname == "Taurus Software") ngood = ngood + 1; else { console.log("Test 101_30d failed"); nerrs = nerrs + 1; }
if (obj101b_30.eyeColor == "blue") ngood = ngood + 1; else { console.log("Test 101_30e failed"); nerrs = nerrs + 1; }
if ({ firstName: "John", lastName: "Jones" }.firstName == "John") ngood = ngood + 1; else { console.log("Test 101_30f failed"); nerrs = nerrs + 1; }
const obj101b_30b = { fn: "Jack", ln: "Long", co: { na: "Taurus", ci: "Belmont", ea: ["Dave", "Andy"] } };
if (obj101b_30b.co.ea[0] == "Dave") ngood = ngood + 1; else { console.log("Test 101_30e failed"); nerrs = nerrs + 1; }
if (func_101_30b('f', obj101b_30b).ci == "Belmont") ngood = ngood + 1; else { console.log("Test 101_30f failed"); nerrs = nerrs + 1; }
ss101_30 = func_101_30b('g', obj101b_30b);
if (ss101_30.na == "Taurus") ngood = ngood + 1; else { console.log("Test 101_30g failed"); nerrs = nerrs + 1; }
obj101_30.model = 'Eagle';
if (obj101_30.model == "Eagle") ngood = ngood + 1; else { console.log("Test 101_30h failed"); nerrs = nerrs + 1; }
obj101_30.make = { coname: "Taurus Software", cocity: "San Carlos" };
if (obj101_30.make.coname == "Taurus Software") ngood = ngood + 1; else { console.log("Test 101_30i failed"); nerrs = nerrs + 1; }
obj101_30.make = { street: "Quarry Road", county: "San Mateo" };
if (obj101_30.make.street == "Quarry Road") ngood = ngood + 1; else { console.log("Test 101_30j failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_31 - 08/08/2022 - String interpolation tests
////////////////////////////////////////////////////////////////////////
if (printf) console.log("Starting test 101-31...");
let v101_31a = "abcdefghiKlmnopqrsT";
let v101_31b = 29;
const v101_31c = 31;
let v101_31n = "";
var v101_31d = "xy${v101_31a}z";
var v101_31e = `xy${v101_31a}z`;
let s101_31a = `xy${v101_31a}z`;
if (s101_31a == "xyabcdefghiKlmnopqrsTz") ngood = ngood + 1; else { console.log("Test 101_31a failed"); nerrs = nerrs + 1; }
let s101_31b = `x${v101_31b}y${v101_31a}z`;
if (s101_31b == "x29yabcdefghiKlmnopqrsTz") ngood = ngood + 1; else { console.log("Test 101_31b failed"); nerrs = nerrs + 1; }
let s101_31c = `x${v101_31b + v101_31c}z`;
if (s101_31c == "x60z") ngood = ngood + 1; else { console.log("Test 101_31c failed"); nerrs = nerrs + 1; }
let s101_31d = `x${v101_31d}z`;
if (s101_31d == "xxy${v101_31a}zz") ngood = ngood + 1; else { console.log("Test 101_31d failed"); nerrs = nerrs + 1; }
let s101_31e = `x${v101_31e}z`;
if (s101_31e == "xxyabcdefghiKlmnopqrsTzz") ngood = ngood + 1; else { console.log("Test 101_31e failed"); nerrs = nerrs + 1; }
let s101_31f = `x${v101_31n}y${v101_31n}z`;
if (s101_31f == "xyz") ngood = ngood + 1; else { console.log("Test 101_31f failed"); nerrs = nerrs + 1; }
let s101_31g = `xy\${v101_31a}z`;
if (s101_31g == "xy${v101_31a}z") ngood = ngood + 1; else { console.log("Test 101_31g failed"); nerrs = nerrs + 1; }
let s101_31h = `xy\\${v101_31a}z`;
if (s101_31h == "xy\\abcdefghiKlmnopqrsTz") ngood = ngood + 1; else { console.log("Test 101_31h failed"); nerrs = nerrs + 1; }
let s101_31i = `xy\u005c${v101_31a}z`;
if (s101_31i == "xy\\abcdefghiKlmnopqrsTz") ngood = ngood + 1; else { console.log("Test 101_31i failed"); nerrs = nerrs + 1; }
var s101_31j = `xy${`12${v101_31a}34`}z`;
if (s101_31j == "xy12abcdefghiKlmnopqrsT34z") ngood = ngood + 1; else { console.log("Test 101_31j failed"); nerrs = nerrs + 1; }
var s101_31k = `xy${"12${v101_31a}34"}z`;
if (s101_31k == "xy12${v101_31a}34z") ngood = ngood + 1; else { console.log("Test 101_31k failed"); nerrs = nerrs + 1; }
var v101_31e = `xy${v101_31a}z`;
function f101_31a(arg1) { return arg1 + "DEF"; }
let s101_31l = `xy${f101_31a("ABC")}z`;
if (s101_31l == "xyABCDEFz") ngood = ngood + 1; else { console.log("Test 101_31l failed"); nerrs = nerrs + 1; }
let s101_31m = `xy${f101_31a(v101_31e)}z`;
if (s101_31m == "xyxyabcdefghiKlmnopqrsTzDEFz") ngood = ngood + 1; else { console.log("Test 101_31m failed"); nerrs = nerrs + 1; }
let s101_31n = `xy${f101_31a(`xy${v101_31a}z`)}z`;
if (s101_31n == "xyxyabcdefghiKlmnopqrsTzDEFz") ngood = ngood + 1; else { console.log("Test 101_31n failed"); nerrs = nerrs + 1; }
let v101_31o1 = 11;
let v101_31o2 = 5;
let s101_31o = `x${v101_31o1 / v101_31o2}y`;
if (s101_31o == "x2.2y") ngood = ngood + 1; else { console.log("Test 101_31o failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_32 - 08/29/2022 - Expression tests
////////////////////////////////////////////////////////////////////////
var v101_32;
v101_32 = "abc", 3, 4;
if (v101_32 == "abc") ngood = ngood + 1; else { console.log("Test 101_32a failed"); nerrs = nerrs + 1; }
v101_32 = ("abc", 3, 4);
if (v101_32 == 4) ngood = ngood + 1; else { console.log("Test 101_32b failed"); nerrs = nerrs + 1; }
v101_32 = 11 / 5;
if (v101_32 == 2.2) ngood = ngood + 1; else { console.log("Test 101_32c failed"); nerrs = nerrs + 1; }
v101_32 = new Number(7);
if (v101_32 == 7) ngood = ngood + 1; else { console.log("Test 101_32d failed"); nerrs = nerrs + 1; }
v101_32 = new Number("3.14");
if (v101_32 == 3.14) ngood = ngood + 1; else { console.log("Test 101_32e failed"); nerrs = nerrs + 1; }
v101_32 = 8 + Number("70") + new Number(600) + 5000 + Number("40000") + new Number("300000");
if (v101_32 == 345678) ngood = ngood + 1; else { console.log("Test 101_32f failed"); nerrs = nerrs + 1; }
v101_32 = new String("abc") + new Number("70");
if (v101_32 == "abc70") ngood = ngood + 1; else { console.log("Test 101_32g failed"); nerrs = nerrs + 1; }
if (Boolean(0) == false) ngood = ngood + 1; else { console.log("Test 101_32h failed"); nerrs = nerrs + 1; }
if (Boolean(2) == true) ngood = ngood + 1; else { console.log("Test 101_32i failed"); nerrs = nerrs + 1; }
if (Boolean('false') == true) ngood = ngood + 1; else { console.log("Test 101_32j failed"); nerrs = nerrs + 1; }
if (Boolean('') == false) ngood = ngood + 1; else { console.log("Test 101_32k failed"); nerrs = nerrs + 1; }
if (Boolean(' ') == true) ngood = ngood + 1; else { console.log("Test 101_32l failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_33 - 08/29/2022 - typeof tests
////////////////////////////////////////////////////////////////////////
var v101_33, s101_33;
v101_33 = 4;
if (typeof v101_33 == "number") ngood = ngood + 1; else { console.log("Test 101_32a failed"); nerrs = nerrs + 1; }
v101_33 = "56";
if (typeof v101_33 == "string") ngood = ngood + 1; else { console.log("Test 101_32b failed"); nerrs = nerrs + 1; }
v101_33 = false;
if (typeof v101_33 == "boolean") ngood = ngood + 1; else { console.log("Test 101_32c failed"); nerrs = nerrs + 1; }
function f101_33(aa) { return typeof aa; }
v101_33 = f101_33;
if (typeof v101_33 == "function") ngood = ngood + 1; else { console.log("Test 101_32d failed"); nerrs = nerrs + 1; }
v101_33 = f101_33();
if (v101_33 == "undefined") ngood = ngood + 1; else { console.log("Test 101_32e failed"); nerrs = nerrs + 1; }
v101_33 = f101_33(function ff(aa) { return typeof aa; });
if (v101_33 == "function") ngood = ngood + 1; else { console.log("Test 101_32f failed"); nerrs = nerrs + 1; }
v101_33 = f101_33(new Number(8));
//console.log("v101_33=", v101_33);
if (v101_33 == "object") ngood = ngood + 1; else { console.log("Test 101_32g failed"); nerrs = nerrs + 1; }
v101_33 = function ff(aa) { return typeof aa; };
if (typeof v101_33 == "function") ngood = ngood + 1; else { console.log("Test 101_32h failed"); nerrs = nerrs + 1; }
v101_33 = { firstName: "John", lastName: "Jones", age: 29 };
if (typeof v101_33 == "object") ngood = ngood + 1; else { console.log("Test 101_32i failed"); nerrs = nerrs + 1; }
s101_33 = typeof v101_33.firstName;
if (s101_33 == "string") ngood = ngood + 1; else { console.log("Test 101_32j failed"); nerrs = nerrs + 1; }
v101_33 = [12, 'three', true];
if (typeof v101_33 == "object") ngood = ngood + 1; else { console.log("Test 101_32k failed"); nerrs = nerrs + 1; }
s101_33 = typeof v101_33[1];
if (s101_33 == "string") ngood = ngood + 1; else { console.log("Test 101_32l failed"); nerrs = nerrs + 1; }
s101_33 = typeof v101_33[2];
if (s101_33 == "boolean") ngood = ngood + 1; else { console.log("Test 101_32m failed"); nerrs = nerrs + 1; }
v101_33 = 12, 'three', true;
if (typeof v101_33 == "number") ngood = ngood + 1; else { console.log("Test 101_32n failed"); nerrs = nerrs + 1; }
v101_33 = (12, 'three', true);
if (typeof v101_33 == "boolean") ngood = ngood + 1; else { console.log("Test 101_32o failed"); nerrs = nerrs + 1; }
v101_33 = Number;
if (typeof v101_33 == "function") ngood = ngood + 1; else { console.log("Test 101_32p failed"); nerrs = nerrs + 1; }
v101_33 = Number.toString;
if (typeof v101_33 == "function") ngood = ngood + 1; else { console.log("Test 101_32q failed"); nerrs = nerrs + 1; }
v101_33 = Number("123");
if (typeof v101_33 == "number") ngood = ngood + 1; else { console.log("Test 101_32r failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_34 - 09/21/2022 - new function() tests
////////////////////////////////////////////////////////////////////////
function f101_34(make, model, year) {
    this.make = make;
    this.model = model;
    this.year = year;
    var make = "BMW";
    
    make = make + "Ford";
    this.make = this.make + "Honda";
    this.make2 = make;
}

const v101_34 = new f101_34('Eagle', 'Talon TSi', 1993);
if (v101_34.make == "EagleHonda") ngood = ngood + 1; else { console.log("Test 101_34a failed"); nerrs = nerrs + 1; }
if (v101_34.model == "Talon TSi") ngood = ngood + 1; else { console.log("Test 101_34b failed"); nerrs = nerrs + 1; }
if (v101_34.year == 1993) ngood = ngood + 1; else { console.log("Test 101_34c failed"); nerrs = nerrs + 1; }
if (v101_34.make2 == "BMWFord") ngood = ngood + 1; else { console.log("Test 101_34d failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_35 - 09/22/2022 - new Object() tests
////////////////////////////////////////////////////////////////////////
var v101_35 = new Object();
v101_35.chapter1 = new Object();
v101_35.chapter1.title = "Introduction to JavaScript";
v101_35.chapter1.pages = 19;
v101_35.chapter2 = { title: "Lexical Structure", pages: 6 };
if (v101_35.chapter1.title == "Introduction to JavaScript") ngood = ngood + 1; else { console.log("Test 101_35a failed"); nerrs = nerrs + 1; }
if (v101_35.chapter2.title == "Lexical Structure") ngood = ngood + 1; else { console.log("Test 101_35b failed"); nerrs = nerrs + 1; }
function f101_35_Rectangle(w, h) {
    function f101_35_compute_area3() { return this.width * this.height + 3; };
    this.width = w;
    this.height = h;
    this.area3 = f101_35_compute_area3;
}
function f101_35_compute_area() { return this.width * this.height; }
var wf101_35 = new f101_35_Rectangle(3, 4);
wf101_35.area = f101_35_compute_area;
wf101_35.area2 = function () { return this.width * this.height + 2; };
if (wf101_35.area() == 12) ngood = ngood + 1; else { console.log("Test 101_35c failed"); nerrs = nerrs + 1; }
if (wf101_35.area2() == 14) ngood = ngood + 1; else { console.log("Test 101_35d failed"); nerrs = nerrs + 1; }
if (wf101_35.area3() == 15) ngood = ngood + 1; else { console.log("Test 101_35e failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
// Test 101_36 - 01/16/2023 - prototype tests
////////////////////////////////////////////////////////////////////////
String.prototype.v101_36 = 16;
if (String.prototype.v101_36 == 16) ngood = ngood + 1; else { console.log("Test 101_36a failed"); nerrs = nerrs + 1; }
////////////////////////////////////////////////////////////////////////
if (nerrs == 0) console.log("All", ngood, "tests successful.");
else console.log("****", nerrs, "tests failed, out of", nerrs + ngood);
console.log("--end xeqfil-Tests101-Tests tests--");
//////// End tests ////////
////////////////////////////////////////////////////////////////////////
