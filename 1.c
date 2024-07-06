#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include<string.h>

// Define colors for output
#define WHITE "\033[0;97m"
#define YELLOW "\033[0;93m"
#define CYAN "\033[0;96m"
#define BLUE "\033[0;94m"
#define GREEN "\033[0;92m"
#define RED "\033[0;91m"
#define RESET "\033[0m"
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int*B;
int*N;
int*K;
sem_t* barista_available;
sem_t* customer_done;

char** coffee_names;
int* coffee_times;

typedef struct {
    int id;
    int coffee_type;
    int arrival_time;
    int tolerance;
    int wait;
} Customer;

typedef struct {
    Customer** customers;
    int front, rear;
} Queue;

Queue waiting_queue;

void initQueue() {
    waiting_queue.front = -1;
    waiting_queue.rear = -1;
    waiting_queue.customers = (Customer**)malloc((*N) * sizeof(Customer*));
    for (int i = 0; i < *N; i++) {
        waiting_queue.customers[i] = NULL;
    }
}

int isQueueEmpty() {
    return waiting_queue.front == -1;
}

void enqueue(Customer* customer) {
    if (waiting_queue.rear == (*N) - 1)
        return; // Queue is full
    if (waiting_queue.front == -1)
        waiting_queue.front = 0;
    waiting_queue.rear++;
    waiting_queue.customers[waiting_queue.rear] = customer;
}

Customer* dequeue() {
    if (waiting_queue.front == -1)
        return NULL; // Queue is empty
    Customer* customer = waiting_queue.customers[waiting_queue.front];
    if (waiting_queue.front == waiting_queue.rear)
        waiting_queue.front = waiting_queue.rear = -1;
    else
        waiting_queue.front++;
    return customer;
}

pthread_cond_t barista_condition = PTHREAD_COND_INITIALIZER;
int* baristafree;

typedef struct {
    int customer_id;
    int event_type; // 0: Customer arrival, 1: Customer order, 2: Barista starts preparing, 3: Order completion,4:customer left without taking order,5. customer left after taking order
    int barista_id;
    int time;
} Event;

Event* events;
int event_count = 0;

void addEvent(int customer_id, int event_type, int barista_id, int time) {
    events[event_count].customer_id = customer_id;
    events[event_count].event_type = event_type;
    events[event_count].barista_id = barista_id;
    events[event_count].time = time;
    event_count++;
}

