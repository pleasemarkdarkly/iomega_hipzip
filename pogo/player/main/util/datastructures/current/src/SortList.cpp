#include <main/util/datastructures/SortList.h>

DEBUG_MODULE_S( DBG_SORTLIST, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_SORTLIST );

bool StringLessThan(void* s1, void* s2)
{
    return (strcmp((char*)s1,(char*)s2) < 0);
}

typedef SortList<char*> StringList;
typedef SimpleListIterator<char*> StringListIterator;

void PrintStringList(StringList lst)
{
    int i = 0;
    for (StringListIterator str = lst.GetHead(); str != lst.GetEnd(); ++str)
    {
        DEBUGP(DBG_SORTLIST, DBGLEV_INFO, "%02d:%s\n",i,(*str));
        ++i;
    }
}

void TestSortList () {
    SortList<char*> sl;
    sl.Clear();
    char* p1 = "as;dfjdlkj";
    char* p2 = "uipqemnm,th";
    char* p3 = "LKJSDKJSD";
    char* p4 = "OIUOPUPIU";
    char* p5 = "<M>N<MNMN<MN<M";
    char* p6 = "EWREWREWREWRE";
    char* p7 = "xvcxvcxvcxcvx";
    char* p8 = "kjlkjlkjlkjkl";
    char* p9 = "08098098098098";
    char* p10 = "GSFDSREWRE";
    char* p11 = "OPIUOPIUPOIUO";
    char* p12 = "hhjhkhkjhkjh";
    sl.PushBack(p1);
    sl.PushBack(p2);
    sl.PushBack(p3);
    sl.PushBack(p4);
    sl.PushBack(p5);
    sl.PushBack(p6);
    sl.PushBack(p7);
    sl.PushBack(p8);
    sl.PushBack(p9);
    sl.PushBack(p10);
    sl.PushBack(p11);
    sl.PushBack(p12);
    PrintStringList(sl);
    sl.Sort(StringLessThan);
    PrintStringList(sl);
}