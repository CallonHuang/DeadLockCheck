#include "deadLockExternal.h"
#include <stdio.h>
#include <string.h>

//#define CONFIG_DOUBLE_DEAD_LOCK 1

pthread_mutex_t mutex_value1;
pthread_mutex_t mutex_value2;
pthread_mutex_t mutex_value3;
pthread_mutex_t mutex_value4;

void *thread_func1(void *param)
{
    while(1) {
        pthread_mutex_lock(&mutex_value1);  
        pthread_mutex_lock(&mutex_value2);
        pthread_mutex_unlock(&mutex_value2);
        pthread_mutex_unlock(&mutex_value1);
        sleep(1);
    }
}

// the second thread function
void *thread_func2(void *param)
{
    while(1) {
        pthread_mutex_lock(&mutex_value2);
        sleep(1);
        pthread_mutex_lock(&mutex_value3);
        pthread_mutex_unlock(&mutex_value3);
        pthread_mutex_unlock(&mutex_value2);
        sleep(1);
    }
}

// the third thread function
void *thread_func3(void *param)
{
    while(1) {
        pthread_mutex_lock(&mutex_value3);
        sleep(1);
        pthread_mutex_lock(&mutex_value1);
        pthread_mutex_unlock(&mutex_value1);
        pthread_mutex_unlock(&mutex_value3);
        sleep(1);
    }
}

// the fouth thread function
void *thread_func4(void *param)
{
    struct timespec tout;
    memset (&tout, 0, sizeof(tout));
    tout.tv_sec += 10;
    while(1) {
        #ifdef CONFIG_DOUBLE_DEAD_LOCK
        if (0 == pthread_mutex_trylock(&mutex_value4)) {
            sleep(1);
            if (0 == pthread_mutex_timedlock(&mutex_value1, &tout)) {
                pthread_mutex_unlock(&mutex_value1);
            }
            pthread_mutex_lock(&mutex_value4);
            pthread_mutex_unlock(&mutex_value4);
        }   
        #else
        pthread_mutex_lock(&mutex_value3);
        #endif
        sleep(1);
    }
}

int main()
{
    pthread_t t1, t2, t3, t4;
    char input = 0;
    pthread_mutex_init(&mutex_value1,NULL);
    pthread_mutex_init(&mutex_value2,NULL);
    pthread_mutex_init(&mutex_value3,NULL);
    pthread_mutex_init(&mutex_value4,NULL);
    
    
    pthread_create(&t1, NULL, thread_func1, NULL);
    pthread_create(&t2, NULL, thread_func2, NULL);
    pthread_create(&t3, NULL, thread_func3, NULL);
    #ifndef CONFIG_DOUBLE_DEAD_LOCK
    sleep(5);
    #endif
    pthread_create(&t4, NULL, thread_func4, NULL);
    
    
    while(1) {
        scanf("%c", &input);
        if ('p' == input) {
            PrtRecord();
        }
    }
    return 0;
}