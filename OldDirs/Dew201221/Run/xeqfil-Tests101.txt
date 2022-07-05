// xeqfil-Tests101.txt
//
////////////////////////////////////////////////////////////////
//  04/11/2018

set ngood = 0;
set nerrs = 0;

//////// Good tests ////////

// 101-01
set ii = 0;
while ii < 8 set ii = ii + 1;
if ii = 8 set ngood = ngood + 1; else { print "Test 101-01 failed"; set nerrs = nerrs + 1; }

// 101-02
set ii = 0;
while ii < 8 { set ii = ii + 1; }
if ii = 8 set ngood = ngood + 1; else { print "Test 101-02 failed"; set nerrs = nerrs + 1; }

// 101-03
set nn = 0;
set ii = 0;
while ii < 3 {
    set jj = 0;
    while jj < 3 {
        set kk = 0;
        while kk < 3 {
            set nn = nn + 1;
            set kk = kk + 1;
        }
        set jj = jj + 1;
    }
    set ii = ii + 1;
}
if nn = 27 set ngood = ngood + 1; else { print "Test 101-03 failed"; set nerrs = nerrs + 1; }

// 101-04
set nn = 0;
set ii = 0;
while ii < 3 {
    set jj = 0;
    while jj < 3 {
        set kk = 0;
        while kk < 3 {
            set nn = nn + 1;
            if (nn > 0) set kk = ((((((kk)) + 1))));
        }
        set jj = jj + 1;
    }
    set ii = ii + 1;
}
if nn = 27 set ngood = ngood + 1; else { print "Test 101-04 failed"; set nerrs = nerrs + 1; }

///////////////////////////
func inc(nn) { return nn + 1; }
func dec(nn) { return nn - 1; }
func add(mm, nn) { if nn = 0 return mm; else { return add(inc(mm), dec(nn)); } }
func mul(mm, nn) { if mm = 0 or nn = 0 return 0; else { if nn = 1 return mm; else return add(mm,  mul(mm, dec(nn))); } }
func pow(mm, nn) { if nn = 0 return 1; else return mul(mm,  pow(mm, dec(nn))); }
func fact(nn) { if nn = 0 { return 1; } else { return mul(nn, fact(dec(nn))); } }

if fact(5) = 120 set ngood = ngood + 1; else { print "Test 101-10 failed"; set nerrs = nerrs + 1; }
if pow(2,6) = 64 set ngood = ngood + 1; else { print "Test 101-11 failed"; set nerrs = nerrs + 1; }
if fact(pow(2,2)) = 24 set ngood = ngood + 1; else { print "Test 101-12 failed"; set nerrs = nerrs + 1; }

///////////////////////////
func inc01(ii, nn) { call ii.set(ii.get() + 1); return nn + 1; }
func dec01(ii, nn) { call ii.set(ii.get() + 1); return nn - 1; }
func add01(ii, mm, nn) { if nn = 0 return mm; else { return add01(ii, inc01(ii, mm), dec01(ii, nn)); } }
func mul01(ii, mm, nn) { if mm = 0 or nn = 0 return 0; else { if nn = 1 return mm; else return add01(ii, mm,  mul01(ii, mm, dec01(ii, nn))); } }
func pow01(ii, mm, nn) { if nn = 0 return 1; else return mul01(ii, mm,  pow01(ii, mm, dec01(ii, nn))); }

set ii=new Integer(0);
if pow01(ii, 2, 6) = 64 set ngood = ngood + 1; else { print "Test 101-20"; set nerrs = nerrs + 1; }
if ii.get() = 2667 set ngood = ngood + 1; else { print "Test 101-21 failed"; set nerrs = nerrs + 1; }

///////////////////////////
set ii01=new Integer(25);
if ii01.get() = 25 set ngood = ngood + 1; else { print "Test 101-30 failed"; set nerrs = nerrs + 1; }
call ii01.set(1 + ii01.get() * ii01.get());
if ii01.get() = 626 set ngood = ngood + 1; else { print "Test 101-31 failed"; set nerrs = nerrs + 1; }

set ii = 0;
while ii < 8 { set aa[ii] = new Integer(ii + 100); set ii = ii + 1; }
set ii = 0;
while ii < 8 {
    if aa[ii].get() = ii + 100 set ngood = ngood + 1; else { print "Test 101-32 (",ii,") failed"; set nerrs = nerrs + 1; }
    set ii = ii + 1;
}
if ii = 8 set ngood = ngood + 1; else { print "Test 101-33 failed"; set nerrs = nerrs + 1; }

///////////////////////////
// Group 04
func fun04(aa04) { set aa04[0] = aa04[0] + 10; set aa04[1] = aa04[0] + 20; }
set aa04[0] = 1;
call fun04(aa04);
if aa04[0] = 11 set ngood = ngood + 1; else { print "Test 101-40 failed"; set nerrs = nerrs + 1; }
if aa04[1] = 31 set ngood = ngood + 1; else { print "Test 101-41 failed"; set nerrs = nerrs + 1; }

