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
#define THREAD_NUM 1    
#define BUFFER_SIZE 256 

typedef struct Clock {
   int p[3];
} Clock;

struct Args {
    int id;
    int pRank;
    Clock clock;
};

struct Args2 {
    int id;
    int pRank;
    
};

int compareClocks(Clock clock1, Clock clock2) {
    if (clock1.p[0] == clock2.p[0] && clock1.p[1] == clock2.p[1] && clock1.p[2] == clock2.p[2]) {
        return 1;
    } else {
        return 0;
    }
}

void Event(int pid, Clock *clock){
   clock->p[pid]++;   
}


void Send(int pidSender, int pidReceiver, Clock *clockSender){
   // TO DO

   
   MPI_Send(clockSender, sizeof(Clock), MPI_BYTE, pidReceiver, 0, MPI_COMM_WORLD);
  
}


Clock clockQueueEntradaP0[BUFFER_SIZE]; // Lista de clocks na fila de entrada de P0
Clock clockQueueEntradaP1[BUFFER_SIZE]; // Lista de clocks na fila de entrada de P1
Clock clockQueueEntradaP2[BUFFER_SIZE]; // Lista de clocks na fila de entrada de P2
Clock clockQueueSaidaP0[BUFFER_SIZE]; // Lista de clocks na fila de saida de P0
Clock clockQueueSaidaP1[BUFFER_SIZE]; // Lista de clocks na fila de saida de P1
Clock clockQueueSaidaP2[BUFFER_SIZE]; // Lista de clocks na fila de saida de P2

int clockCountEntradaP0 = 0;
int clockCountSaidaP0 = 0;
int clockCountEntradaP1 = 0;
int clockCountSaidaP1 = 0;
int clockCountEntradaP2 = 0;
int clockCountSaidaP2 = 0;

pthread_mutex_t mutexEntradaP0; // define mutex
pthread_mutex_t mutexEntradaP1; // define mutex
pthread_mutex_t mutexEntradaP2; // define mutex
pthread_mutex_t mutexSaidaP0; // define mutex
pthread_mutex_t mutexSaidaP1; // define mutex
pthread_mutex_t mutexSaidaP2; // define mutex

pthread_cond_t condFullEntradaP0;  // declara condição
pthread_cond_t condFullSaidaP0;  // declara condição
pthread_cond_t condFullEntradaP1;  // declara condição
pthread_cond_t condFullSaidaP1;  // declara condição
pthread_cond_t condFullEntradaP2;  // declara condição
pthread_cond_t condFullSaidaP2;  // declara condição
pthread_cond_t condEmptyEntradaP0; // declara condição
pthread_cond_t condEmptySaidaP0; // declara condição
pthread_cond_t condEmptyEntradaP1; // declara condição
pthread_cond_t condEmptySaidaP1; // declara condição
pthread_cond_t condEmptyEntradaP2; // declara condição
pthread_cond_t condEmptySaidaP2; // declara condição

void printConsumeClock(Clock *clock, int id)
{
    printf("Thread %d consumed Clock: (%d, %d, %d)\n", id, clock->p[0], clock->p[1], clock->p[2]);
}

void printSubmitClock(Clock *clock, int id)
{
    printf("Thread %d submitted Clock: (%d, %d, %d)\n", id, clock->p[0], clock->p[1], clock->p[2]);
}

