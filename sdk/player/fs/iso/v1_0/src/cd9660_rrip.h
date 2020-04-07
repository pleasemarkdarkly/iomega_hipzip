/* Stolen from OpenBSD sources */
#ifndef CD9660_RRIP_H
#define CD9660_RRIP_H

#include "cd9660_iso.h"
#include <cyg/kernel/ktypes.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
	char   type			[ISODCL (  0,    1)];
	cyg_uint8 length		[ISODCL (  2,    2)]; /* 711 */
	cyg_uint8 version		[ISODCL (  3,    3)];
} ISO_SUSP_HEADER;

typedef struct {
	ISO_SUSP_HEADER			h;
	char mode			[ISODCL (  4,   11)]; /* 733 */
	char links			[ISODCL ( 12,   19)]; /* 733 */
	char uid			[ISODCL ( 20,   27)]; /* 733 */
	char gid			[ISODCL ( 28,   35)]; /* 733 */
} ISO_RRIP_ATTR;

typedef struct {
	ISO_SUSP_HEADER			h;
	char dev_t_high			[ISODCL (  4,   11)]; /* 733 */
	char dev_t_low			[ISODCL ( 12,   19)]; /* 733 */
} ISO_RRIP_DEVICE;

#define	ISO_SUSP_CFLAG_CONTINUE	0x01
#define	ISO_SUSP_CFLAG_CURRENT	0x02
#define	ISO_SUSP_CFLAG_PARENT	0x04
#define	ISO_SUSP_CFLAG_ROOT	0x08
#define	ISO_SUSP_CFLAG_VOLROOT	0x10
#define	ISO_SUSP_CFLAG_HOST	0x20

typedef struct {
	cyg_uint8 cflag			[ISODCL (  1,    1)];
	cyg_uint8 clen			[ISODCL (  2,    2)];
	cyg_uint8 name			[1];			/* XXX */
} ISO_RRIP_SLINK_COMPONENT;
#define	ISO_RRIP_SLSIZ	2

typedef struct {
	ISO_SUSP_HEADER			h;
	cyg_uint8 flags			[ISODCL (  4,    4)];
	cyg_uint8 component		[ISODCL (  5,    5)];
} ISO_RRIP_SLINK;

typedef struct {
	ISO_SUSP_HEADER			h;
	char flags			[ISODCL (  4,    4)];
} ISO_RRIP_ALTNAME;

typedef struct {
	ISO_SUSP_HEADER			h;
	char dir_loc			[ISODCL (  4,    11)]; /* 733 */
} ISO_RRIP_CLINK;

typedef struct {
	ISO_SUSP_HEADER			h;
	char dir_loc			[ISODCL (  4,    11)]; /* 733 */
} ISO_RRIP_PLINK;

typedef struct {
	ISO_SUSP_HEADER			h;
} ISO_RRIP_RELDIR;

#define	ISO_SUSP_TSTAMP_FORM17	0x80
#define	ISO_SUSP_TSTAMP_FORM7	0x00
#define	ISO_SUSP_TSTAMP_CREAT	0x01
#define	ISO_SUSP_TSTAMP_MODIFY	0x02
#define	ISO_SUSP_TSTAMP_ACCESS	0x04
#define	ISO_SUSP_TSTAMP_ATTR	0x08
#define	ISO_SUSP_TSTAMP_BACKUP	0x10
#define	ISO_SUSP_TSTAMP_EXPIRE	0x20
#define	ISO_SUSP_TSTAMP_EFFECT	0x40

typedef struct {
	ISO_SUSP_HEADER			h;
	cyg_uint8 flags			[ISODCL (  4,    4)];
	cyg_uint8 time			[ISODCL (  5,    5)];
} ISO_RRIP_TSTAMP;

typedef struct {
	ISO_SUSP_HEADER			h;
	cyg_uint8 flags			[ISODCL (  4,    4)];
} ISO_RRIP_IDFLAG;

typedef struct {
	ISO_SUSP_HEADER			h;
	char len_id			[ISODCL (  4,    4)];
	char len_des			[ISODCL (  5,	 5)];
	char len_src			[ISODCL (  6,	 6)];
	char version			[ISODCL (  7,	 7)];
} ISO_RRIP_EXTREF;

typedef struct {
	ISO_SUSP_HEADER			h;
	char check			[ISODCL (  4,	 5)];
	char skip			[ISODCL (  6,	 6)];
} ISO_RRIP_OFFSET;

typedef struct {
	ISO_SUSP_HEADER			h;
	char location			[ISODCL (  4,	11)];
	char offset			[ISODCL ( 12,	19)];
	char length			[ISODCL ( 20,	27)];
} ISO_RRIP_CONT;

/*
 *	Analyze function flag (similar to RR field bits)
 */
#define	ISO_SUSP_ATTR		0x0001
#define	ISO_SUSP_DEVICE		0x0002
#define	ISO_SUSP_SLINK		0x0004
#define	ISO_SUSP_ALTNAME	0x0008
#define	ISO_SUSP_CLINK		0x0010
#define	ISO_SUSP_PLINK		0x0020
#define	ISO_SUSP_RELDIR		0x0040
#define	ISO_SUSP_TSTAMP		0x0080
#define	ISO_SUSP_IDFLAG		0x0100
#define	ISO_SUSP_EXTREF		0x0200
#define	ISO_SUSP_CONT		0x0400
#define	ISO_SUSP_OFFSET		0x0800
#define	ISO_SUSP_STOP		0x1000
#define	ISO_SUSP_UNKNOWN	0x8000

typedef struct {
	struct iso_node	*inop;
	int		fields;		/* interesting fields in this analysis */
	cyg_int32	iso_ce_blk;	/* block of continuation area */
	off_t		iso_ce_off;	/* offset of continuation area */
	int		iso_ce_len;	/* length of continuation area */
	struct iso_mnt	*imp;		/* mount structure */
	ino_t		*inump;		/* inode number pointer */
	char		*outbuf;	/* name/symbolic link output area */
	cyg_uint16	*outlen;	/* length of above */
	cyg_uint16	maxlen;		/* maximum length of above */
	int		cont;		/* continuation of above */
} ISO_RRIP_ANALYZE;

int cd9660_rrip_analyze(struct iso_directory_record *isodir,
			struct iso_node *inop, struct iso_mnt *imp);
int cd9660_rrip_getname(struct iso_directory_record *isodir,
			char *outbuf, cyg_uint16 *outlen,
			ino_t *inump, struct iso_mnt *imp);
int cd9660_rrip_getsymname(struct iso_directory_record *isodir,
			   char *outbuf, cyg_uint16 *outlen,
			   struct iso_mnt *imp);
int cd9660_rrip_offset(struct iso_directory_record *isodir,
		       struct iso_mnt *imp);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* CD9660_RRIP_H */
