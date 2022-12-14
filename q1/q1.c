#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>

// declaring semaphore and initial time
sem_t* washingMachine;
int tme;
int ind;
int timeWasted = 0;
int counter = 0;

// declaring mutex and cond for conditional variable
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct Student
{
    int id;
    int arrivalTime;
    int washingTime;
    int patienceTime;
    int washingFlag;
} Student;

Student arrStudents[1000];
int n, m;

// using bubble sort to sort array of structs 
// on the basis of their arrival time
void sort(int n, Student arrStudents[n])
{
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            if (arrStudents[j].arrivalTime > arrStudents[j + 1].arrivalTime)
            {
                Student temp;
                temp.id = arrStudents[j].id;
                temp.arrivalTime = arrStudents[j].arrivalTime;
                temp.washingTime = arrStudents[j].washingTime;
                temp.patienceTime = arrStudents[j].patienceTime;
                arrStudents[j].id = arrStudents[j + 1].id;
                arrStudents[j].arrivalTime = arrStudents[j + 1].arrivalTime;
                arrStudents[j].washingTime = arrStudents[j + 1].washingTime;
                arrStudents[j].patienceTime = arrStudents[j + 1].patienceTime;
                arrStudents[j + 1].id = temp.id;
                arrStudents[j + 1].arrivalTime = temp.arrivalTime;
                arrStudents[j + 1].washingTime = temp.washingTime;
                arrStudents[j + 1].patienceTime = temp.patienceTime;
            }
        }
    }
}

// thread function to be allocated for each thread
void *threadFunc(void *args)
{
    int i = ind;
    // sleep till time of arrival comes
    sleep(arrStudents[i].arrivalTime);
    int tme2 = time(NULL);
    tme2 = tme2 - tme;
    int returnVal;
    printf("\033[0;37m%d: Student %d arrives\033[0;37m\n", tme2, arrStudents[i].id);
    // if semaphore is available, it goes inside the if condition
    if (sem_trywait(washingMachine) == 0)
    {
        timeWasted += tme2 - arrStudents[i].arrivalTime;
        printf("\033[0;32m%d: Student %d starts washing\033[0;37m\n", tme2, arrStudents[i].id);
        sleep(arrStudents[i].washingTime);
        tme2 = time(NULL);
        tme2 = tme2 - tme;
        printf("\033[0;33m%d: Student %d leaves after washing\033[0;37m\n", tme2, arrStudents[i].id);
        arrStudents[i].washingFlag = 1;
        // release the semaphore i.e. increment it's value
        sem_post(washingMachine);
        // send signal, that machine is available
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        struct timespec waitTime;
        struct timeval now;
        gettimeofday(&now, NULL);
        waitTime.tv_sec = now.tv_sec + arrStudents[i].patienceTime;
        waitTime.tv_nsec = (1.5)*(now.tv_usec * 1000); // could use 7 zeros
        
        // initializing patience time of student
        pthread_mutex_lock(&mutex);
        returnVal = pthread_cond_timedwait(&cond, &mutex, &waitTime);
        pthread_mutex_unlock(&mutex);
    }

    // if returnVal == 0 then machine has become 
    // empty during the patience time period
    if(returnVal == 0)
    {
        if (arrStudents[i].washingFlag == 0)
        {
            // allocate machine to this new thread
            if (sem_trywait(washingMachine) == 0)
            {
                tme2 = time(NULL);
                tme2 = tme2 - tme;
                timeWasted += tme2 - arrStudents[i].arrivalTime;
                printf("\033[0;32m%d: Student %d starts washing\033[0;37m\n", tme2, arrStudents[i].id);
                sleep(arrStudents[i].washingTime);
                tme2 = time(NULL);
                tme2 = tme2 - tme;
                printf("\033[0;33m%d: Student %d leaves after washing\033[0;37m\n", tme2, arrStudents[i].id);
                arrStudents[i].washingFlag = 1;
                sem_post(washingMachine);
                pthread_mutex_lock(&mutex);
                pthread_cond_signal(&cond);
                pthread_mutex_unlock(&mutex);
            }
        }
    }
    // if value other than 0 is returned
    // then patience time has exhausted
    else{
        int tm2 = time(NULL);
        tm2 = tm2 - tme;
        counter++;
        timeWasted += arrStudents[i].patienceTime;
        printf("\033[0;31m%d: Student %d leaves without washing\033[0;37m\n", tm2, arrStudents[i].id);
        arrStudents[i].washingFlag = 1;
    }
}

int main()
{
    // scanning of input
    scanf("%d %d", &n, &m);
    int arrivalTime[n];
    int washingTime[n];
    int patienceTime[n];

    for (int i = 0; i < n; i++)
    {
        scanf("%d %d %d", &arrivalTime[i], &washingTime[i], &patienceTime[i]);
    }
    tme = time(NULL);

    // initializing array of structs
    for (int i = 0; i < n; i++)
    {
        arrStudents[i].id = i + 1;
        arrStudents[i].arrivalTime = arrivalTime[i];
        arrStudents[i].washingTime = washingTime[i];
        arrStudents[i].patienceTime = patienceTime[i];
        arrStudents[i].washingFlag = 0;
    }

    // sorting
    sort(n, arrStudents);

    if ((washingMachine = sem_open("/semaphore", O_CREAT, 0777, m)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // initializing semaphore with value m
    sem_init(washingMachine, 0, m);

    // creating n threads for each student
    pthread_t students[n];
    for (int i = 0; i < n; i++)
    {
        ind = i;
        pthread_create(&students[i], NULL, threadFunc, NULL);
        usleep(1);
    }

    // joining threads in the end
    for (int i = 0; i < n; i++)
    {
        pthread_join(students[i], NULL);
    }

    if (sem_unlink("/semaphore") == -1)
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }

    // priting number of students who couldn't 
    // wash their clothes and the time wasted
    printf("%d\n%d\n", counter, timeWasted);

    // checking if more machines are required
    if((((float)counter / n)*100) > 25){
        printf("Yes\n");
    }
    else{
        printf("No\n");
    }
    return 0;
}