Clock getClockFromSaida(int pRank) {
    if (pRank == 0) {
        pthread_mutex_lock(&mutexSaidaP0);
    
        while (clockCountSaidaP0 == 0) {
            pthread_cond_wait(&condEmptySaidaP0, &mutexSaidaP0);
        }
        
        Clock clock = clockQueueSaidaP0[0];
        int i;
        for (i = 0; i < clockCountSaidaP0 - 1; i++) {
        clockQueueSaidaP0[i] = clockQueueSaidaP0[i + 1];
        }
        clockCountSaidaP0--;
        
        pthread_mutex_unlock(&mutexSaidaP0);
        pthread_cond_signal(&condFullSaidaP0);
        return clock;
    } else if (pRank == 1) {
        pthread_mutex_lock(&mutexSaidaP1);
    
        while (clockCountSaidaP1 == 0) {
            pthread_cond_wait(&condEmptySaidaP1, &mutexSaidaP1);
        }
        
        Clock clock = clockQueueSaidaP1[0];
        int i;
        for (i = 0; i < clockCountSaidaP1 - 1; i++) {
        clockQueueSaidaP1[i] = clockQueueSaidaP1[i + 1];
        }
        clockCountSaidaP1--;
        
        pthread_mutex_unlock(&mutexSaidaP1);
        pthread_cond_signal(&condFullSaidaP1);
        return clock;
        
    } else if (pRank == 2) {
        pthread_mutex_lock(&mutexSaidaP2);
    
        while (clockCountSaidaP2 == 0) {
            pthread_cond_wait(&condEmptySaidaP2, &mutexSaidaP2);
        }
        
        Clock clock = clockQueueSaidaP2[0];
        int i;
        for (i = 0; i < clockCountSaidaP2 - 1; i++) {
        clockQueueSaidaP2[i] = clockQueueSaidaP2[i + 1];
        }
        clockCountSaidaP2--;
        
        pthread_mutex_unlock(&mutexSaidaP2);
        pthread_cond_signal(&condFullSaidaP2);
        return clock;
    }
}

Clock getClockFromEntrada(int pRank)
{
    if (pRank == 0) {
        pthread_mutex_lock(&mutexEntradaP0);
    
        while (clockCountEntradaP0 == 0) {
            pthread_cond_wait(&condEmptyEntradaP0, &mutexEntradaP0);
        }
        
        Clock clock = clockQueueEntradaP0[0];
        int i;
        for (i = 0; i < clockCountEntradaP0 - 1; i++) {
        clockQueueEntradaP0[i] = clockQueueEntradaP0[i + 1];
        }
        clockCountEntradaP0--;
        
        pthread_mutex_unlock(&mutexEntradaP0);
        pthread_cond_signal(&condFullEntradaP0);
        return clock;
    } else if (pRank == 1) {
        pthread_mutex_lock(&mutexEntradaP1);
    
        while (clockCountEntradaP1 == 0) {
            pthread_cond_wait(&condEmptyEntradaP1, &mutexEntradaP1);
        }
        
        Clock clock = clockQueueEntradaP1[0];
        int i;
        for (i = 0; i < clockCountEntradaP1 - 1; i++) {
        clockQueueEntradaP1[i] = clockQueueEntradaP1[i + 1];
        }
        clockCountEntradaP1--;
        
        pthread_mutex_unlock(&mutexEntradaP1);
        pthread_cond_signal(&condFullEntradaP1);
        return clock;
        
    } else if (pRank == 2) {
        pthread_mutex_lock(&mutexEntradaP2);
    
        while (clockCountEntradaP2 == 0) {
            pthread_cond_wait(&condEmptyEntradaP2, &mutexEntradaP2);
        }
        
        Clock clock = clockQueueEntradaP2[0];
        int i;
        for (i = 0; i < clockCountEntradaP2 - 1; i++) {
        clockQueueEntradaP2[i] = clockQueueEntradaP2[i + 1];
        }
        clockCountEntradaP2--;
        
        pthread_mutex_unlock(&mutexEntradaP2);
        pthread_cond_signal(&condFullEntradaP2);
        return clock;
    }
}

