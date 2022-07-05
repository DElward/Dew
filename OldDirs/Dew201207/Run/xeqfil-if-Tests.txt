// xeqfil-if-Tests.txt
//
////////////////////////////////////////////////////////////////
//  04/10/2018

//////// Good tests ////////
// 163 - 11/07/2019
if true {
    if false { print "Bad test 163a"; }
    if false { print "Bad test 163b"; }
    if false { print "Bad test 163c"; }
    if false { print "Bad test 163d"; }
    if true  { print "Good test 163"; }
}
print "-- --";

// 162 - 11/07/2019
if false { print "Bad test 162a"; }
if false { print "Bad test 162b"; }
if false { print "Bad test 162c"; }
if false { print "Bad test 162d"; }
if true  { print "Good test 162"; }
print "-- --";

// 161 - 11/06/2019
func func161 {
    if true {
        if false print "Bad test 161a";
        if false print "Bad test 161b";
        if false print "Bad test 161c";
        if false print "Bad test 161d";
        if true  print "Good test 161";
    }
}
call func161();
print "-- --";

// 160 - 11/06/2019
if true {
    if false print "Bad test 160a";
    if false print "Bad test 160b";
    if false print "Bad test 160c";
    if false print "Bad test 160d";
    if true  print "Good test 160";
}
print "-- --";

// 159 - 11/06/2019
if true {
    if false print "Bad test 160a";
    if false print "Bad test 160b";
    if false print "Bad test 160c";
    if false print "Bad test 160d";
    if true  print "Good test 160";
}
print "-- --";

// 158 - 11/04/2019
if true {
    if false { print "Bad test 159a"; }
    if false { print "Bad test 159b"; }
    if false { print "Bad test 159c"; }
    if false { print "Bad test 159d"; }
    if true  { print "Good test 159"; }
}
print "-- --";

// 158 - 11/04/2019
func func158 {
    if false { print "Bad test 158a"; }
    if false { print "Bad test 158b"; }
    if false { print "Bad test 158c"; }
    if false { print "Bad test 158d"; }
    if true  { print "Good test 158"; }
}
call func158();
print "-- --";

// 157 - 08/20/2019
if 3 + 2 = 5 if 1 + 4 = 6 if 1 + 7 = 9 { print "Bad test 157a"; }
else { print "Bad test 157b"; }
else { print "Good test 157"; }
else { print "Bad test 157c"; }
print "-- --";

// 156 - 08/20/2019
if 3 + 2 = 5 print "Good test 156";
else ;
print "-- --";

// 155 - 08/20/2019
if 3 + 4 = 5 ;
else print "Good test 155";
print "-- --";

// 154 - 08/13/2019
if 1 = 2
{
    if 4 = 2 print "Bad test 154a";
    print "Bad test 154b";
}
print "Good test 154";
print "-- --";

//////// Good tests ////////
// 153 - 07/31/2019
if 3 + 2 = 5 if 1 + 4 = 6 if 1 + 7 = 9 print "Bad test 153a";
else print "Bad test 153b";
else print "Good test 153";
else print "Bad test 153c";
print "-- --";

if 1 + 3 = 4 {
    if 1 + 4 = 5 {
        if 1 + 7 = 9 {
            print "Bad",; print "test",; print "152a";
        }
        print "Good",; print "test",; print "152";
    }
}
else { print "Bad",; print "test",; print "152b"; }
print "-- --";

if 1 + 3 = 4 {
    if 1 + 4 = 9 {
        if 1 + 7 = 8 {
            print "Bad",; print "test",; print "151a";
        }
    }
    print "Good",; print "test",; print "151";
}
else { print "Bad",; print "test",; print "151b"; }
print "-- --";

if 1 + 2 = 9 {
    if 1 + 4 = 5 {
        if 1 + 7 = 8 {
            print "Bad",; print "test",; print "150a";
        }
        else { print "Bad",; print "test",; print "150b"; }
    }
}
print "Good",; print "test",; print "150";
print "-- --";

if 1 + 2 = 9 {
    if 1 + 4 = 5 {
        if 1 + 7 = 8 {
            print "Bad",; print "test",; print "149a";
        }
    }
    else { print "Good",; print "test",; print "149b"; }
}
print "Good",; print "test",; print "149";
print "-- --";

if 1 + 2 = 9 {
    if 1 + 4 = 5 {
        if 1 + 7 = 8 {
            print "Bad",; print "test",; print "148";
        }
    }
}
else { print "Good",; print "test",; print "148"; }
print "-- --";

if 1 + 2 = 3 {
    if 1 + 4 = 5 {
        if 1 + 7 = 9 {
            print "Bad",; print "test",; print "147";
        }
    }
}
print "Good",; print "test",; print "147";
print "-- --";

