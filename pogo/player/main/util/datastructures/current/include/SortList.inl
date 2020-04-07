#include <util/debug/debug.h>          // debugging hooks
DEBUG_USE_MODULE( DBG_SORTLIST );

// note that template member functions won't end up actually being inline per se, but the file itself is included "inline".
// use merge sort to sort the playlist entries according to the comparison function lt.
template <class DataType>
void SortList<DataType>::MergeSort(SortListLessThanFunction lt)
{
    DEBUGP( DBG_SORTLIST, DBGLEV_TRACE, "sl:MergeSort\n");
    DataNode<DataType> *p, *q, *e;
    int insize, nmerges, psize, qsize, i;

    insize = 1;

    while (1)
    {
        p = m_pHead;
        m_pHead = NULL;
        m_pTail = NULL;
        
        nmerges = 0;  /* count number of merges we do in this pass */
        while (p)
        {
            nmerges++;   /* there exists a merge to be done */
            /* step `insize' places along from p */
            q = p;
            psize = 0;
            for (i = 0; i < insize; i++) 
            {
                psize++;
                q = q->pNext;
                if (!q) break;
            }
            /* if q hasn't fallen off end, we have two lists to merge */
            qsize = insize;
            /* now we have two lists; merge them */
            while (psize > 0 || (qsize > 0 && q)) 
            {

                /* decide whether next element of merge comes from p or q */
                if (psize == 0) {
                    /* p is empty; e must come from q. */
                    e = q; q = q->pNext; qsize--;
                } else if (qsize == 0 || !q) {
                    /* q is empty; e must come from p. */
                    e = p; p = p->pNext; psize--;
                } else if (lt(p->dataType,q->dataType)) {
                /* First element of p is lower (or same);
                    * e must come from p. */
                    e = p; p = p->pNext; psize--;
                } else {
                    /* First element of q is lower; e must come from q. */
                    e = q; q = q->pNext; qsize--;
                }
                
                /* add the next element to the merged list */
                if (m_pTail) {
                    m_pTail->pNext = e;
                } else {
                    m_pHead = e;
                }
                e->pPrev = m_pTail;
                m_pTail = e;
            }
            p = q;
        }
        m_pTail->pNext = NULL;
        
        /* If we have done only one merge, we're finished. */
        if (nmerges <= 1)   /* allow for nmerges==0, the empty list case */
            return;
        
        /* Otherwise repeat, merging lists twice the size */
        insize *= 2;
    }
}