void submitClockToEntrada(Clock clock, int pRank)
{
    if (pRank == 0) {
        pthread_mutex_lock(&mutexEntradaP0);
        while (clockCountEntradaP0 == BUFFER_SIZE) {
            pthread_cond_wait(&condFullEntradaP0, &mutexEntradaP0);
        }
        
        clockQueueEntradaP0[clockCountEntradaP0] = clock;
        clockCountEntradaP0++;
        
        pthread_mutex_unlock(&mutexEntradaP0);
        pthread_cond_signal(&condEmptyEntradaP0);
        
    } else if (pRank == 1) {
        pthread_mutex_lock(&mutexEntradaP1);
        while (clockCountEntradaP1 == BUFFER_SIZE) {
            pthread_cond_wait(&condFullEntradaP1, &mutexEntradaP1);
        }
        
        clockQueueEntradaP1[clockCountEntradaP1] = clock;
        clockCountEntradaP1++;
        
        pthread_mutex_unlock(&mutexEntradaP1);
        pthread_cond_signal(&condEmptyEntradaP1);
    } else if (pRank == 2) {
        pthread_mutex_lock(&mutexEntradaP2);
        while (clockCountEntradaP2 == BUFFER_SIZE) {
            pthread_cond_wait(&condFullEntradaP2, &mutexEntradaP2);
        }
        
        clockQueueEntradaP2[clockCountEntradaP2] = clock;
        clockCountEntradaP2++;
        
        pthread_mutex_unlock(&mutexEntradaP2);
        pthread_cond_signal(&condEmptyEntradaP2);
    }
}

void submitClockToSaida(Clock clock, int pRank)
{
    if (pRank == 0) {
        pthread_mutex_lock(&mutexSaidaP0);
        while (clockCountSaidaP0 == BUFFER_SIZE) {
            pthread_cond_wait(&condFullSaidaP0, &mutexSaidaP0);
        }
        
        clockQueueSaidaP0[clockCountSaidaP0] = clock;
        clockCountSaidaP0++;
        
        pthread_mutex_unlock(&mutexSaidaP0);
        pthread_cond_signal(&condEmptySaidaP0);
        
    } else if (pRank == 1) {
        pthread_mutex_lock(&mutexSaidaP1);
        while (clockCountSaidaP1 == BUFFER_SIZE) {
            pthread_cond_wait(&condFullSaidaP1, &mutexSaidaP1);
        }
        
        clockQueueSaidaP1[clockCountSaidaP1] = clock;
        clockCountSaidaP1++;
        
        pthread_mutex_unlock(&mutexSaidaP1);
        pthread_cond_signal(&condEmptySaidaP1);
    } else if (pRank == 2) {
        pthread_mutex_lock(&mutexSaidaP2);
        while (clockCountSaidaP2 == BUFFER_SIZE) {
            pthread_cond_wait(&condFullSaidaP2, &mutexSaidaP2);
        }
        
        clockQueueSaidaP2[clockCountSaidaP2] = clock;
        clockCountSaidaP2++;
        
        pthread_mutex_unlock(&mutexSaidaP2);
        pthread_cond_signal(&condEmptySaidaP2);
    }
}

