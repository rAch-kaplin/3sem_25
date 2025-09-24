#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_LOOPS 1000000

long long int sum = 0;

void *f(void *args) {
	int i = 0;
	int offset = *(int*) args;
	
	for (i = 0; i < NUM_LOOPS; i++) {
		sum += offset;
	}
	pthread_exit(NULL);
}

int main() {
	pthread_t p1, p2;
	int offset1 = 1;
	int offset2 = -1;
	pthread_create(&p1, NULL, f, &offset1);
	pthread_create(&p2, NULL, f, &offset2);

	pthread_join(p1, NULL);
	pthread_join(p2, NULL);


	printf("Result: %lld\n", sum);
	return 0;
}
