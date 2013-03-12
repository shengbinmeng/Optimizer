//Test program for simple constant propagation
//example taken from the lecture

#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", (long)x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long

void main()
{
	long s, a, i, k, b, n;
	s = 0;  
	a = 4;
	i = 2;
	k = 3;
	b = 4;
	n = 5;

	if(k == 0){ 			// shoule propagate here (k=3)
		b = 1;
	} else { 
		b = 2;
	}
	
	while(i < n){			// should propagate here (n=5, but not i)
		s = s + a * b;		// should propagate here (a=4)
		i = i + 1;
	}

	WriteLong(s);
}


/* Expected output:
24
*/

