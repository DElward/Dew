// JavaScript source code
// jstest02.js

'use strict';

function glob0() {
    console.log("-- glob0()");
}

function foo() {
    function bar() {
        console.log("-- bar()");
        glob0();
    }
    console.log("-- foo()");
    bar();
}

foo();
return;     ////////////////////////////////////////////////////////////////

'use strict';

var vv = "glob";

function foo() {
  console.log("-- begin foo()=", vv);
  var xx = 'one';
  var vv = "zero";
  function bar() {
    console.log("-- begin bar()=", vv);
    var yy = 'two';
    var vv = "three";
    console.log("-- end bar()=", vv);
    console.log(xx); // 1 (function `bar` closes over `x`)
    console.log(yy); // 2 (`y` is in scope)
  }
  bar();
  console.log(xx); // 1 (`x` is in scope)
  //console.log(y); // ReferenceError in strict mode, `y` is scoped to `bar`
}

console.log("-- begin main()=", vv);
foo();
// Expected output:
//      one
//      two
//      one
return;     ////////////////////////////////////////////////////////////////

'use strict';

console.log("-- begin vv=", vv);

var vv = "zero";

function f1() {
    console.log("-- begin f1() vv=", vv);
    var vv = 'one';
    function f2() {
        console.log("-- begin f2()=", vv);
        var vv = 'two';
        console.log("-- f2() called vv=", vv);
    }
    f2();
    console.log("-- f1() called vv=", vv);
}
f1();
console.log("-- end vv=", vv);
// Expected output:
//      -- begin vv= undefined
//      -- begin f1() vv= undefined
//      -- begin f2()= undefined
//      -- f2() called vv= two
//      -- f1() called vv= one
//      -- end vv= zero
return;     ////////////////////////////////////////////////////////////////


'use strict';

function show2() {
    var vv = "two";
    'debug show allvars';
    console.log("-- show2() called vv=", vv);
    return "show2";
}

function show1() {
    var vv = "one";
    console.log("-- 1 show1() called vv=", vv);
    console.log("-- 2 show1() called vv=", vv);
    return show2();
}

console.log(show1(), show1(), "--end function tests-- vv=", vv);
return;     ////////////////////////////////////////////////////////////////

var vv = 0;

function inc1(nn) {
    return nn + 1;
}

vv = inc1(4);

console.log("inc1(4)=", vv);
console.log("--end function tests--");
return;

function inc1(nn) {
    return nn + 1;
}

var inc2 = function (nn) {
    return nn + 1;
}
var nn, vv1, vv2;
nn = 4;
vv1 = inc1(nn);
console.log("vv1=", vv1);

nn = 44;
vv2 = inc2(nn);
console.log("vv2=", vv2);
console.log("--end function tests--");

//////// End tests ////////

return; ////////

'use strict';

var vvar1 = "zero";
var glob0 = 'gl0b0';

function f1() {
    //console.log("-- begin f1() vvar1=", vvar1);
    //var vvar1 = 'one';
    function f2() {
        console.log("-- f2() vvar1=", vvar1);
        console.log("-- f2() glob0=", glob0);
        if (false) {
            var vvar1 = 'two';
            console.log("-- f2() called vvar1=", vvar1);
        }
    }
    f2();
    console.log("-- f1() called vvar1=", vvar1);
}
f1();
//console.log("-- end vvar1=", vvar1);
return; ////////
