// tmapped.cpp -- Regression test program, mapped view tests
// $Id: tmapped.cpp,v 1.4 2001/11/25 21:10:39 wcvs Exp $
// This is part of MetaKit, see http://www.equi4.com/metakit/

#include "regress.h"

void TestMapped()
{
  B(m01, Hash mapping, 0);
  {
    c4_StringProp p1 ("p1");

  c4_Storage s1;
  c4_View v1 = s1.GetAs("v1[p1:S]");
  c4_View v2 = s1.GetAs("v2[_H:I,_R:I]");
  c4_View v3 = v1.Hash(v2, 1);

  v3.Add(p1 ["b93655249726e5ef4c68e45033c2e0850570e1e07"]);
  v3.Add(p1 ["2ab03fba463d214f854a71ab5c951cea096887adf"]);
  v3.Add(p1 ["2e196eecb91b02c16c23360d8e1b205f0b3e3fa3d"]);
    A(v3.GetSize() == 3);

    // infinite loop in 2.4.0, reported by Nathan Rogers, July 2001
    // happens when looking for missing key after a hash collision
  int f = v3.Find(p1 ["7c0734c9187133f34588517fb5b39294076f22ba3"]);
    A(f == -1);
  } E;
}
