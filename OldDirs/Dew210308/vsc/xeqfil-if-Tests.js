// xeqfil-if-Tests.js
//  Adapted for js 08/07/2020
//
////////////////////////////////////////////////////////////////
//  04/10/2018

//////// Good tests ////////
// 163 - 11/07/2019
if (true) {
    if (false) { console.log( "Bad test 163a"); }
    if (false) { console.log( "Bad test 163b"); }
    if (false) { console.log( "Bad test 163c"); }
    if (false) { console.log( "Bad test 163d"); }
    if (true)  { console.log( "Good test 163"); }
}
console.log( "-- --");

// 162 - 11/07/2019
if (false) { console.log( "Bad test 162a"); }
if (false) { console.log( "Bad test 162b"); }
if (false) { console.log( "Bad test 162c"); }
if (false) { console.log( "Bad test 162d"); }
if (true)  { console.log( "Good test 162"); }
console.log( "-- --");

//  // 161 - 11/06/2019
//  function func161() {
//      if (true) {
//          if (false) console.log( "Bad test 161a");
//          if (false) console.log( "Bad test 161b");
//          if (false) console.log( "Bad test 161c");
//          if (false) console.log( "Bad test 161d");
//          if (true)  console.log( "Good test 161");
//      }
//  }
//  func161();
//  console.log( "-- --");

// 160 - 11/06/2019
if (true) {
    if (false) console.log( "Bad test 160a");
    if (false) console.log( "Bad test 160b");
    if (false) console.log( "Bad test 160c");
    if (false) console.log( "Bad test 160d");
    if (true)  console.log( "Good test 160");
}
console.log( "-- --");

// 159 - 11/06/2019
if (true) {
    if (false) console.log( "Bad test 160a");
    if (false) console.log( "Bad test 160b");
    if (false) console.log( "Bad test 160c");
    if (false) console.log( "Bad test 160d");
    if (true)  console.log( "Good test 160");
}
console.log( "-- --");

//  // 158 - 11/04/2019
//  function func158() {
//      if (false) { console.log( "Bad test 158a"); }
//      if (false) { console.log( "Bad test 158b"); }
//      if (false) { console.log( "Bad test 158c"); }
//      if (false) { console.log( "Bad test 158d"); }
//      if (true)  { console.log( "Good test 158"); }
//  }
//  func158();
//  console.log( "-- --");

// 157 - 08/20/2019
if (3 + 2 == 5) if (1 + 4 == 6) if (1 + 7 == 9) { console.log( "Bad test 157a"); }
else { console.log( "Bad test 157b"); }
else { console.log( "Good test 157"); }
else { console.log( "Bad test 157c"); }
console.log( "-- --");

// 156 - 08/20/2019
if (3 + 2 == 5) console.log( "Good test 156");
else ;
console.log( "-- --");

// 155 - 08/20/2019
if (3 + 4 == 5 );
else console.log( "Good test 155");
console.log( "-- --");

// 154 - 08/13/2019
if (1 == 2)
{
    if (4 == 2) console.log( "Bad test 154a");
    console.log( "Bad test 154b");
}
console.log( "Good test 154");
console.log( "-- --");

//////// Good tests ////////
// 153 - 07/31/2019
if (3 + 2 == 5) if (1 + 4 == 6) if (1 + 7 == 9) console.log( "Bad test 153a");
else console.log( "Bad test 153b");
else console.log( "Good test 153");
else console.log( "Bad test 153c");
console.log( "-- --");

if (1 + 3 == 4) {
    if (1 + 4 == 5) {
        if (1 + 7 == 9) {
            console.log( "Bad",); console.log( "test",); console.log( "152a");
        }
        console.log( "Good",); console.log( "test",); console.log( "152");
    }
}
else { console.log( "Bad",); console.log( "test",); console.log( "152b"); }
console.log( "-- --");

if (1 + 3 == 4) {
    if (1 + 4 == 9) {
        if (1 + 7 == 8) {
            console.log( "Bad",); console.log( "test",); console.log( "151a");
        }
    }
    console.log( "Good",); console.log( "test",); console.log( "151");
}
else { console.log( "Bad",); console.log( "test",); console.log( "151b"); }
console.log( "-- --");

