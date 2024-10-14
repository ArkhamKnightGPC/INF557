/*
 * mutex.c
 *
 *  Created on: Mar 19, 2016
 *      Author: jiaziyi
 */
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#define NTHREADS 50
void *increase_counter();

int  counter = 0;
pthread_mutex_t lock;

int main()
{

    pthread_t mythreads[NTHREADS];

    if(pthread_mutex_init(&lock, NULL)){
        fprintf(stderr, "MUTEX INIT FAILED\n");
        return 1;
    }

	//create 50 threads of increase_counter, each thread adding 1 to the counter
    for(int i=0; i < NTHREADS; i++){
        if(pthread_create(mythreads + i, NULL, increase_counter, NULL)){
            fprintf(stderr, "ERROR CREATING THREAD A\n");
            return 1;
        }
    }

    usleep (30000);

	printf("\nFinal counter value: %d\n", counter);
}

void *increase_counter()
{
    
    pthread_mutex_lock(&lock);
    printf("Thread number %ld, working on counter. The current value is %d\n", (long)pthread_self(), counter);
	int i = counter;
	counter = i+1;
    pthread_mutex_unlock(&lock);

    return NULL;

}