func farr04(arr04)
{
    set arr04[1] = "t1";
    set arr04[2][2] = 22;
    set arr04[3][3][3] = 333;
    set arr04[4][4][4][4] = "t4444";
}
set arr04 = new Array();
call farr04(arr04);
if arr04[1] = "t1" set ngood = ngood + 1; else { print "Test 101-42 failed"; set nerrs = nerrs + 1; }
if arr04[2][2] = 22 set ngood = ngood + 1; else { print "Test 101-43 failed"; set nerrs = nerrs + 1; }
if arr04[3][3][3] = 333 set ngood = ngood + 1; else { print "Test 101-44 failed"; set nerrs = nerrs + 1; }
if arr04[4][4][4][4] = "t4444" set ngood = ngood + 1; else { print "Test 101-45 failed"; set nerrs = nerrs + 1; }
set tmptmp04 = arr04;
set tmptmp04[3][3][3] = -tmptmp04[3][3][3];
set tmp04 = tmptmp04;
if tmp04[1] = "t1" set ngood = ngood + 1; else { print "Test 101-46 failed"; set nerrs = nerrs + 1; }
if tmp04[2][2] = 22 set ngood = ngood + 1; else { print "Test 101-47 failed"; set nerrs = nerrs + 1; }
if tmp04[3][3][3] = -333 set ngood = ngood + 1; else { print "Test 101-48 failed"; set nerrs = nerrs + 1; }
if tmp04[4][4][4][4] = "t4444" set ngood = ngood + 1; else { print "Test 101-49 failed"; set nerrs = nerrs + 1; }

func mul04(aa, bb) { return new Integer(aa.get() * bb.get()); }
if mul04(new Integer(2), new Integer(3)).get() = 6 set ngood = ngood + 1; else { print "Test 101-491 failed"; set nerrs = nerrs + 1; }

///////////////////////////
if 1 + 2 * 3 = 7 set ngood = ngood + 1; else { print "Test 101-50 failed"; set nerrs = nerrs + 1; }
if (1 + 2) * 3 = 9 set ngood = ngood + 1; else { print "Test 101-51 failed"; set nerrs = nerrs + 1; }
if (1 < 2 or 3 > 4 and 5 > 6) = true set ngood = ngood + 1; else { print "Test 101-52 failed"; set nerrs = nerrs + 1; }
if (1 < 2 or 3 > 4) and 5 > 6 = false set ngood = ngood + 1; else { print "Test 101-53 failed"; set nerrs = nerrs + 1; }
if (1 > 2 and 3 < 4 and 5 < 6) = false set ngood = ngood + 1; else { print "Test 101-54 failed"; set nerrs = nerrs + 1; }
if (1 > 2 or 3 > 4 or 5 < 6) = true set ngood = ngood + 1; else { print "Test 101-55 failed"; set nerrs = nerrs + 1; }
if "aa\\bb\\cc" = F"aa\bb\cc" set ngood = ngood + 1; else { print "Test 101-56 failed"; set nerrs = nerrs + 1; }
if F"aa\\bb\\cc" <> "aa\\bb\\cc" set ngood = ngood + 1; else { print "Test 101-57 failed"; set nerrs = nerrs + 1; }
if "Alpha" "Beta"    "Gamma" = "AlphaBetaGamma" set ngood = ngood + 1; else { print "Test 101-58 failed"; set nerrs = nerrs + 1; }
if "Alpha "  // One
   "Beta "   // Two
   "Gamma" = "Alpha Beta Gamma" set ngood = ngood + 1; else { print "Test 101-59 failed"; set nerrs = nerrs + 1; }
///////////////////////////

///////////////////////////
if defined("ngood") and defined("nerrs") set ngood = ngood + 1; else { print "Test 101-60 failed"; set nerrs = nerrs + 1; }
if not defined("pi31415926") set ngood = ngood + 1; else { print "Test 101-61 failed"; set nerrs = nerrs + 1; }
func mul06(pi31415926) { set e2718281828 = pi31415926; }
if defined("mul06") set ngood = ngood + 1; else { print "Test 101-63 failed"; set nerrs = nerrs + 1; }
if not defined("pi31415926") and not defined("e2718281828") set ngood = ngood + 1; else { print "Test 101-64 failed"; set nerrs = nerrs + 1; }
if defined("Integer") and defined("defined") set ngood = ngood + 1; else { print "Test 101-65 failed"; set nerrs = nerrs + 1; }
///////////////////////////

//////// End tests ////////

if nerrs = 0 print "All", ngood, "tests successful.";
else print "****", nerrs, "tests failed, out of", nerrs + ngood;