void Receive(int pidSender, int pidReceiver){
   Clock clockMsg; 
   MPI_Recv(&clockMsg, sizeof(Clock), MPI_BYTE, pidSender, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   submitClockToEntrada(clockMsg, pidReceiver);
   printf("O clock (%d, %d, %d) foi recebido pelo processo %d\n", clockMsg.p[0], clockMsg.p[1], clockMsg.p[2], pidReceiver);
   
}

void *startThreadsEntrada(void *args);
void *startThreadsSaida(void *args);
void *startThreadsPrincipal(void *args);


// Representa o processo de rank 0
void process0(int pRank){
    Clock clock = {{0,0,0}};
    
    int i = 0;
    struct Args argumentosEntradaP0;
    struct Args2 argumentosSaidaP0;
    struct Args2 argumentosPrincipalP0;
    argumentosEntradaP0.id = 0;
    argumentosEntradaP0.clock = clock;
    argumentosEntradaP0.pRank = pRank;
    
    
    argumentosSaidaP0.id = 1;
    argumentosSaidaP0.pRank = pRank;
    
    argumentosPrincipalP0.id = 2;
    argumentosPrincipalP0.pRank = pRank;
    
    pthread_mutex_init(&mutexEntradaP0, NULL);
    pthread_mutex_init(&mutexSaidaP0, NULL);
    pthread_cond_init(&condEmptyEntradaP0, NULL);
    pthread_cond_init(&condFullEntradaP0, NULL);
    pthread_cond_init(&condFullSaidaP0, NULL);
    pthread_cond_init(&condEmptySaidaP0, NULL);
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
    
    for (i = 0; i < 3; i++) {
        pthread_join(thread[i], NULL);
    }
    
    
    pthread_mutex_destroy(&mutexEntradaP0);
    pthread_mutex_destroy(&mutexSaidaP0);
    pthread_cond_destroy(&condEmptyEntradaP0);
    pthread_cond_destroy(&condFullEntradaP0);
    pthread_cond_destroy(&condFullSaidaP0);
    pthread_cond_destroy(&condEmptySaidaP0);
}

// Representa o processo de rank 1
void process1(int pRank){
    Clock clock = {{0,0,0}};
    int i = 0;
    struct Args argumentosEntradaP1;
    struct Args2 argumentosSaidaP1;
    struct Args2 argumentosPrincipalP1;
    
    argumentosEntradaP1.id = 0;
    argumentosEntradaP1.clock = clock;
    argumentosEntradaP1.pRank = pRank;
    
    argumentosSaidaP1.id = 1;
    argumentosSaidaP1.pRank = pRank;
    
    argumentosPrincipalP1.id = 2;
    argumentosPrincipalP1.pRank = pRank;
    
    pthread_mutex_init(&mutexEntradaP1, NULL);
    pthread_mutex_init(&mutexSaidaP1, NULL);
    pthread_cond_init(&condEmptyEntradaP1, NULL);
    pthread_cond_init(&condEmptySaidaP1, NULL);
    pthread_cond_init(&condFullEntradaP1, NULL);
    pthread_cond_init(&condFullSaidaP1, NULL);
    pthread_t thread[THREAD_NUM];
    srand(time(NULL));
    
    
    
    if (pthread_create(&thread[i], NULL, &startThreadsEntrada, (void *)&argumentosEntradaP1) != 0) // testar com e sem o "&" no argumento
    {
      perror("Failed to create the thread");
    }
    
    i += 1;
    
    if (pthread_create(&thread[i], NULL, &startThreadsSaida, (void *)&argumentosSaidaP1) != 0) // testar com e sem o "&" no argumento
    {
      perror("Failed to create the thread");
    }
    
    i += 1;
    
    if (pthread_create(&thread[i], NULL, &startThreadsPrincipal, (void *)&argumentosPrincipalP1) != 0)
    {
      perror("Failed to create the thread");
    }
    
    for (i = 0; i < 3; i++) {
        pthread_join(thread[i], NULL);
    }
    
    
    pthread_mutex_destroy(&mutexEntradaP1);
    pthread_mutex_destroy(&mutexSaidaP1);
    pthread_cond_destroy(&condEmptyEntradaP1);
    pthread_cond_destroy(&condEmptySaidaP1);
    pthread_cond_destroy(&condFullEntradaP1);
    pthread_cond_destroy(&condFullSaidaP1);
   
   
}

// Representa o processo de rank 2
void process2(int pRank){
    Clock clock = {{0,0,0}};
    int i = 0;
    struct Args argumentosEntradaP2;
    struct Args2 argumentosSaidaP2;
    struct Args2 argumentosPrincipalP2;
    argumentosEntradaP2.id = 0;
    argumentosEntradaP2.clock = clock;
    argumentosEntradaP2.pRank = pRank;
    
    argumentosSaidaP2.id = 1;
    argumentosSaidaP2.pRank = pRank;
    
    argumentosPrincipalP2.id = 2;
    argumentosPrincipalP2.pRank = pRank;
    
    pthread_mutex_init(&mutexEntradaP2, NULL);
    pthread_mutex_init(&mutexSaidaP2, NULL);
    pthread_cond_init(&condEmptyEntradaP2, NULL);
    pthread_cond_init(&condEmptySaidaP2, NULL);
    pthread_cond_init(&condFullEntradaP2, NULL);
    pthread_cond_init(&condFullSaidaP2, NULL);
    
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
    
    for (i = 0; i < 3; i++) {
        pthread_join(thread[i], NULL);
    }
    
    
    pthread_mutex_destroy(&mutexEntradaP2);
    pthread_mutex_destroy(&mutexSaidaP2);
    pthread_cond_destroy(&condEmptyEntradaP2);
    pthread_cond_destroy(&condEmptySaidaP2);
    pthread_cond_destroy(&condFullEntradaP2);
    pthread_cond_destroy(&condFullSaidaP2);
   
}

int main(void) {
    
    int my_rank;
    
    MPI_Init(NULL, NULL); 
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); 
    
    if (my_rank == 0) { 
      process0(0);
    } else if (my_rank == 1) {  
      process1(1);
    } else if (my_rank == 2) {  
      process2(2);
    }
    
    
    /* Finaliza MPI */
    MPI_Finalize(); 
    
    return 0;
}  /* main */



