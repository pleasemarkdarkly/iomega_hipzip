// tbasics.cpp -- Regression test program, basic tests
// $Id: tbasics.cpp,v 1.5 2001/11/25 21:10:39 wcvs Exp $
// This is part of MetaKit, the homepage is http://www.equi4.com/metakit/

#include "regress.h"

  static int ViewSize(c4_View v)
  {
    return v.GetSize();
  }

void TestBasics()
{      
  B(b00, Should fail, 0)
  {
    A(false);
  } E;

  B(b01, Should succeed, 0)
  {
    A(sizeof (t4_byte) == 1);
    A(sizeof (short) == 2);
    A(sizeof (t4_i32) == 4);
    A(sizeof (float) == 4);
    A(sizeof (double) == 8);
  } E;

  B(b02, Int property, 0)
  {
    c4_Row r1;
    c4_IntProp p1 ("p1");
    p1 (r1) = 1234567890L;
      long x1 = p1 (r1);
      A(x1 == 1234567890L);
  } E;

  B(b03, Float property, 0)
  {
    c4_Row r1;
    c4_FloatProp p1 ("p1");
    p1 (r1) = 123.456;
      double x1 = p1 (r1);
      A((float) x1 == (float) 123.456);
  } E;

  B(b04, String property, 0)
  {
    c4_Row r1;
    c4_StringProp p1 ("p1");
    p1 (r1) = "abc";
      const char* x1 = p1 (r1);
      A((c4_String) x1 == "abc");
  } E;

  B(b05, View property, 0)
  {
    c4_View v1;
    c4_Row r1;
    c4_ViewProp p1 ("p1");
    p1 (r1) = v1;
      c4_View x1 = p1 (r1);
        // compare cursors to make sure this is the same sequence
      A(x1[0] == v1[0]);
  } E;

  B(b06, View construction, 0)
  {
    c4_IntProp p1 ("p1"), p2 ("p2"), p3 ("p3");
    c4_View v1 = (p1, p3, p2);
      A(v1.FindProperty(p1.GetId()) == 0);
      A(v1.FindProperty(p2.GetId()) == 2);
      A(v1.FindProperty(p3.GetId()) == 1);
  } E;

  B(b07, Row manipulation, 0)
  {
    c4_StringProp p1 ("p1"), p2 ("p2");
    c4_IntProp p3 ("p3");
    c4_Row r1;
    p1 (r1) =  "look at this" ;
      const char* x1 = p1 (r1);
      A(x1 == (c4_String) "look at this");
    r1 = p1 [ "what's in a" ] + p2 [ "name..." ];
    c4_String t = (const char*) p2 (r1);
    p1 (r1) = t + (const char*) (p1 (r1));
    p2 (r1) = p1 (r1);
      c4_String x2 = (const char*) p1 (r1); // 2000-03-16, store as c4_String
      A(x2 == "name...what's in a");
        // the above change avoids an evaluation order issue in assert below
      A(x2 == p2 (r1));
    p3 (r1) = 12345;
    p3 (r1) = p3 (r1) + 123;
      int x3 = p3 (r1);
      A(x3 == 12345 + 123);
  } E;

  B(b08, Row expressions, 0)
  {
    c4_StringProp p1 ("p1"), p2 ("p2");
    c4_IntProp p3 ("p3");
    c4_Row r1;
    c4_View v1 = (p1, p2, p3);
    v1.SetSize(5);
    r1 = v1[1];
    v1[2] = v1[1];
    v1[3] = r1;
    v1[4] = v1[4];
    r1 = r1;
  } E;

  B(b09, View manipulation, 0)
  {
    c4_StringProp p1 ("p1"), p2 ("p2");
    c4_Row r1 = p1 ["One"] + p2 ["Two"];
    c4_Row r2;
    c4_View v1;
    v1.Add(r1);
    v1.Add(r2);
    v1.Add(r1);
      A(v1.GetSize() == 3);
      A(v1[0] == r1);
      A(v1[1] == r2);
      A(v1[2] == r1);
    v1.RemoveAt(1, 1);
      A(v1.GetSize() == 2);
      A(v1[0] == r1);
      A(v1[0] == v1[1]);
  } E;

  B(b10, View sorting, 0)
  {
    c4_IntProp p1 ("p1");
    c4_View v1;
    v1.Add(p1 [111]);
    v1.Add(p1 [222]);
    v1.Add(p1 [333]);
    v1.Add(p1 [345]);
    v1.Add(p1 [234]);
    v1.Add(p1 [123]);
    c4_View v2 = v1.Sort();
      A(v2.GetSize() == 6);
      A(p1 (v2[0]) == 111);
      A(p1 (v2[1]) == 123);
      A(p1 (v2[2]) == 222);
      A(p1 (v2[3]) == 234);
      A(p1 (v2[4]) == 333);
      A(p1 (v2[5]) == 345);
  } E;

  B(b11, View selection, 0)
  {
    c4_IntProp p1 ("p1");
    c4_View v1;
    v1.Add(p1 [111]);
    v1.Add(p1 [222]);
    v1.Add(p1 [333]);
    v1.Add(p1 [345]);
    v1.Add(p1 [234]);
    v1.Add(p1 [123]);
    c4_View v2 = v1.SelectRange(p1 [200], p1 [333]);
      A(v2.GetSize() == 3);
      A(p1 (v2[0]) == 222);
      A(p1 (v2[1]) == 333);
      A(p1 (v2[2]) == 234);
  } E;

  B(b12, Add after remove, 0)
  {
    c4_StringProp p1 ("p1");
    c4_View v1;
    v1.Add(p1 ["abc"]);
      A(v1.GetSize() == 1);
    v1.RemoveAt(0);
      A(v1.GetSize() == 0);
    v1.Add(p1 ["def"]);
      A(v1.GetSize() == 1);
  } E;

  B(b13, Clear view entry, 0)
  {
    c4_IntProp p1 ("p1");
    c4_View v1;

    v1.Add(p1 [123]);
      A(v1.GetSize() == 1);
      A(p1 (v1[0]) == 123);

    v1[0] = c4_Row ();
      A(v1.GetSize() == 1);
      A(p1 (v1[0]) == 0);
  } E;

  B(b14, Empty view outlives temp storage, 0)
  {
    c4_View v1;
    c4_Storage s1;
    v1 = s1.GetAs("a[p1:I,p2:S]");
  } E;

  B(b15, View outlives temp storage, 0)
  {
    c4_IntProp p1 ("p1");
    c4_View v1;

    {
      c4_Storage s1;
      v1 = s1.GetAs("a[p1:I,p2:S]");
      v1.Add(p1 [123]);
    }

      // 19990916 - semantics changed, view now 1 row, but 0 props
    A(v1.GetSize() == 1);
    A(v1.NumProperties() == 0);
    //A(p1 (v1[0]) == 123);
  } E;

  B(b16, View outlives cleared temp storage, 0)
  {
    c4_IntProp p1 ("p1");
    c4_View v1;

    {
      c4_Storage s1;
      v1 = s1.GetAs("a[p1:I,p2:S]");
      v1.Add(p1 [123]);
      v1.RemoveAll();
    }

    A(v1.GetSize() == 0);
      v1.Add(p1 [123]);
    A(v1.GetSize() == 1);
    A(p1 (v1[0]) == 123);
  } E;

  B(b17, Double property, 0)
  {
    c4_Row r1;
    c4_DoubleProp p1 ("p1");
    p1 (r1) = 1234.5678;
      double x1 = p1 (r1);
      A(x1 == (double) 1234.5678);
  } E;

  B(b18, SetAtGrow usage, 0)
  {
    c4_IntProp p1 ("p1");
    c4_View v1;

    v1.SetAtGrow(3, p1 [333]);
    v1.SetAtGrow(1, p1 [111]);
    v1.SetAtGrow(5, p1 [555]);

    A(v1.GetSize() == 6);
    A(p1 (v1[1]) == 111);
    A(p1 (v1[3]) == 333);
    A(p1 (v1[5]) == 555);
  } E;

  B(b19, Bytes property, 0)
  {
    c4_Row r1;
    c4_BytesProp p1 ("p1");
    c4_Bytes x1 ("hi!", 3);

    p1 (r1) = x1;
      c4_Bytes x2 = p1 (r1);
      A(x1 == x2);
  } E;

  B(b20, Search sorted view, 0)
  {
    c4_IntProp p1 ("p1");
    c4_StringProp p2 ("p2");
    c4_View v1;
    v1.Add(p1 [111] + p2 ["one"]);
    v1.Add(p1 [222] + p2 ["two"]);
    v1.Add(p1 [333] + p2 ["three"]);
    v1.Add(p1 [345] + p2 ["four"]);
    v1.Add(p1 [234] + p2 ["five"]);
    v1.Add(p1 [123] + p2 ["six"]);
    c4_View v2 = v1.Sort();
      A(v2.GetSize() == 6);
      A(p1 (v2[0]) == 111);
      A(p1 (v2[1]) == 123);
      A(p1 (v2[2]) == 222);
      A(p1 (v2[3]) == 234);
      A(p1 (v2[4]) == 333);
      A(p1 (v2[5]) == 345);
      A(v2.Search(p1 [123]) == 1);
      A(v2.Search(p1 [100]) == 0);
      A(v2.Search(p1 [200]) == 2);
      A(v2.Search(p1 [400]) == 6);
    c4_View v3 = v1.SortOn(p2);
      A(v3.GetSize() == 6);
      A(p1 (v3[0]) == 234);
      A(p1 (v3[1]) == 345);
      A(p1 (v3[2]) == 111);
      A(p1 (v3[3]) == 123);
      A(p1 (v3[4]) == 333);
      A(p1 (v3[5]) == 222);
      A(v3.Search(p2 ["six"]) == 3);
      A(v3.Search(p2 ["aha"]) == 0);
      A(v3.Search(p2 ["gee"]) == 2);
      A(v3.Search(p2 ["wow"]) == 6);
    c4_View v4 = v1.SortOnReverse(p2, p2);
      A(v4.GetSize() == 6);
      A(p1 (v4[0]) == 222);
      A(p1 (v4[1]) == 333);
      A(p1 (v4[2]) == 123);
      A(p1 (v4[3]) == 111);
      A(p1 (v4[4]) == 345);
      A(p1 (v4[5]) == 234);
      A(v4.Search(p2 ["six"]) == 2);
      A(v4.Search(p2 ["aha"]) == 6);
      A(v4.Search(p2 ["gee"]) == 4);
      A(v4.Search(p2 ["wow"]) == 0);
  } E;

  B(b21, Memo property, 0)
  {
    c4_Row r1;
    c4_MemoProp p1 ("p1");
    c4_Bytes x1 ("hi!", 3);

    p1 (r1) = x1;
      c4_Bytes x2 = p1 (r1);
      A(x1 == x2);
  } E;

  B(b22, Stored view references, 0)
  {
    c4_ViewProp p1 ("p1");
    c4_View v1;

    {
      v1.Add(p1 [c4_View ()]);
    }

        // this works
    int n = ViewSize(p1.Get(v1[0]));
      A(n == 0);

      // this fails in 1.8b2 using MSVC 1.52 (tq_wvus)
      //
      // The compiler destructs temp c4_View once too often, or
      // what's more likely, fails to call the constructor once.
      //
      // So for MSVC 1.52: use prop.Get(rowref) for subviews,
      // or immediately assign the result to a c4_View object,
      // do not pass a "prop (rowref)" expression as argument.
      
#if _MSC_VER != 800     
    int m = ViewSize(p1 (v1[0]));
      A(m == 0);
#endif
  } E;

  B(b23, Sort comparison fix, 0) // 1.9e: compare buffering problem
  {
    c4_DoubleProp p1 ("p1");
    c4_View v1;
    for (int i = 0; i < 100; ++i)
      v1.Add(p1 [99-i]);
    c4_View v2 = v1.Sort();
      A(v2.GetSize() == 100);
    for (int j = 0; j < 100; ++j)
    {
      A(p1 (v1[j]) == (double) 99-j);
      A(p1 (v2[j]) == (double) j);
    }
  } E;

  B(b24, Custom view comparisons, 0) // 1.9f: more compare cache problems
  {
    c4_IntProp    p1 ("p1");
    c4_FloatProp  p2 ("p2");
    c4_DoubleProp p3 ("p3");
    c4_IntProp    p4 ("p4");
    c4_View v1;
    v1.Add(p1 [2] + p2 [2] + p3 [2]);
    v1.Add(p1 [1] + p2 [1] + p3 [1]);
    v1.Add(p1 [3] + p2 [3] + p3 [3]);
      A(v1.GetSize() == 3);
      A((int)    p1 (v1[0]) > (int)  p1 (v1[1]));
      A((float)  p2 (v1[0]) > (float)  p2 (v1[1]));
      A((double) p3 (v1[0]) > (double) p3 (v1[1]));
      A((int)    p1 (v1[0]) < (int)  p1 (v1[2]));
      A((float)  p2 (v1[0]) < (float)  p2 (v1[2]));
      A((double) p3 (v1[0]) < (double) p3 (v1[2]));
    c4_View v2 = v1.Unique();
      A(v2.GetSize() == 3);
      A((int)    p1 (v2[0]) != (int)    p1 (v2[1]));
      A((float)  p2 (v2[0]) != (float)  p2 (v2[1]));
      A((double) p3 (v2[0]) != (double) p3 (v2[1]));
      A((int)    p1 (v2[0]) != (int)    p1 (v2[2]));
      A((float)  p2 (v2[0]) != (float)  p2 (v2[2]));
      A((double) p3 (v2[0]) != (double) p3 (v2[2]));
    v1.Add(p1 [2] + p2 [2] + p3 [2]);
    v1.Add(p1 [1] + p2 [1] + p3 [1]);
    v1.Add(p1 [3] + p2 [3] + p3 [3]);
    c4_View v3 = v1.Unique();
      A(v3.GetSize() == 3);
      A((int)    p1 (v3[0]) != (int)    p1 (v3[1]));
      A((float)  p2 (v3[0]) != (float)  p2 (v3[1]));
      A((double) p3 (v3[0]) != (double) p3 (v3[1]));
      A((int)    p1 (v3[0]) != (int)    p1 (v3[2]));
      A((float)  p2 (v3[0]) != (float)  p2 (v3[2]));
      A((double) p3 (v3[0]) != (double) p3 (v3[2]));
    c4_View v4 = v1.Counts(p1, p4);
      A(v4.GetSize() == 3);
    c4_View v5 = v1.Counts(p2, p4);
      A(v5.GetSize() == 3);     // this failed in 1.9f
    c4_View v6 = v1.Counts(p3, p4);
      A(v6.GetSize() == 3);     // this failed in 1.9f
  } E;

  B(b25, Copy row from derived, 0)
  {
    c4_IntProp p1 ("p1");
    c4_View v1;
    v1.Add(p1 [111]);
    v1.Add(p1 [222]);
    v1.Add(p1 [333]);
    c4_View v2 = v1.Select(p1 [222]);
      A(v2.GetSize() == 1);
      A(p1 (v2[0]) == 222);
    c4_Row r = v2[0];
      A(p1 (r) == 222); // 1.9g: failed because SetAt did not remap
  } E;

  B(b26, Partial memo field access, 0)
  {
  c4_BytesProp p1 ("p1");
  c4_View v1;
  v1.Add(p1 [c4_Bytes ("12345", 5)]);
    A(v1.GetSize() == 1);
  c4_Bytes buf = p1 (v1[0]);
    A(buf.Size() == 5);
    A(buf == c4_Bytes ("12345", 5));
  buf = p1(v1[0]).Access(1,3);
    A(buf == c4_Bytes ("234", 3));
  p1 (v1[0]).Modify(c4_Bytes ("ab", 2), 2, 0);
  buf = p1 (v1[0]);
    A(buf == c4_Bytes ("12ab5", 5));
  p1 (v1[0]).Modify(c4_Bytes ("ABC", 3), 1, 2);
  buf = p1 (v1[0]);
    A(buf == c4_Bytes ("1ABCab5", 7));
  p1 (v1[0]).Modify(c4_Bytes ("xyz", 3), 2, -2);
  buf = p1 (v1[0]);
    A(buf == c4_Bytes ("1Axyz", 5));
  p1 (v1[0]).Modify(c4_Bytes ("3456", 4), 4, 0);
  buf = p1 (v1[0]);
    A(buf == c4_Bytes ("1Axy3456", 8));
  } E;

  B(b27, Copy value to another row, 0)
  {
    c4_StringProp p1 ("p1");
    c4_View v1;
    v1.SetSize(2);
    p1 (v1[1]) = "abc";
      A((const char*) (p1 (v1[0])) == (c4_String) "");
      A((const char*) (p1 (v1[1])) == (c4_String) "abc");

    // fails in 2.4.0, reported by Jerry McRae, August 2001
    p1 (v1[0]) = p1 (v1[1]);
      A((const char*) (p1 (v1[0])) == (c4_String) "abc");
  } E;
}
