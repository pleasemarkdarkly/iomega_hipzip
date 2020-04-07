//  Copyright (C) 1996-2001 Jean-Claude Wippler <jcw@equi4.com>
//
//  Regression test program, storage tests, part 3

#include "regress.h"

/////////////////////////////////////////////////////////////////////////////

void TestStores4()
{
    B(s30, Memo storage, 0) W(s30a);
    {
        c4_Bytes hi ("hi", 2);
        c4_Bytes gday ("gday", 4);
        c4_Bytes hello ("hello", 5);

        c4_MemoProp p1 ("p1");
        c4_Storage s1 ("s30a", 1);
        s1.SetStructure("a[p1:B]");
        c4_View v1 = s1.View("a");

        v1.Add(p1 [hi]);
            A(p1 (v1[0]) == hi);
        v1.Add(p1 [hello]);
            A(p1 (v1[0]) == hi);
            A(p1 (v1[1]) == hello);
        v1.InsertAt(1, p1 [gday]);
            A(p1 (v1[0]) == hi);
            A(p1 (v1[1]) == gday);
            A(p1 (v1[2]) == hello);
        s1.Commit();
            A(p1 (v1[0]) == hi);
            A(p1 (v1[1]) == gday);
            A(p1 (v1[2]) == hello);

    }   D(s30a); R(s30a); E;

        // this failed in the unbuffered 1.8.5a interim release in Mk4tcl 1.0.5
    B(s31, Check sort buffer use, 0) W(s31a);
    {
        c4_IntProp p1 ("p1");
        c4_Storage s1 ("s31a", 1);
        s1.SetStructure("a[p1:I]");
            c4_View v1 = s1.View("a");
            v1.Add(p1 [3]);
            v1.Add(p1 [1]);
            v1.Add(p1 [2]);
        s1.Commit();

        c4_View v2 = v1.SortOn(p1);
            A(v2.GetSize() == 3);
            A(p1 (v2[0]) == 1);
            A(p1 (v2[1]) == 2);
            A(p1 (v2[2]) == 3);

    }   D(s31a); R(s31a); E;

        // this failed in 1.8.6, fixed 19990828
    B(s32, Set memo empty or same size, 0) W(s32a);
    {
        c4_Bytes empty;
        c4_Bytes full ("full", 4);
        c4_Bytes more ("more", 4);

        c4_MemoProp p1 ("p1");
        c4_Storage s1 ("s32a", 1);
        s1.SetStructure("a[p1:B]");
        c4_View v1 = s1.View("a");

        v1.Add(p1 [full]);
            A(p1 (v1[0]) == full);
        s1.Commit();
            A(p1 (v1[0]) == full);
        
        p1 (v1[0]) = empty;
            A(p1 (v1[0]) == empty);
        s1.Commit();
            A(p1 (v1[0]) == empty);

        p1 (v1[0]) = more;
            A(p1 (v1[0]) == more);
        s1.Commit();
            A(p1 (v1[0]) == more);

        p1 (v1[0]) = full;
            A(p1 (v1[0]) == full);
        s1.Commit();
            A(p1 (v1[0]) == full);

    }   D(s32a); R(s32a); E;

        // this failed in 1.8.6, fixed 19990828
    B(s33, Serialize memo fields, 0) W(s33a); W(s33b); W(s33c);
    {
        c4_Bytes hi ("hi", 2);
        c4_Bytes gday ("gday", 4);
        c4_Bytes hello ("hello", 5);

        c4_MemoProp p1 ("p1");

        c4_Storage s1 ("s33a", 1);
        s1.SetStructure("a[p1:B]");
        c4_View v1 = s1.View("a");

        v1.Add(p1 [hi]);
        v1.Add(p1 [gday]);
        v1.Add(p1 [hello]);
            A(p1 (v1[0]) == hi);
            A(p1 (v1[1]) == gday);
            A(p1 (v1[2]) == hello);
        s1.Commit();
            A(p1 (v1[0]) == hi);
            A(p1 (v1[1]) == gday);
            A(p1 (v1[2]) == hello);

        {
            c4_FileStream fs1 (fopen("s33b", "wb"), true);
            s1.SaveTo(fs1);
        }

        c4_Storage s2 ("s33c", 1);

        c4_FileStream fs2 (fopen("s33b", "rb"), true);
        s2.LoadFrom(fs2);

        c4_View v2 = s2.View("a");
            A(p1 (v2[0]) == hi);
            A(p1 (v2[1]) == gday);
            A(p1 (v2[2]) == hello);
        s2.Commit();
            A(p1 (v2[0]) == hi);
            A(p1 (v2[1]) == gday);
            A(p1 (v2[2]) == hello);
        s2.Commit();
            A(p1 (v2[0]) == hi);
            A(p1 (v2[1]) == gday);
            A(p1 (v2[2]) == hello);

    }   D(s33a); D(s33b); D(s33c); R(s33a); R(s33b); R(s33c); E;

        // check smarter commit and commit failure on r/o
    B(s34, Smart and failed commits, 0) W(s34a);
    {
        c4_IntProp p1 ("p1");
        {
            c4_Storage s1 ("s34a", 1);
            s1.SetStructure("a[p1:I]");
            c4_View v1 = s1.View("a");
            v1.Add(p1 [111]);
                A(v1.GetSize() == 1);
                A(p1 (v1[0]) == 111);
            bool f1 = s1.Commit();
                A(f1);
                A(v1.GetSize() == 1);
                A(p1 (v1[0]) == 111);
            bool f2 = s1.Commit();
                A(f2); // succeeds, but should not write anything
                A(v1.GetSize() == 1);
                A(p1 (v1[0]) == 111);
        }
        {
            c4_Storage s1 ("s34a", 0);
            c4_View v1 = s1.View("a");
            v1.Add(p1 [222]);
                A(v1.GetSize() == 2);
                A(p1 (v1[0]) == 111);
                A(p1 (v1[1]) == 222);
            bool f1 = s1.Commit();
                A(!f1);
                A(v1.GetSize() == 2);
                A(p1 (v1[0]) == 111);
                A(p1 (v1[1]) == 222);
        }
    }   D(s34a); R(s34a); E;

    B(s35, Datafile with preamble, 0) W(s35a);
    {
        {
            c4_FileStream fs1 (fopen("s35a", "wb"), true);
            fs1.Write("abc", 3);
        }
        c4_IntProp p1 ("p1");
        {
            c4_Storage s1 ("s35a", 1);
            s1.SetStructure("a[p1:I]");
            c4_View v1 = s1.View("a");
            v1.Add(p1 [111]);
                A(v1.GetSize() == 1);
                A(p1 (v1[0]) == 111);
            bool f1 = s1.Commit();
                A(f1);
                A(v1.GetSize() == 1);
                A(p1 (v1[0]) == 111);
            bool f2 = s1.Commit();
                A(f2); // succeeds, but should not write anything
                A(v1.GetSize() == 1);
                A(p1 (v1[0]) == 111);
        }
        {
            c4_FileStream fs1 (fopen("s35a", "rb"), true);
	    char buffer [10];
            int n1 = fs1.Read(buffer, 3);
		A(n1 == 3);
		A(c4_String (buffer, 3) == "abc");
        }
        {
            c4_Storage s1 ("s35a", 0);
            c4_View v1 = s1.View("a");
                A(v1.GetSize() == 1);
                A(p1 (v1[0]) == 111);
            v1.Add(p1 [222]);
                A(v1.GetSize() == 2);
                A(p1 (v1[0]) == 111);
                A(p1 (v1[1]) == 222);
            bool f1 = s1.Commit();
                A(!f1);
                A(v1.GetSize() == 2);
                A(p1 (v1[0]) == 111);
                A(p1 (v1[1]) == 222);
        }
    }   D(s35a); R(s35a); E;
}

/////////////////////////////////////////////////////////////////////////////