void *startThreadsEntrada(void *args)
{
    struct Args *argumentos = (struct Args *)args;
    Clock clockAtual = argumentos->clock; // testar
    int pRank = argumentos->pRank; // testar
    int id = argumentos->id;
    // printf("////// PROCESSO %d | THREAD ENTRADA: %d //////\n", pRank, id);
    
    if (pRank == 0) {
        
           Receive(1, 0);
           Receive(2, 0); 
        
            
    } else if (pRank == 1) {
        while (1) {
            Receive(0, 1);
        }
        
    } else if (pRank == 2) {
        while (1) {
            Receive(0, 2);
        }
    } 
    return NULL;
}

void *startThreadsSaida(void *args)
{
    struct Args *argumentos = (struct Args *)args;
    int pRank = argumentos->pRank; 
    int id = argumentos->id;
    
    // printf("////// PROCESSO %d | THREAD SAIDA: %d //////\n", pRank, id);
    
    if (pRank == 0 ) {
        Clock clock = {{0, 0, 0}}; 
        
        Clock a = {{1, 0, 0}};
        Clock c = {{3, 1, 0}};
        Clock e = {{5, 1, 2}};
        
        while (1) {
            
            if (clockCountSaidaP0 > 0) {
                clock = getClockFromSaida(pRank);
                // printf("O clock (%d, %d, %d) foi retirado da fila de saida do processo %d\n", clock.p[0], clock.p[1], clock.p[2], pRank);
            }
            
            if (compareClocks(clock, a)) {
                clock.p[pRank]++;
                Send(pRank, 1, &clock);
                printf("O clock (%d, %d, %d) foi enviado do processo %d para o processo %d\n", clock.p[0], clock.p[1], clock.p[2], 0, 1);
            } else if (compareClocks(clock, c)) {
                clock.p[pRank]++;
                Send(pRank, 2, &clock);
                printf("O clock (%d, %d, %d) foi enviado do processo %d para o processo %d\n", clock.p[0], clock.p[1], clock.p[2], 0, 2);
            } else if (compareClocks(clock, e )) {
                clock.p[pRank]++;
                Send(pRank, 1, &clock);
                printf("O clock (%d, %d, %d) foi enviado do processo %d para o processo %d\n", clock.p[0], clock.p[1], clock.p[2], 0, 1);
            }
            
        }
        
        
    } else if (pRank == 1) {
        Clock clock = {{0, 0, 0}}; 
        
        while (1) {
            
            if (clockCountSaidaP1 > 0) {
                clock = getClockFromSaida(pRank);
            // printf("O clock (%d, %d, %d) foi retirado da fila de saida do processo %d\n", clock.p[0], clock.p[1], clock.p[2], pRank);
            }
            
            Clock inicial = {{0, 0, 0}};
            if (compareClocks(clock, inicial)) {
                clock.p[pRank]++;
                Send(pRank, 0, &clock);
                printf("O clock (%d, %d, %d) foi enviado do processo %d para o processo %d\n", clock.p[0], clock.p[1], clock.p[2], 1, 0);
            }
        }
        
        
    } else if (pRank == 2) {
        Clock clock = {{0, 0, 0}}; 
        while(1) {
            
            if (clockCountSaidaP2 > 0) {
               clock = getClockFromSaida(pRank);
            // printf("O clock (%d, %d, %d) foi retirado da fila de saida do processo %d\n", clock.p[0], clock.p[1], clock.p[2], pRank);
            }
            
            Clock k = {{0, 0, 1}};
            if (compareClocks(clock, k)) {
                clock.p[pRank]++;
                Send(pRank, 0, &clock);
                printf("O clock (%d, %d, %d) foi enviado do processo %d para o processo %d\n", clock.p[0], clock.p[1], clock.p[2], 2, 0);
            }
        }
    }
  
    return NULL;
}

