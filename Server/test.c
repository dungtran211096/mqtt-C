#include <stdio.h>

int *getArr( int a[]){
	a[0] = 1;
	a[1] =2;
	return a;
}
int main(int argc, char const *argv[])
{
	int a[]
	int *a = getArr();
	printf("%d\n", *(a + 0) );
	printf("%d\n", *(a+ 1));
	return 0;
}