if (1 + 2 == 9) {
    if (1 + 4 == 5) {
        if (1 + 7 == 8) {
            console.log( "Bad",); console.log( "test",); console.log( "150a");
        }
        else { console.log( "Bad",); console.log( "test",); console.log( "150b"); }
    }
}
console.log( "Good",); console.log( "test",); console.log( "150");
console.log( "-- --");

if (1 + 2 == 9) {
    if (1 + 4 == 5) {
        if (1 + 7 == 8) {
            console.log( "Bad",); console.log( "test",); console.log( "149a");
        }
    }
    else { console.log( "Good",); console.log( "test",); console.log( "149b"); }
}
console.log( "Good",); console.log( "test",); console.log( "149");
console.log( "-- --");

if (1 + 2 == 9) {
    if (1 + 4 == 5) {
        if (1 + 7 == 8) {
            console.log( "Bad",); console.log( "test",); console.log( "148");
        }
    }
}
else { console.log( "Good",); console.log( "test",); console.log( "148"); }
console.log( "-- --");

if (1 + 2 == 3) {
    if (1 + 4 == 5) {
        if (1 + 7 == 9) {
            console.log( "Bad",); console.log( "test",); console.log( "147");
        }
    }
}
console.log( "Good",); console.log( "test",); console.log( "147");
console.log( "-- --");

if (1 + 2 == 3) {
    if (1 + 4 == 9) {
        if (1 + 7 == 8) {
            console.log( "Bad",); console.log( "test",); console.log( "146");
        }
    }
}
console.log( "Good",); console.log( "test",); console.log( "146");
console.log( "-- --");

if (1 + 2 == 9) {
    if (1 + 4 == 5) {
        if (1 + 7 == 8) {
            console.log( "Bad",); console.log( "test",); console.log( "145");
        }
    }
}
console.log( "Good",); console.log( "test",); console.log( "145");
console.log( "-- --");

if (1 + 2 == 3) {
    if (1 + 4 == 6) {
        if (1 + 7 == 8) {
            console.log( "Bad",); console.log( "test",); console.log( "144a");
        } else {
            console.log( "Bad",); console.log( "test",); console.log( "144b");
        }
    } else {
        console.log( "Good",); console.log( "test",); console.log( "144");
    }
} else {
    console.log( "Bad",); console.log( "test",); console.log( "144c");
}
console.log( "-- --");

if (1 + 2 == 9) {
    if (1 + 5 == 6)
        if (1 + 7 == 8) {
            console.log( "Bad test 143a");
        } else
            console.log( "Bad test 143b");
    else
        console.log( "Bad test 143c");
} else
    console.log( "Good test 143");
console.log( "-- --");

if (1 + 2 == 3) {
    if (1 + 5 == 9) {
        if (1 + 7 == 8)
            console.log( "Bad test 142a");
        else
            console.log( "Bad test 142b");
    } else
        console.log( "Good test 142");
} else
    console.log( "Bad test 142c");
console.log( "-- --");

if (1 + 2 == 3) {
    if (1 + 5 == 6) {
        if (1 + 7 == 9)
            console.log( "Bad test 141a");
        else
            console.log( "Good test 141");
    } else
        console.log( "Bad test 141b");
} else
    console.log( "Bad test 141c");
console.log( "-- --");

{ if (1 + 2 == 3) {
    if (1 + 5 == 6)
        if (1 + 7 == 8)
            console.log( "Good test 140");
        else
            console.log( "Bad test 140a");
    else
        console.log( "Bad test 140b");
} else
    console.log( "Bad test 140c"); }
console.log( "-- --");

if (1 + 2 == 9)
    if (1 + 5 == 6)
        if (1 + 7 == 8) {
            console.log( "Bad test 133a");
        } else
            console.log( "Bad test 133b");
    else
        console.log( "Bad test 133c");
else
    console.log( "Good test 133");
console.log( "-- --");

if (1 + 2 == 3)
    if (1 + 5 == 9) {
        if (1 + 7 == 8)
            console.log( "Bad test 132a");
        else
            console.log( "Bad test 132b");
    } else
        console.log( "Good test 132");
else
    console.log( "Bad test 132c");
console.log( "-- --");