void *startThreadsPrincipal(void *args) {
    
    struct Args *argumentos = (struct Args *)args;
    int pRank = argumentos->pRank; 
    int id = argumentos->id;
    
    // printf("////// PROCESSO %d | THREAD PRINCIPAL: %d //////\n", pRank, id);
    if (pRank == 0) {
        Clock currentClock = {{0, 0, 0}};
        Clock g = {{6, 1, 2}};
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
        Event(pRank, &currentClock); 
        while (1) {
            printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
            
            Clock a = {{1, 0, 0}};
            Clock c = {{3, 1, 0}};
            Clock e = {{5, 1, 2}};
            
            if (compareClocks(currentClock, a) || compareClocks(currentClock, c) || compareClocks(currentClock, e)) {
                submitClockToSaida(currentClock, pRank);
                // printf("O clock (%d, %d, %d) foi submetido para a fila de saida do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
                currentClock.p[pRank]++;
                printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
            }
            
            if (compareClocks(currentClock, g)) {
                currentClock.p[pRank]++;
                printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
            }
            
            
            if (clockQueueEntradaP0 > 0) {
                Clock nextClock = getClockFromEntrada(pRank);
                currentClock.p[pRank]++;
                for(int i = 0; i < 3; i++) {
                    currentClock.p[i] = max(nextClock.p[i], currentClock.p[i]);
                }
            }
        }
    } else if (pRank == 1) {
        
        Clock currentClock = {{0, 0, 0}};
        while (1) {
            printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
            
            Clock h = {{0, 0, 0}};
            
            if (compareClocks(currentClock, h)) {
                submitClockToSaida(currentClock, pRank);
                currentClock.p[pRank]++;
                printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
                // printf("O clock (%d, %d, %d) foi submetido para a fila de saida do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
            }
            if (clockQueueEntradaP1 > 0) {
                Clock nextClock = getClockFromEntrada(pRank);
                currentClock.p[pRank]++;
                for(int i = 0; i < 3; i++) {
                    currentClock.p[i] = max(nextClock.p[i], currentClock.p[i]);
                }
            }
        }
    } else if (pRank == 2) {
        Clock currentClock = {{0, 0, 0}};
        printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
        Event(pRank, &currentClock); 
        
        while (1) {
            printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
            
            Clock k = {{0, 0, 1}};
            
            if (compareClocks(currentClock, k)) {
                submitClockToSaida(currentClock, pRank);
                currentClock.p[pRank]++;
                printf("O clock atual é (%d, %d, %d) do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
                // printf("O clock (%d, %d, %d) foi submetido para a fila de saida do processo %d\n", currentClock.p[0], currentClock.p[1], currentClock.p[2], pRank);
            }
            
            if (clockQueueEntradaP2 > 0) {
                Clock nextClock = getClockFromEntrada(pRank);
                currentClock.p[pRank]++;
                for(int i = 0; i < 3; i++) {
                    currentClock.p[i] = max(nextClock.p[i], currentClock.p[i]);
                }
            }
        }
    }
}
