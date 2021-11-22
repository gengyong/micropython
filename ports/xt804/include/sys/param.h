
#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

/* Bit map related macros.  */
#define	setbit(a,i)	((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a,i)	((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a,i)	((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a,i)	(((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)

#define howmany(x, y)	(((x) + ((y) - 1)) / (y))
#define roundup(x, y)	((((x) + ((y) - 1)) / (y)) * (y))
#define powerof2(x)	((((x) - 1) & (x)) == 0)

/* Macros for min/max.  */
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))


#endif