if (1 + 2 == 3) {
    if (1 + 5 == 6)
        if (1 + 7 == 9)
            console.log( "Bad test 131a");
        else
            console.log( "Good test 131");
    else
        console.log( "Bad test 131b");
} else
    console.log( "Bad test 131c");
console.log( "-- --");

{ if (1 + 2 == 3)
    if (1 + 5 == 6)
        if (1 + 7 == 8)
            console.log( "Good test 130");
        else
            console.log( "Bad test 130a");
    else
        console.log( "Bad test 130b");
else
    console.log( "Bad test 130c"); }
console.log( "-- --");

if (1 + 2 == 9)
    if (1 + 5 == 6)
        if (1 + 7 == 8)
            console.log( "Bad test 123a");
        else
            console.log( "Bad test 123b");
    else
        console.log( "Bad test 123c");
else
    console.log( "Good test 123");
console.log( "-- --");

if (1 + 2 == 3)
    if (1 + 5 == 9)
        if (1 + 7 == 8)
            console.log( "Bad test 122a");
        else
            console.log( "Bad test 122b");
    else
        console.log( "Good test 122");
else
    console.log( "Bad test 122c");
console.log( "-- --");

if (1 + 2 == 3)
    if (1 + 5 == 6)
        if (1 + 7 == 9)
            console.log( "Bad test 121a");
        else
            console.log( "Good test 121");
    else
        console.log( "Bad test 121b");
else
    console.log( "Bad test 121c");
console.log( "-- --");

if (1 + 2 == 3)
    if (1 + 5 == 6)
        if (1 + 7 == 8)
            console.log( "Good test 120");
        else
            console.log( "Bad test 120a");
    else
        console.log( "Bad test 120b");
else
    console.log( "Bad test 120c");
console.log( "-- --");

if (1 + 2 == 4) if (1 + 5 == 7) {
    if (11 + 12 == 23) if (1 + 8 == 9) { console.log( "Bad",); console.log( "test",); console.log( "119a"); }
    else { console.log( "Bad",); console.log( "test",); console.log( "119b"); } }
else { console.log( "Bad",); console.log( "test",); console.log( "119c"); }
else { console.log( "Good",); console.log( "test",); console.log( "119"); }
console.log( "-- --");

if (1 + 2 == 4) if (1 + 5 == 7) if (11 + 12 == 23) { if (1 + 8 == 9) { console.log( "Bad",); console.log( "test",); console.log( "118a"); } }
else { console.log( "Bad",); console.log( "test",); console.log( "118b"); }
else { console.log( "Bad",); console.log( "test",); console.log( "118c"); }
else { console.log( "Good",); console.log( "test",); console.log( "118"); }
console.log( "-- --");

if (1 + 2 == 4) if (1 + 5 == 7) { if (1 + 8 == 9) { console.log( "Bad",); console.log( "test",); console.log( "117a"); } }
else { console.log( "Bad",); console.log( "test",); console.log( "117b"); }
else { console.log( "Good",); console.log( "test",); console.log( "117"); }
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 7) { if (1 + 8 == 9) { console.log( "Bad",); console.log( "test",); console.log( "116a"); } }
else { console.log( "Good",); console.log( "test",); console.log( "116"); }
else { console.log( "Bad",); console.log( "test",); console.log( "116b"); }
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) { if (1 + 8 == 9) { console.log( "Good",); console.log( "test",); console.log( "115"); } }
else { console.log( "Bad",); console.log( "test",); console.log( "115a"); }
else { console.log( "Bad",); console.log( "test",); console.log( "115b"); }
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) { if (1 + 8 == 9) { console.log( "Good",); console.log( "test",); console.log( "114"); } }
else { console.log( "Bad",); console.log( "test",); console.log( "114"); }
console.log( "-- --");

if (1 + 2 == 3) { if (1 + 5 == 6) if (1 + 8 == 9) { console.log( "Good",); console.log( "test",); console.log( "113"); } }
else { console.log( "Bad",); console.log( "test",); console.log( "113"); }
console.log( "-- --");

if (1 + 2 == 3) { if (1 + 5 == 6) { console.log( "Good",); console.log( "test",); console.log( "112"); } }
else { console.log( "Bad",); console.log( "test",); console.log( "112"); }
console.log( "-- --");