if 1 + 2 = 3 {
    if 1 + 4 = 9 {
        if 1 + 7 = 8 {
            print "Bad",; print "test",; print "146";
        }
    }
}
print "Good",; print "test",; print "146";
print "-- --";

if 1 + 2 = 9 {
    if 1 + 4 = 5 {
        if 1 + 7 = 8 {
            print "Bad",; print "test",; print "145";
        }
    }
}
print "Good",; print "test",; print "145";
print "-- --";

if 1 + 2 = 3 {
    if 1 + 4 = 6 {
        if 1 + 7 = 8 {
            print "Bad",; print "test",; print "144a";
        } else {
            print "Bad",; print "test",; print "144b";
        }
    } else {
        print "Good",; print "test",; print "144";
    }
} else {
    print "Bad",; print "test",; print "144c";
}
print "-- --";

if 1 + 2 = 9 {
    if 1 + 5 = 6
        if 1 + 7 = 8 {
            print "Bad test 143a";
        } else
            print "Bad test 143b";
    else
        print "Bad test 143c";
} else
    print "Good test 143";
print "-- --";

if 1 + 2 = 3 {
    if 1 + 5 = 9 {
        if 1 + 7 = 8
            print "Bad test 142a";
        else
            print "Bad test 142b";
    } else
        print "Good test 142";
} else
    print "Bad test 142c";
print "-- --";

if 1 + 2 = 3 {
    if 1 + 5 = 6 {
        if 1 + 7 = 9
            print "Bad test 141a";
        else
            print "Good test 141";
    } else
        print "Bad test 141b";
} else
    print "Bad test 141c";
print "-- --";

{ if 1 + 2 = 3 {
    if 1 + 5 = 6
        if 1 + 7 = 8
            print "Good test 140";
        else
            print "Bad test 140a";
    else
        print "Bad test 140b";
} else
    print "Bad test 140c"; }
print "-- --";

if 1 + 2 = 9
    if 1 + 5 = 6
        if 1 + 7 = 8 {
            print "Bad test 133a";
        } else
            print "Bad test 133b";
    else
        print "Bad test 133c";
else
    print "Good test 133";
print "-- --";

if 1 + 2 = 3
    if 1 + 5 = 9 {
        if 1 + 7 = 8
            print "Bad test 132a";
        else
            print "Bad test 132b";
    } else
        print "Good test 132";
else
    print "Bad test 132c";
print "-- --";

if 1 + 2 = 3 {
    if 1 + 5 = 6
        if 1 + 7 = 9
            print "Bad test 131a";
        else
            print "Good test 131";
    else
        print "Bad test 131b";
} else
    print "Bad test 131c";
print "-- --";

{ if 1 + 2 = 3
    if 1 + 5 = 6
        if 1 + 7 = 8
            print "Good test 130";
        else
            print "Bad test 130a";
    else
        print "Bad test 130b";
else
    print "Bad test 130c"; }
print "-- --";

if 1 + 2 = 9
    if 1 + 5 = 6
        if 1 + 7 = 8
            print "Bad test 123a";
        else
            print "Bad test 123b";
    else
        print "Bad test 123c";
else
    print "Good test 123";
print "-- --";

if 1 + 2 = 3
    if 1 + 5 = 9
        if 1 + 7 = 8
            print "Bad test 122a";
        else
            print "Bad test 122b";
    else
        print "Good test 122";
else
    print "Bad test 122c";
print "-- --";

if 1 + 2 = 3
    if 1 + 5 = 6
        if 1 + 7 = 9
            print "Bad test 121a";
        else
            print "Good test 121";
    else
        print "Bad test 121b";
else
    print "Bad test 121c";
print "-- --";

if 1 + 2 = 3
    if 1 + 5 = 6
        if 1 + 7 = 8
            print "Good test 120";
        else
            print "Bad test 120a";
    else
        print "Bad test 120b";
else
    print "Bad test 120c";
print "-- --";

if 1 + 2 = 4 if 1 + 5 = 7 {
    if 11 + 12 = 23 if 1 + 8 = 9 { print "Bad",; print "test",; print "119a"; }
    else { print "Bad",; print "test",; print "119b"; } }
else { print "Bad",; print "test",; print "119c"; }
else { print "Good",; print "test",; print "119"; }
print "-- --";

if 1 + 2 = 4 if 1 + 5 = 7 if 11 + 12 = 23 { if 1 + 8 = 9 { print "Bad",; print "test",; print "118a"; } }
else { print "Bad",; print "test",; print "118b"; }
else { print "Bad",; print "test",; print "118c"; }
else { print "Good",; print "test",; print "118"; }
print "-- --";

