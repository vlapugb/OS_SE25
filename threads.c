#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int const COUNT = 100;
int shared_value = 0;
pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;
int ready = 0;

void *consumer(void *arg) {
	for (int i=0; i<COUNT; i++) {
		int printable = 0;
		while (1) {
                        pthread_mutex_lock(&fastmutex);
                        if (ready == 1) {
				printable = shared_value;
                                ready = 0;
                                pthread_mutex_unlock(&fastmutex);
                                break;
                        }
                        pthread_mutex_unlock(&fastmutex);
                }	
		usleep(100000);
		printf("Consumer has consumed: %d\n", printable);
	}
}



int main() {
	pthread_t t;
	int thread_cr = pthread_create(&t, 0, &consumer, 0);
	if (thread_cr != 0) {
		perror("Error creating thread");
		return -1;
	}
	for (int i=0; i<COUNT; i++) {
		while (1) {
			pthread_mutex_lock(&fastmutex);	
			if (ready == 0) {
				shared_value = i;
				ready = 1;
				pthread_mutex_unlock(&fastmutex);
				break;		
			}
			pthread_mutex_unlock(&fastmutex);
		}

		printf("Producer has produced: %d\n", i);
	}
	return 0;
}

