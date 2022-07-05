// xeqfil-while-Tests.txt
//
////////////////////////////////////////////////////////////////
//  08/21/2019 
//  01/29/2021 Converted from Omar to javascript

//////// Good tests ////////

// JavaScript source code
// jstest02.js

'use strict';
var ii, jj, kk, tt;

// xeqfil-while-Tests.txt
//
////////////////////////////////////////////////////////////////
//  08/21/2019

//////// Good tests ////////

// 208 - 08/22/2019
ii = 0;
tt = 0;
while (ii < 8) {
    jj = 0;
    if (ii < 4) {
        while (jj < 6) {
            if (1 + jj < 777) {
                kk = 0;
                while (kk < 4) {
                    kk = kk + 1;
                    if (kk < 2) tt = tt + 1;
                }
            } else
                tt = tt + 1000;
            jj = jj + 1;
        }
    } else
        tt = tt + 100;
    ii = ii + 1;
}
if (tt == 424) console.log("Good test 208");
else console.log("Bad test 208, tt=", tt);
console.log("-- --");

// 207 - 08/22/2019
ii = 0;
tt = 0;
while (ii < 8) {
    jj = 0;
    if (ii < 4) {
        while (jj < 6) {
            if (1 + jj < 777)
                tt = tt + 1;
            else
                tt = tt + 1000;
            jj = jj + 1;
        }
    } else
        tt = tt + 100;
    ii = ii + 1;
}
if (tt == 424) console.log("Good test 207");
else console.log("Bad test 207, tt=", tt);
console.log("-- --");

// 206 - 08/22/2019
ii = 0;
tt = 0;
while (ii < 8) {
    jj = 0;
    if (ii < 4) {
        while (jj < 6) {
            if (1 + jj < 777) {
                tt = tt + 1;
            } else {
                tt = tt + 1000;
            }
            jj = jj + 1;
        }
    } else {
        tt = tt + 100;
    }
    ii = ii + 1;
}
if (tt == 424) console.log("Good test 206");
else console.log("Bad test 206, tt=", tt);
console.log("-- --");

// 205 - 08/22/2019
ii = 0;
tt = 0;
while (ii < 8) {
    jj = 0;
    while (jj < 6) {
        if (1 + jj == 777) {
            tt = tt + 1000;
        } else {
            tt = tt + 1;
        }
        jj = jj + 1;
    }
    ii = ii + 1;
}
if (tt == 48) console.log("Good test 205");
else console.log("Bad test 205, tt=", tt);
console.log("-- --");

// 204 - 08/21/2019
ii = 0;
tt = 0;
while (ii < 8) {
    jj = 0;
    while (jj < 6) {
        kk = 0;
        while (kk < 4) {
            kk = kk + 1;
            tt = tt + 1;
        }
        jj = jj + 1;
    }
    ii = ii + 1;
}
if (tt == 192) console.log("Good test 204");
else console.log("Bad test 204");
console.log("-- --");

// 203 - 08/21/2019
ii = 0;
tt = 0;
while (ii < 8) {
    jj = 0;
    while (jj < 6) { jj = jj + 1; tt = tt + 1; }
    ii = ii + 1;
}
if (tt == 48) console.log("Good test 203");
else console.log("Bad test 203");
console.log("-- --");

// 202 - 08/21/2019
ii = 0;
while (ii < 8) { ii = ii + 1; }
if (ii == 8) console.log("Good test 202");
else console.log("Bad test 202");
console.log("-- --");

// 201 - 08/21/2019
ii = 0;
while (ii < 8) ii = ii + 1;
if (ii == 8) console.log("Good test 201");
else console.log("Bad test 201");
console.log("-- --");

console.log("--end while tests--");

//////// End tests ////////