if (1 + 2 == 5) if (1 + 4 == 6) if (1 + 7 == 9) { console.log( "Bad",); console.log( "test",); console.log( "111a"); }
else { console.log( "Bad",); console.log( "test",); console.log( "111b"); }
else { console.log( "Bad",); console.log( "test",); console.log( "111c"); }
else { console.log( "Good",); console.log( "test",); console.log( "111"); }
console.log( "-- --");

if (1 + 2 == 3) if (1 + 4 == 6) if (1 + 7 == 9) { console.log( "Bad",); console.log( "test",); console.log( "110a"); }
else { console.log( "Bad",); console.log( "test",); console.log( "110b"); }
else { console.log( "Good",); console.log( "test",); console.log( "110"); }
else { console.log( "Bad",); console.log( "test",); console.log( "110c"); }
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) if (1 + 7 == 9) { console.log( "Bad",); console.log( "test",); console.log( "109a"); }
else { console.log( "Good",); console.log( "test",); console.log( "109"); }
else { console.log( "Bad",); console.log( "test",); console.log( "109b"); }
else { console.log( "Bad",); console.log( "test",); console.log( "109c"); }
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) if (1 + 8 == 9) { console.log( "Good",); console.log( "test",); console.log( "108"); }
else { console.log( "Bad",); console.log( "test",); console.log( "108a"); }
else { console.log( "Bad",); console.log( "test",); console.log( "108b"); }
else { console.log( "Bad",); console.log( "test",); console.log( "108c"); }
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) if (1 + 8 == 9) { console.log( "Good",); console.log( "test",); console.log( "107"); }
else { console.log( "Bad",); console.log( "test",); console.log( "107a"); }
else { console.log( "Bad",); console.log( "test",); console.log( "107b"); }
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) if (1 + 8 == 9) { console.log( "Good",); console.log( "test",); console.log( "106"); }
else { console.log( "Bad",); console.log( "test",); console.log( "106"); }
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) if (1 + 8 == 9) { console.log( "Good",); console.log( "test",); console.log( "105"); }
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) { console.log( "Good",); console.log( "test",); console.log( "104"); }
console.log( "-- --");

if (1 + 2 == 4) { console.log( "Bad",); console.log( "test",); console.log( "103"); }
else { console.log( "Good",); console.log( "test",); console.log( "103"); }
console.log( "-- --");

if (1 + 2 == 3) { console.log( "Good",); console.log( "test",); console.log( "102"); }
else { console.log( "Bad",); console.log( "test",); console.log( "102"); }
console.log( "-- --");

if (1 + 2 == 3) { console.log( "Good",); console.log( "test",); console.log( "101"); }
console.log( "-- --");

if (1 + 2 == 5) if (1 + 4 == 6) if (1 + 7 == 9) console.log( "Bad test 11a");
else console.log( "Bad test 11b");
else console.log( "Bad test 11c");
else console.log( "Good test 11");
console.log( "-- --");

if (1 + 2 == 3) if (1 + 4 == 6) if (1 + 7 == 9) console.log( "Bad test 10a");
else console.log( "Bad test 10b");
else console.log( "Good test 10");
else console.log( "Bad test 10c");
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) if (1 + 7 == 9) console.log( "Bad test 9a");
else console.log( "Good test 9");
else console.log( "Bad test 9b");
else console.log( "Bad test 9c");
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) if (1 + 8 == 9) console.log( "Good test 8");
else console.log( "Bad test 8a");
else console.log( "Bad test 8b");
else console.log( "Bad test 8c");
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) if (1 + 8 == 9) console.log( "Good test 7");
else console.log( "Bad test 7a");
else console.log( "Bad test 7b");
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) if (1 + 8 == 9) console.log( "Good test 6");
else console.log( "Bad test 6");
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) if (1 + 8 == 9) console.log( "Good test 5");
console.log( "-- --");

if (1 + 2 == 3) if (1 + 5 == 6) console.log( "Good test 4");
console.log( "-- --");

if (1 + 2 == 4) console.log( "Bad test 3");
else console.log( "Good test 3");
console.log( "-- --");

if (1 + 2 == 3) console.log( "Good test 2");
else console.log( "Bad test 2");
console.log( "-- --");

if (1 + 2 == 3) console.log( "Good test 1");

console.log( "--end if tests--");

//////// End tests ////////

setTimeout(function () { process.exit();}, 1000);