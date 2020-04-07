#include <stdio.h>

#include "huffman.h"
#include "types.h"
#include "memory.h"


void init_hbits(void)
{
        int i, j, k;
	int milen;
        const int nbits[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
        for (i = 0; i < 32; i++) {
		const unsigned char *len;
		/*const*/ unsigned long *table;
//diag_printf("%d", i);
		switch (i) {
			case 1: table = t1HB; len = t1l; break;
			case 2: table = t2HB; len = t2l; break;
			case 3: table = t3HB; len = t3l; break;
			case 5: table = t5HB; len = t5l; break;
			case 6: table = t6HB; len = t6l; break;
			case 7: table = t7HB; len = t7l; break;
			case 8: table = t8HB; len = t8l; break;
			case 9: table = t9HB; len = t9l; break;
			case 10: table = t10HB; len = t10l; break;
			case 11: table = t11HB; len = t11l; break;
			case 12: table = t12HB; len = t12l; break;
			case 13: table = t13HB; len = t13l; break;
			case 15: table = t15HB; len = t15l; break;
			case 16: table = t16HB; len = t16l; break;
			case 17: table = t16HB; len = t16l; break;
			case 18: table = t16HB; len = t16l; break;
			case 19: table = t16HB; len = t16l; break;
			case 20: table = t16HB; len = t16l; break;
			case 21: table = t16HB; len = t16l; break;
			case 22: table = t16HB; len = t16l; break;
			case 23: table = t16HB; len = t16l; break;
			case 24: table = t24HB; len = t24l; break;
			case 25: table = t24HB; len = t24l; break;
			case 26: table = t24HB; len = t24l; break;
			case 27: table = t24HB; len = t24l; break;
			case 28: table = t24HB; len = t24l; break;
			case 29: table = t24HB; len = t24l; break;
			case 30: table = t24HB; len = t24l; break;
			case 31: table = t24HB; len = t24l; break;
			case 32: table = t32HB; len = t32l; break;
			case 33: table = t33HB; len = t33l; break;
//			default: table = 0;  // this will trap errors ;-)
				// ...but the dharma just dies.  oopsie.
				// fixed now
			default: /*diag_printf("!\n");*/ continue;  // this will trap errors ;-)
		};
//diag_printf(".\n");

		milen = ht[i].len;
//diag_printf("\n\n&&&&& %d => %d &&&&&\n\n\n", i, milen);
		for (j = 0; j < milen; j++)
                        for (k = 0; k < milen; k++) {

//diag_printf("%d, %d => 0x%08x, 0x%08x\n", j, k, &table[j*milen+k], &len[j*milen+k]);
if (k == 100) { barf: goto barf; }
table[j*milen+k] |= len[j*milen+k] << 24;
                                FASTVAR(hbits)[i][j][k] = len[j*milen+k] + (j ? 1 : 0) + (k ? 1 : 0);
			}
//fprintf(stderr, "tab %2d: 0x%04x, 0x%04x, %d\n", i, mask, maskl, maxl);
                for (j = 0; j < milen; j++)
                        FASTVAR(hbits)[i][j][milen-1] += ht[i].linbits;
                for (k = 0; k < milen; k++)
                        FASTVAR(hbits)[i][milen-1][k] += ht[i].linbits;
        }

        for (k = 0; k < 16; k++) {
                FASTVAR(hbits)[32][0][k] = t32l[k] + nbits[k];
                FASTVAR(hbits)[33][0][k] = t33l[k] + nbits[k];
        }
}

