// JavaScript source code
// jstest02.js

'use strict';

let ii = 123, ss = 'One two three';
const aa = [11, ['aa', 'bb', 'cc'], 33];
const util = require('util');

console.log("ii=%d", ii, ss);                       // Expected: ii=123 One two three
@@
////////////////////////////////

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
console.log("ff4=%s", ff3);                         // Expected: ff4=aa:[11,["aa","bb","cc"],33]
console.log("ii=%04d ii=%-4d", ii, ii);             // Expected: ii=%04d ii=%-4d 123 123
console.log("aa=%s", aa);                           // Expected: Array(3)
console.log("ii=%s", ii);                           // Expected: ii=123
console.log("ss=%d", ss);                           // Expected: ss=NaN
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
console.log("--end jstest02.js tests--");
////////////////////////////////////////////////////////////////////////

//  Arrays:
//      1. (3) [11, Array(3), 33]                                           0  , JFMT_FLAGS_LOG
//      2. 11,aa,bb,cc,33                                                   0  , JFMT_FLAG_TOSTRING
//      3. [ 11, [ 'aa', 'bb', 'cc', [length]: 3 ], 33, [length]: 3 ]       'o', JFMT_FLAG_FORMAT
//      4. [ 11, [ 'aa', 'bb', 'cc' ], 33 ]                                 'O', JFMT_FLAG_FORMAT
//      5. [11,["aa","bb","cc"],33]                                         'j', JFMT_FLAGS_LOG
//      6. Array(3)                                                         's', JFMT_FLAGS_LOG

