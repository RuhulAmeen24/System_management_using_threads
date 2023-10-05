# include <pthread.h>
# include <semaphore.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <time.h>

struct args {
    int* t;
    int* w;
    int* p;
    int student_number;
};

struct args glob_arg;

pthread_mutex_t mutex_arg;

sem_t arg_empty;
sem_t arg_full;
sem_t washing_machines;

time_t start_time;
int left_without_washing = 0;
time_t wasted_time = 0;

void* student(void* arguments) {
    sem_wait(&arg_full);
    pthread_mutex_lock(&mutex_arg);

    int student_number = glob_arg.student_number-1;
    int t = glob_arg.t[student_number];
    int w = glob_arg.w[student_number];
    int p = glob_arg.p[student_number];
    student_number++;
    pthread_mutex_unlock(&mutex_arg);
    sem_post(&arg_empty);
    sleep(t);
   
    nanosleep((const struct timespec[]){{0, student_number*1000}}, NULL);


    time_t arriving_time = time(NULL);

    printf("\033[1;37m");
    printf("%ld: Student %d arrives\n", arriving_time-start_time, student_number);
    printf("\033[0m");

    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += p;
    timeout.tv_nsec+=1000000;

    if (sem_timedwait(&washing_machines, &timeout) == -1) {

        printf("\033[1;31m");
        printf("%ld: Student %d leaves without washing\n", time(NULL)-start_time, student_number);
        printf("\033[0m");

        left_without_washing++;
        wasted_time += p;
        return NULL;
    }
    time_t washing_allot_time = time(NULL);
    wasted_time += washing_allot_time - arriving_time;

    printf("\033[1;32m");
    printf("%ld: Student %d starts washing\n", washing_allot_time-start_time, student_number);
    printf("\033[0m");

    sleep(w);
    time_t washing_finish= time(NULL);

    printf("\033[1;33m");
    printf("%ld: Student %d finishes washing clothes\n", washing_finish-start_time, student_number);
    printf("\033[0m");
    
    sem_post(&washing_machines);
    return NULL;

}

void* arg_create_func(void * arguments)
{
    sem_wait(&arg_empty);
    pthread_mutex_lock(&mutex_arg);

    int student_number=glob_arg.student_number;
    int t=glob_arg.t[student_number];
    int w=glob_arg.w[student_number];
    int p=glob_arg.p[student_number];

    glob_arg.student_number++;
    pthread_mutex_unlock(&mutex_arg);
    sem_post(&arg_full);
    return NULL;
}


int main()
{
    glob_arg.student_number = 0;
    
    int n,m;
    scanf("%d %d",&n,&m);

    pthread_mutex_init(&mutex_arg, NULL);
    sem_init(&arg_empty, 0, 1);
    sem_init(&arg_full, 0, 0);
    sem_init(&washing_machines, 0, m);

    int t[n],w[n],p[n];
    for (int i = 0; i < n; i++)
    {
        scanf("%d %d %d",&t[i],&w[i],&p[i]);
    }
    
    glob_arg.t = t;
    glob_arg.w = w;
    glob_arg.p = p;

    pthread_t students[n];
    pthread_t arg_create[n];

    start_time = time(NULL);
    for (int i = 0; i < n; i++)
    {
        pthread_create(&arg_create[i], NULL, arg_create_func, NULL);

        pthread_create(&students[i],NULL,student,NULL);
        // sleep(3);
    }
    
    for (int i = 0; i < n; i++)
    {
        pthread_join(students[i],NULL);
    }

    pthread_mutex_destroy(&mutex_arg);
    sem_destroy(&arg_empty);
    sem_destroy(&arg_full);
    sem_destroy(&washing_machines);
    
    printf("Left without washing: %d\n", left_without_washing);
    printf("Wasted time: %ld\n", wasted_time);
    if(left_without_washing<=n/4)
    {   
        printf("No\n");
    }
    else
    {
        printf("Yes\n");
    }

    return 0;
}