int coffee_waste=0;
void* customer_behavior(void* arg) {
    Customer* customer = (Customer*)arg;
    int id = customer->id;
    int coffee_type = customer->coffee_type;
    int arrival_time = customer->arrival_time;
    int tolerance = customer->tolerance;
    int needtowait = customer->wait;

    //usleep(arrival_time * 1000000);
    addEvent(id, 0, -1, arrival_time); // Customer arrival event
    //printf(WHITE "Customer %d arrives at %d second(s)\n" RESET, id, arrival_time);
    addEvent(id, 1, -1, arrival_time); // Customer order event
     //printf(YELLOW "Customer %d orders a %s\n" RESET, id, coffee_names[coffee_type]);
    int selected_barista = -1;
    pthread_mutex_lock(&mutex);
    while (selected_barista == -1) {
        for (int i = 0; i < *B; i++) {
            if (sem_trywait(&barista_available[i]) == 0) {
                selected_barista = i;
                break;
            }
        }
        if (selected_barista == -1) {
            enqueue(customer);
            customer->wait = 1;
            pthread_cond_wait(&barista_condition, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);

    if (customer->wait != 1) {
        addEvent(id, 2, selected_barista, arrival_time + 1); // Barista starts preparing event
         //printf(CYAN "Barista %d begins preparing the order of customer %d at %d second(s)\n" RESET, selected_barista + 1, id, arrival_time+1);
    } else {
        Customer* next_customer = dequeue();
        if (next_customer != NULL) {
            if(arrival_time+tolerance>=baristafree[selected_barista]){
            addEvent(next_customer->id, 2, selected_barista, baristafree[selected_barista] + 1); // Barista starts preparing event for the next customer
             //printf(CYAN "Barista %d begins preparing the order of customer %d at %d second(s)\n" RESET,selected_barista + 1, next_customer->id,baristafree[selected_barista] + 1);

            }
            else{
                addEvent(next_customer->id, 4, selected_barista,arrival_time+tolerance);
                 //printf(RED "Customer %d leaves without their order at %d second(s)\n" RESET, next_customer->id,arrival_time+tolerance+1 );
            }
        }
    }
    if(customer->wait==0 || (customer->wait==1 && arrival_time+tolerance>=baristafree[selected_barista]))
    {
    usleep(coffee_times[coffee_type] * 1000000);
    int completed_time;
    if (customer->wait != 1) {
        completed_time = arrival_time + 1 + coffee_times[coffee_type];
    } else {
        completed_time = baristafree[selected_barista] + 1 + coffee_times[coffee_type];
    }

    if (completed_time > arrival_time + tolerance && completed_time != arrival_time + tolerance + 1) {
        //addEvent(id, 3, selected_barista, completed_time); // customer left without taking order event
         //printf(BLUE "Barista %d completes the order of customer %d at %d second(s)\n" RESET, selected_barista,id, completed_time);
         addEvent(id, 4, selected_barista, arrival_time+tolerance); 
          //printf(RED "Customer %d leaves without their order at %d second(s)\n" RESET,id, arrival_time+tolerance+1 );
          addEvent(id, 3, selected_barista, completed_time); // customer left without taking order event
         //printf(BLUE "Barista %d completes the order of customer %d at %d second(s)\n" RESET, selected_barista+1,id, completed_time);
        baristafree[selected_barista] = completed_time;
        coffee_waste++;
        //sem_post(&barista_available[selected_barista]);
        //pthread_cond_signal(&barista_condition);
    } else {
        addEvent(id, 3, selected_barista, completed_time); // Order completion event
         //printf(BLUE "Barista %d completes the order of customer %d at %d second(s)\n" RESET, selected_barista+1,id, completed_time);
         addEvent(id, 5, selected_barista, completed_time); 
         //printf(GREEN "Customer %d leaves with their order at %d second(s)\n" RESET, id, completed_time );
        baristafree[selected_barista] = completed_time;
        //sem_post(&barista_available[selected_barista]);
        //pthread_cond_signal(&barista_condition);
    }
    }
    sem_post(&barista_available[selected_barista]);
        pthread_cond_signal(&barista_condition);
    sem_post(&customer_done[id - 1]); 
    pthread_exit(NULL);
}

int main() {
    B=(int*)malloc(sizeof(int));
    K=(int*)malloc(sizeof(int));
    N=(int*)malloc(sizeof(int));
    scanf("%d %d %d",B,K,N);
    initQueue(); // Initialize the waiting queue
    int numcoffee=0;
     barista_available = malloc(sizeof(sem_t) * (*B));
     customer_done = malloc(sizeof(sem_t) * (*N));

    coffee_names = (char**)malloc((*K) * sizeof(char*));
    coffee_times = (int*)malloc((*K) * sizeof(int));

    for (int i = 0; i < *K; i++) {
        coffee_names[i] = (char*)malloc(20 * sizeof(char));
        scanf("%s %d", coffee_names[i], &coffee_times[i]);
        numcoffee++;
    }

    pthread_t customers[*N];
    for (int i = 0; i < *B; i++) {
        sem_init(&barista_available[i], 0, 1);
    }
    baristafree=(int*)malloc((*B)*sizeof(int));
     events=(Event*)malloc((((*N) * 5)+5)*sizeof(Event));
    Customer customer_data[*N];
    

   
     
    for (int i = 0; i < *N; i++) {
        int id, coffee_types, arrival_time, tolerance, wait;
        char coffee_index[20];
        scanf("%d %s %d %d", &id, coffee_index, &arrival_time, &tolerance);
         for(int j=0;j<numcoffee;j++){
            if(strcmp(coffee_index,coffee_names[j])==0){
                      coffee_types=j;
                      break;
            }
         }
        customer_data[i].id = id;
        customer_data[i].coffee_type = coffee_types;
        customer_data[i].arrival_time = arrival_time;
        customer_data[i].tolerance = tolerance;
        customer_data[i].wait =0;
    }
for (int i = 0; i < (*N) - 1; i++) {
    for (int j = i + 1; j < *N; j++) {
        if (
            (customer_data[i].arrival_time == customer_data[j].arrival_time && customer_data[i].id > customer_data[j].id)) {
            Customer temp = customer_data[i];
            customer_data[i] = customer_data[j];
            customer_data[j] = temp;
        }
    }
}

// Create threads for customers after sorting
for (int i = 0; i < *N; i++) {
    pthread_create(&customers[i], NULL, customer_behavior, &customer_data[i]);
     usleep(100); // Add a small delay between creating threads to prioritize lower ID customers

}
// Wait for each customer's thread to finish before processing the next
    for (int i = 0; i < *N; i++) {
        sem_wait(&customer_done[i]);
    }
    

    for (int i = 0; i < event_count - 1; i++) {
        for (int j = 0; j < event_count - i - 1; j++) {
            if (events[j].time > events[j + 1].time) {
                Event temp = events[j];
                events[j] = events[j + 1];
                events[j + 1] = temp;
            }
        }
    }

    for (int i = 0; i < event_count; i++) {
        if (events[i].event_type == 0) {
            if(i!=0){
            usleep((events[i].time-events[i-1].time)*1000000);
            }
            else{
                usleep(events[i].time*1000000);
            }
            printf(WHITE "Customer %d arrives at %d second(s)\n" RESET, events[i].customer_id, events[i].time);
        } else if (events[i].event_type == 1) {
              if(i!=0){
            usleep((events[i].time-events[i-1].time)*1000000);
            }
            else{
                usleep(events[i].time*1000000);
            }
            printf(YELLOW "Customer %d orders a %s\n" RESET, events[i].customer_id, coffee_names[customer_data[events[i].customer_id - 1].coffee_type]);
        } else if (events[i].event_type == 2) {
              if(i!=0){
            usleep((events[i].time-events[i-1].time)*1000000);
            }
            else{
                usleep(events[i].time*1000000);
            }
            printf(CYAN "Barista %d begins preparing the order of customer %d at %d second(s)\n" RESET, events[i].barista_id + 1, events[i].customer_id, events[i].time);
        } else if (events[i].event_type == 3) {
             if(i!=0){
            usleep((events[i].time-events[i-1].time)*1000000);
            }
            else{
                usleep(events[i].time*1000000);
            }
            printf(BLUE "Barista %d completes the order of customer %d at %d second(s)\n" RESET, events[i].barista_id + 1, events[i].customer_id, events[i].time);
        }
        else if(events[i].event_type==4){
              if(i!=0){
            usleep((events[i].time-events[i-1].time)*1000000);
            }
            else{
                usleep(events[i].time*1000000);
            }
            printf(RED "Customer %d leaves without their order at %d second(s)\n" RESET, events[i].customer_id, events[i].time+1 );
        }
        else if(events[i].event_type==5){
              if(i!=0){
            usleep((events[i].time-events[i-1].time)*1000000);
            }
            else{
                usleep(events[i].time*1000000);
            }
            printf(GREEN "Customer %d leaves with their order at %d second(s)\n" RESET, events[i].customer_id, events[i].time );
        }
    }
    printf("%d coffee wasted\n",coffee_waste);
    for (int i = 0; i < *B; i++) {
        sem_destroy(&barista_available[i]);
    }

    return 0;
}