if 1 + 2 = 4 if 1 + 5 = 7 { if 1 + 8 = 9 { print "Bad",; print "test",; print "117a"; } }
else { print "Bad",; print "test",; print "117b"; }
else { print "Good",; print "test",; print "117"; }
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 7 { if 1 + 8 = 9 { print "Bad",; print "test",; print "116a"; } }
else { print "Good",; print "test",; print "116"; }
else { print "Bad",; print "test",; print "116b"; }
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 { if 1 + 8 = 9 { print "Good",; print "test",; print "115"; } }
else { print "Bad",; print "test",; print "115a"; }
else { print "Bad",; print "test",; print "115b"; }
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 { if 1 + 8 = 9 { print "Good",; print "test",; print "114"; } }
else { print "Bad",; print "test",; print "114"; }
print "-- --";

if 1 + 2 = 3 { if 1 + 5 = 6 if 1 + 8 = 9 { print "Good",; print "test",; print "113"; } }
else { print "Bad",; print "test",; print "113"; }
print "-- --";

if 1 + 2 = 3 { if 1 + 5 = 6 { print "Good",; print "test",; print "112"; } }
else { print "Bad",; print "test",; print "112"; }
print "-- --";

if 1 + 2 = 5 if 1 + 4 = 6 if 1 + 7 = 9 { print "Bad",; print "test",; print "111a"; }
else { print "Bad",; print "test",; print "111b"; }
else { print "Bad",; print "test",; print "111c"; }
else { print "Good",; print "test",; print "111"; }
print "-- --";

if 1 + 2 = 3 if 1 + 4 = 6 if 1 + 7 = 9 { print "Bad",; print "test",; print "110a"; }
else { print "Bad",; print "test",; print "110b"; }
else { print "Good",; print "test",; print "110"; }
else { print "Bad",; print "test",; print "110c"; }
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 if 1 + 7 = 9 { print "Bad",; print "test",; print "109a"; }
else { print "Good",; print "test",; print "109"; }
else { print "Bad",; print "test",; print "109b"; }
else { print "Bad",; print "test",; print "109c"; }
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 if 1 + 8 = 9 { print "Good",; print "test",; print "108"; }
else { print "Bad",; print "test",; print "108a"; }
else { print "Bad",; print "test",; print "108b"; }
else { print "Bad",; print "test",; print "108c"; }
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 if 1 + 8 = 9 { print "Good",; print "test",; print "107"; }
else { print "Bad",; print "test",; print "107a"; }
else { print "Bad",; print "test",; print "107b"; }
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 if 1 + 8 = 9 { print "Good",; print "test",; print "106"; }
else { print "Bad",; print "test",; print "106"; }
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 if 1 + 8 = 9 { print "Good",; print "test",; print "105"; }
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 { print "Good",; print "test",; print "104"; }
print "-- --";

if 1 + 2 = 4 { print "Bad",; print "test",; print "103"; }
else { print "Good",; print "test",; print "103"; }
print "-- --";

if 1 + 2 = 3 { print "Good",; print "test",; print "102"; }
else { print "Bad",; print "test",; print "102"; }
print "-- --";

if 1 + 2 = 3 { print "Good",; print "test",; print "101"; }
print "-- --";

if 1 + 2 = 5 if 1 + 4 = 6 if 1 + 7 = 9 print "Bad test 11a";
else print "Bad test 11b";
else print "Bad test 11c";
else print "Good test 11";
print "-- --";

if 1 + 2 = 3 if 1 + 4 = 6 if 1 + 7 = 9 print "Bad test 10a";
else print "Bad test 10b";
else print "Good test 10";
else print "Bad test 10c";
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 if 1 + 7 = 9 print "Bad test 9a";
else print "Good test 9";
else print "Bad test 9b";
else print "Bad test 9c";
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 if 1 + 8 = 9 print "Good test 8";
else print "Bad test 8a";
else print "Bad test 8b";
else print "Bad test 8c";
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 if 1 + 8 = 9 print "Good test 7";
else print "Bad test 7a";
else print "Bad test 7b";
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 if 1 + 8 = 9 print "Good test 6";
else print "Bad test 6";
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 if 1 + 8 = 9 print "Good test 5";
print "-- --";

if 1 + 2 = 3 if 1 + 5 = 6 print "Good test 4";
print "-- --";

if 1 + 2 = 4 print "Bad test 3";
else print "Good test 3";
print "-- --";

if 1 + 2 = 3 print "Good test 2";
else print "Bad test 2";
print "-- --";

if 1 + 2 = 3 print "Good test 1";
print "--end if tests--";

//////// End tests ////////
