// Compilação: mpicc -o integracao integracao.c
// Execução:   mpiexec -n 3 ./integracao

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define THREAD_NUM 1    // Tamanho do pool de threads
#define BUFFER_SIZE 256 // Número máximo de clocks enfileirados

typedef struct Clock
{
    int p[3];
} Clock;

struct Args
{
    int id;
    int pRank;
};

Clock clockQueueSaida[BUFFER_SIZE];   // Lista de clocks
Clock clockQueueEntrada[BUFFER_SIZE]; // Lista de clocks
int clockCountSaida = 0;
int clockCountEntrada = 0;

pthread_mutex_t mutexSaida;   // define mutex
pthread_mutex_t mutexEntrada; // define mutex

pthread_cond_t condFullEntrada;  // declara condição
pthread_cond_t condFullSaida;    // declara condição
pthread_cond_t condEmptyEntrada; // declara condição
pthread_cond_t condEmptySaida;   // declara condição

int compareClocks(Clock clock1, Clock clock2)
{
    if (clock1.p[0] == clock2.p[0] && clock1.p[1] == clock2.p[1] && clock1.p[2] == clock2.p[2])
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void updateClock(Clock *currentClock, Clock *nextClock)
{
    for (int i = 0; i < 3; i++)
    {
        currentClock->p[i] = max(nextClock->p[i], currentClock->p[i]);
    }
}

void Event(int pid, Clock *clock)
{
    clock->p[pid]++;
}

void SendMPI(int pidSender, int pidReceiver, Clock *clockSender)
{
    MPI_Send(clockSender, sizeof(Clock), MPI_BYTE, pidReceiver, 0, MPI_COMM_WORLD);
}

Clock getClockFromSaida()
{
    pthread_mutex_lock(&mutexSaida);

    while (clockCountSaida == 0)
    {
        pthread_cond_wait(&condEmptySaida, &mutexSaida);
    }

    Clock clock = clockQueueSaida[0];
    int i;
    for (i = 0; i < clockCountSaida - 1; i++)
    {
        clockQueueSaida[i] = clockQueueSaida[i + 1];
    }
    clockCountSaida--;

    pthread_mutex_unlock(&mutexSaida);
    pthread_cond_signal(&condFullSaida);
    return clock;
}

Clock Receive()
{
    pthread_mutex_lock(&mutexEntrada);

    while (clockCountEntrada == 0)
    {
        pthread_cond_wait(&condEmptyEntrada, &mutexEntrada);
    }

    Clock clock = clockQueueEntrada[0];
    int i;
    for (i = 0; i < clockCountEntrada - 1; i++)
    {
        clockQueueEntrada[i] = clockQueueEntrada[i + 1];
    }
    clockCountEntrada--;

    pthread_mutex_unlock(&mutexEntrada);
    pthread_cond_signal(&condFullEntrada);
    return clock;
}

void submitClockToEntrada(Clock clock)
{
    pthread_mutex_lock(&mutexEntrada);
    while (clockCountEntrada == BUFFER_SIZE)
    {
        pthread_cond_wait(&condFullEntrada, &mutexEntrada);
    }

    clockQueueEntrada[clockCountEntrada] = clock;
    clockCountEntrada++;

    pthread_mutex_unlock(&mutexEntrada);
    pthread_cond_signal(&condEmptyEntrada);
}

void Send(Clock clock)
{
    pthread_mutex_lock(&mutexSaida);
    while (clockCountSaida == BUFFER_SIZE)
    {
        pthread_cond_wait(&condFullSaida, &mutexSaida);
    }

    clockQueueSaida[clockCountSaida] = clock;
    clockCountSaida++;

    pthread_mutex_unlock(&mutexSaida);
    pthread_cond_signal(&condEmptySaida);
}

void ReceiveMPI()
{
    Clock clockMsg;
    MPI_Recv(&clockMsg, sizeof(Clock), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    submitClockToEntrada(clockMsg);
}

void *startThreadsEntrada(void *args);
void *startThreadsSaida(void *args);
void *startThreadsPrincipal(void *args);

// Representa o processo de rank 0
void process0(int pRank)
{

    pthread_mutex_init(&mutexEntrada, NULL);
    pthread_mutex_init(&mutexSaida, NULL);

    pthread_cond_init(&condEmptySaida, NULL);
    pthread_cond_init(&condEmptyEntrada, NULL);
    pthread_cond_init(&condFullSaida, NULL);
    pthread_cond_init(&condFullEntrada, NULL);

    Clock clock = {{0, 0, 0}};

    int i = 0;
    struct Args argumentosEntradaP0;
    struct Args argumentosSaidaP0;
    struct Args argumentosPrincipalP0;
    argumentosEntradaP0.id = 0;
    argumentosEntradaP0.pRank = pRank;

    argumentosSaidaP0.id = 1;
    argumentosSaidaP0.pRank = pRank;

    argumentosPrincipalP0.id = 2;
    argumentosPrincipalP0.pRank = pRank;

    pthread_t thread[THREAD_NUM];
    srand(time(NULL));

    if (pthread_create(&thread[i], NULL, &startThreadsEntrada, (void *)&argumentosEntradaP0) != 0)
    {
        perror("Failed to create the thread");
    }

    i += 1;

    if (pthread_create(&thread[i], NULL, &startThreadsSaida, (void *)&argumentosSaidaP0) != 0)
    {
        perror("Failed to create the thread");
    }

    i += 1;

    if (pthread_create(&thread[i], NULL, &startThreadsPrincipal, (void *)&argumentosPrincipalP0) != 0)
    {
        perror("Failed to create the thread");
    }

    for (i = 0; i < 3; i++)
    {
        pthread_join(thread[i], NULL);
    }

    pthread_mutex_destroy(&mutexEntrada);
    pthread_mutex_destroy(&mutexSaida);
    pthread_cond_destroy(&condEmptyEntrada);
    pthread_cond_destroy(&condEmptySaida);
    pthread_cond_destroy(&condFullEntrada);
    pthread_cond_destroy(&condFullSaida);
}

// Representa o processo de rank 1
void process1(int pRank)
{
    pthread_mutex_init(&mutexEntrada, NULL);
    pthread_mutex_init(&mutexSaida, NULL);

    pthread_cond_init(&condEmptySaida, NULL);
    pthread_cond_init(&condEmptyEntrada, NULL);
    pthread_cond_init(&condFullSaida, NULL);
    pthread_cond_init(&condFullEntrada, NULL);

    Clock clockQueueSaida[BUFFER_SIZE];   // Lista de clocks
    Clock clockQueueEntrada[BUFFER_SIZE]; // Lista de clocks

    Clock clock = {{0, 0, 0}};
    int i = 0;
    struct Args argumentosEntradaP1;
    struct Args argumentosSaidaP1;
    struct Args argumentosPrincipalP1;

    argumentosEntradaP1.id = 0;
    argumentosEntradaP1.pRank = pRank;

    argumentosSaidaP1.id = 1;
    argumentosSaidaP1.pRank = pRank;

    argumentosPrincipalP1.id = 2;
    argumentosPrincipalP1.pRank = pRank;

    pthread_t thread[THREAD_NUM];
    srand(time(NULL));

    if (pthread_create(&thread[i], NULL, &startThreadsEntrada, (void *)&argumentosEntradaP1) != 0)
    {
        perror("Failed to create the thread");
    }

    i += 1;

    if (pthread_create(&thread[i], NULL, &startThreadsSaida, (void *)&argumentosSaidaP1) != 0)
    {
        perror("Failed to create the thread");
    }

    i += 1;

    if (pthread_create(&thread[i], NULL, &startThreadsPrincipal, (void *)&argumentosPrincipalP1) != 0)
    {
        perror("Failed to create the thread");
    }

    for (i = 0; i < 3; i++)
    {
        pthread_join(thread[i], NULL);
    }

    pthread_mutex_destroy(&mutexEntrada);
    pthread_mutex_destroy(&mutexSaida);
    pthread_cond_destroy(&condEmptyEntrada);
    pthread_cond_destroy(&condEmptySaida);
    pthread_cond_destroy(&condFullEntrada);
    pthread_cond_destroy(&condFullSaida);
}

// Representa o processo de rank 2
void process2(int pRank)
{

    pthread_mutex_init(&mutexEntrada, NULL);
    pthread_mutex_init(&mutexSaida, NULL);

    pthread_cond_init(&condEmptySaida, NULL);
    pthread_cond_init(&condEmptyEntrada, NULL);
    pthread_cond_init(&condFullSaida, NULL);
    pthread_cond_init(&condFullEntrada, NULL);

    Clock clock = {{0, 0, 0}};
    int i = 0;
    struct Args argumentosEntradaP2;
    struct Args argumentosSaidaP2;
    struct Args argumentosPrincipalP2;
    argumentosEntradaP2.id = 0;
    argumentosEntradaP2.pRank = pRank;

    argumentosSaidaP2.id = 1;
    argumentosSaidaP2.pRank = pRank;

    argumentosPrincipalP2.id = 2;
    argumentosPrincipalP2.pRank = pRank;

    pthread_t thread[THREAD_NUM];

    if (pthread_create(&thread[i], NULL, &startThreadsEntrada, (void *)&argumentosEntradaP2) != 0)
    {
        perror("Failed to create the thread");
    }

    i += 1;

    if (pthread_create(&thread[i], NULL, &startThreadsSaida, (void *)&argumentosSaidaP2) != 0)
    {
        perror("Failed to create the thread");
    }

    i += 1;

    if (pthread_create(&thread[i], NULL, &startThreadsPrincipal, (void *)&argumentosPrincipalP2) != 0)
    {
        perror("Failed to create the thread");
    }

    for (i = 0; i < 3; i++)
    {
        pthread_join(thread[i], NULL);
    }

    pthread_mutex_destroy(&mutexEntrada);
    pthread_mutex_destroy(&mutexSaida);
    pthread_cond_destroy(&condEmptyEntrada);
    pthread_cond_destroy(&condEmptySaida);
    pthread_cond_destroy(&condFullEntrada);
    pthread_cond_destroy(&condFullSaida);
}

int main(void)
{

    int my_rank;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (my_rank == 0)
    {
        process0(0);
    }
    else if (my_rank == 1)
    {
        process1(1);
    }
    else if (my_rank == 2)
    {
        process2(2);
    }

    /* Finaliza MPI */
    MPI_Finalize();

    return 0;
} /* main */

void *startThreadsEntrada(void *args)
{
    struct Args *argumentos = (struct Args *)args;
    int pRank = argumentos->pRank;
    int id = argumentos->id;

    while (1)
    {
        ReceiveMPI();
    }

    return NULL;
}

void *startThreadsSaida(void *args)
{
    struct Args *argumentos = (struct Args *)args;
    int pRank = argumentos->pRank;
    int id = argumentos->id;

    if (pRank == 0)
    {
        Clock clock = {{0, 0, 0}};

        Clock b = {{2, 0, 0}};
        Clock d = {{4, 1, 0}};
        Clock f = {{6, 1, 2}};

        while (1)
        {
            clock = getClockFromSaida();

            if (compareClocks(clock, b) || compareClocks(clock, f))
            {
                SendMPI(pRank, 1, &clock);
            }
            else if (compareClocks(clock, d))
            {
                SendMPI(pRank, 2, &clock);
            }
        }
    }
    else if (pRank == 1)
    {
        Clock clock = {{0, 0, 0}};

        while (1)
        {
            clock = getClockFromSaida();

            Clock h = {{0, 1, 0}};
            if (compareClocks(clock, h))
            {
                SendMPI(pRank, 0, &clock);
            }
        }
    }
    else if (pRank == 2)
    {
        Clock clock = {{0, 0, 0}};
        while (1)
        {
            clock = getClockFromSaida();

            Clock l = {{0, 0, 2}};
            if (compareClocks(clock, l))
            {
                SendMPI(pRank, 0, &clock);
            }
        }
    }
    return NULL;
}

void *startThreadsPrincipal(void *args)
{

    struct Args *argumentos = (struct Args *)args;
    int pRank = argumentos->pRank;
    int id = argumentos->id;

    if (pRank == 0)
    {
        Clock currentClock = {{0, 0, 0}};

        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
        Event(pRank, &currentClock);

        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        currentClock.p[pRank]++;
        Send(currentClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        Clock nextClock = Receive();
        currentClock.p[pRank]++;
        updateClock(&currentClock, &nextClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        currentClock.p[pRank]++;
        Send(currentClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        nextClock = Receive();
        currentClock.p[pRank]++;
        updateClock(&currentClock, &nextClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        currentClock.p[pRank]++;
        Send(currentClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        Event(pRank, &currentClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
    }
    else if (pRank == 1)
    {
        Clock currentClock = {{0, 0, 0}};

        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        currentClock.p[pRank]++;
        Send(currentClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        Clock nextClock = Receive();
        currentClock.p[pRank]++;
        updateClock(&currentClock, &nextClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        nextClock = Receive();
        currentClock.p[pRank]++;
        updateClock(&currentClock, &nextClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
    }
    else if (pRank == 2)
    {
        Clock currentClock = {{0, 0, 0}};
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
        Event(pRank, &currentClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        currentClock.p[pRank]++;
        Send(currentClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);

        Clock nextClock = Receive();
        currentClock.p[pRank]++;
        updateClock(&currentClock, &nextClock);
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
    }
}
