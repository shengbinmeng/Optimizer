//Test program for dead statement elimination
//example taken from the lecture

#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", (long)x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long

void main()
{
	long x, y, z, a, b, c, t; 
	x = 0;
	y = 1;
	z = 2;
	a = 3;
	b = 4;
	c = 5;
	t = 6;
	a = x + y;  //dead code, should be eliminated
	t = a;
	c = a + x;  //looks like, but NOT dead code, should not be eliminated
	if(x == 0) {
		b = t + z;
	} else {
		c = y + 1;	
	}
	
	WriteLong(c);
}


/* Expected output:
1
*/

