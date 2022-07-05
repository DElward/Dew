// xeqfil-while-Tests.txt
//
////////////////////////////////////////////////////////////////
//  08/21/2019

//////// Good tests ////////

// 208 - 08/22/2019
set ii = 0;
set tt = 0;
while ii < 8 {
    set jj = 0;
    if ii < 4 {
        while jj < 6 {
            if 1 + jj < 777 {
                set kk = 0;
                while kk < 4 {
                    set kk = kk + 1;
                    if (kk < 2) set tt = tt + 1;
                }
            } else
                set tt = tt + 1000;
            set jj = jj + 1;
        }
    } else
        set tt = tt + 100;
    set ii = ii + 1;
}
if tt = 424 print "Good test 208";
else print "Bad test 208, tt=", tt;
print "-- --";

// 207 - 08/22/2019
set ii = 0;
set tt = 0;
while ii < 8 {
    set jj = 0;
    if ii < 4 {
        while jj < 6 {
            if 1 + jj < 777
                set tt = tt + 1;
            else
                set tt = tt + 1000;
            set jj = jj + 1;
        }
    } else
        set tt = tt + 100;
    set ii = ii + 1;
}
if tt = 424 print "Good test 207";
else print "Bad test 207, tt=", tt;
print "-- --";

// 206 - 08/22/2019
set ii = 0;
set tt = 0;
while ii < 8 {
    set jj = 0;
    if ii < 4 {
        while jj < 6 {
            if 1 + jj < 777 {
                set tt = tt + 1;
            } else {
                set tt = tt + 1000;
            }
            set jj = jj + 1;
        }
    } else {
        set tt = tt + 100;
    }
    set ii = ii + 1;
}
if tt = 424 print "Good test 206";
else print "Bad test 206, tt=", tt;
print "-- --";

// 205 - 08/22/2019
set ii = 0;
set tt = 0;
while ii < 8 {
    set jj = 0;
    while jj < 6 {
        if 1 + jj = 777 {
            set tt = tt + 1000;
        } else {
            set tt = tt + 1;
        }
        set jj = jj + 1;
    }
    set ii = ii + 1;
}
if tt = 48 print "Good test 205";
else print "Bad test 205, tt=", tt;
print "-- --";

// 204 - 08/21/2019
set ii = 0;
set tt = 0;
while ii < 8 {
    set jj = 0;
    while jj < 6 {
        set kk = 0;
        while kk < 4 {
            set kk = kk + 1;
            set tt = tt + 1;
        }
        set jj = jj + 1;
    }
    set ii = ii + 1;
}
if tt = 192 print "Good test 204";
else print "Bad test 204";
print "-- --";

// 203 - 08/21/2019
set ii = 0;
set tt = 0;
while ii < 8 {
    set jj = 0;
    while jj < 6 { set jj = jj + 1; set tt = tt + 1; }
    set ii = ii + 1;
}
if tt = 48 print "Good test 203";
else print "Bad test 203";
print "-- --";

// 202 - 08/21/2019
set ii = 0;
while ii < 8 { set ii = ii + 1; }
if ii = 8 print "Good test 202";
else print "Bad test 202";
print "-- --";

// 201 - 08/21/2019
set ii = 0;
while ii < 8 set ii = ii + 1;
if ii = 8 print "Good test 201";
else print "Bad test 201";
print "-- --";

print "--end while tests--";

//////// End tests ////////
