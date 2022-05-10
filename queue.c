#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>

#define FACTOR 1000000 // usec to sec multiply factor

/* Struct for list nodes */
struct list_node_s {
int data;
struct list_node_s* next;
};

/* Struct for task nodes */
struct task_node_s {
int task_num;
int task_type; // insert:0, delete:1, search:2
int value;
struct task_node_s* next;
};

// List root
struct list_node_s** root_list;
// Root task
struct task_node_s** root_task;

int task_flag = 0;

pthread_mutex_t mutex; 
pthread_cond_t cond;

struct timeval check_point; 
long beginning_time;
long end_time;

/* List operations */
int Insert(int value);
int Delete(int value);
int Search(int value);

/* Task queue functions */
void Task_queue(int n); //generate random tasks for the task queue
void Task_enqueue(int task_num, int task_type, int value); //insert a new task into task queue
int Task_dequeue(long my_rank, int* task_num_p, int* task_type_p, int* value_p); //take a task from task queue

void Deallocate_queues();
void end(); // A function for stoping program in single thread I implement it because
            // my program gives an error at the end of the queue for some reason.


void* routine(void* i);

int main(int argc, char const *argv[])
{
    gettimeofday(&check_point, NULL);
    beginning_time = (long)check_point.tv_sec * FACTOR + (long)check_point.tv_usec;

    // Allocating roots
    root_task = malloc(sizeof(struct task_node_s));
    root_list = malloc(sizeof(struct list_node_s));

    // Innitializing mutex and cond
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // get thread count from first agument
    int thread_count = atoi(argv[1]);
    printf("thread count is %d\n", thread_count);
    
    // get task count from second agument
    int task_count = atoi(argv[2]);
    printf("task count is %d\n", task_count);

    pthread_t th[thread_count];
    int i;

    for (i = 0; i < thread_count; i++){
        int* a = malloc(sizeof(int));     // thread index as argument
        *a = i;                           //
        if (pthread_create(&th[i], NULL, &routine, a) != 0) {
            return 1;
        }
    }

    Task_queue(task_count); 

    for (i = 0; i < thread_count; i++){
        if (pthread_join(th[i], NULL) != 0) {
            return 1;
        }
    }

    end();
    return 0;
}

int Insert(int value){

    struct list_node_s* new_node = malloc(sizeof(struct list_node_s));
    new_node->data = value;
    new_node->next = NULL;

    // Adding at beginning
    if (*root_list == NULL  || (*root_list)->data > value) {
        new_node->next = *root_list;
        *root_list = new_node;
        printf(" %d is instered\n", value);
        return 0;
    } 
    else if ((*root_list)->data == value) {
        printf(" %d cannot be instered\n", value);
        return -1;       
    }

    struct list_node_s *curr = *root_list;
    while (curr->next != NULL){
        if (curr->next->data > value) {
            new_node->next = curr->next;
            curr->next = new_node;
            printf(" %d is instered\n", value);
            return 0;
        }
        else if (curr->next->data == value){
            printf(" %d cannot be instered\n", value);
            return -1;  
        }
        curr = curr->next;
    }
    // Adding to end
    new_node->next = curr->next;
    curr->next = new_node;
    printf(" %d is instered\n", value);
    return 0;

}

int Delete(int value){

    if (*root_list == NULL){
        printf(" %d cannot be removed\n", value);
        return -1;
    }
    
    if ((*root_list)->data == value) {
        struct list_node_s* to_remove = *root_list;
        *root_list = (*root_list)->next;
        free(to_remove);
        printf(" %d is removed\n", value);
        return 0;
    }

    for (struct list_node_s* curr = *root_list; curr->next != NULL; curr = curr->next){
        if (curr->next->data == value) {
            struct list_node_s* to_remove = curr->next;
            curr->next = curr->next->next;
            free(to_remove);
            printf(" %d is removed\n", value);
            return 0;
        }
    }
    printf(" %d cannot be removed\n", value);
    return -1;
}

int Search(int value){

    if (*root_list == NULL){
        printf(" %d is not found\n", value);
        return -1;
    }
    int i = 0;
    struct list_node_s *curr = *root_list;
    while (curr != NULL){
        if (curr->data == value) {
            printf(" %d is found\n", value);
            return 1;
        }
        curr = curr->next;
        i++;
    }
    printf(" %d is not found\n", value);
    return -1;
}

void Task_enqueue(int task_num, int task_type, int value){

    struct task_node_s* new_node;
    new_node = malloc(sizeof(struct task_node_s));
    new_node->task_num = task_num;
    new_node->task_type = task_type;
    new_node->value = value;
    new_node->next = NULL;

    if (*root_task == NULL) {
        *root_task = new_node;
        return;
    }
    struct task_node_s *curr = *root_task;
    while (curr->next != NULL){
        curr = curr->next;
    }
    curr->next = new_node;
    return;
}

void Task_queue(int n){

    int i;
    int *thread_no = malloc(sizeof(int));
    *thread_no = 0;

    for (i = 0; i < n+1; i++) {
        pthread_mutex_lock(&mutex);
        Task_enqueue( i,               // task num
                      rand() % 3,      // task type
                      rand() % 501     // task data
        );
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
    }

    task_flag = 1;
    pthread_cond_broadcast(&cond);
    free(thread_no);
}

int Task_dequeue(long my_rank, int* task_num_p, int* task_type_p, int* value_p){

    *task_num_p = (*root_task)->task_num;
    *task_type_p = (*root_task)->task_type;
    *value_p = (*root_task)->value;

    struct task_node_s* to_remove = *root_task;
    if ((**root_task).next != NULL){
        *root_task = (**root_task).next;
    } else {
        return -1;
    }
    free(to_remove);

}


void execute_task(int* task_num_p,int* task_type_p,int* value_p){

    printf(" task num %d:", *task_num_p);
    
    if (*task_type_p == 0) {
        Insert(*value_p);
    } 
    else if (*task_type_p == 1) {
        Delete(*value_p);
    } 
    else if (*task_type_p == 2) {
        Search(*value_p);
    }
     
}

void Deallocate_queues(){
    // Deallocating list queue
    struct list_node_s *curr2 = *root_list;
    while (curr2 != NULL){
        struct list_node_s *aux = curr2;
        curr2 = curr2->next;
        free(aux);
    }
    // Deallocating task queue
    struct task_node_s *curr3 = *root_task;
    while (curr3 != NULL){
        struct list_node_s *aux2 = curr2;
        curr3 = curr3->next;
        free(aux2);
    }
}

void end(){

    // displaying final list
    printf("Main: ");
    printf("Final List:\n");
    struct list_node_s *curr = *root_list;
    while (curr != NULL){
        printf("%d ",curr->data);
        curr = curr->next;
    }

    printf("\n");

    // calculating elapsed time
    gettimeofday(&check_point, NULL);
    end_time = (long)check_point.tv_sec * FACTOR + (long)check_point.tv_usec;
    double elapsed_time = (end_time - beginning_time) / (double)FACTOR;
    printf("Elapsed time: %f seconds\n", elapsed_time);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    Deallocate_queues();
    exit(1);
}

void* routine(void* i){
    int count = *(int*)i;

    while (task_flag == 0 || (*root_task)->next  != NULL) {

        int* task_num_p = malloc(sizeof(int));
        int* task_type_p = malloc(sizeof(int));
        int* value_p = malloc(sizeof(int));

        pthread_mutex_lock(&mutex);
        while (task_flag == 0 && (*root_task)->next  == NULL) {
            pthread_cond_wait(&cond, &mutex);
        }

        if (Task_dequeue(0, task_num_p, task_type_p, value_p) == -1 && task_flag == 1) // end of task queue flag
            end();
        printf("thread %d:", count);
        pthread_mutex_unlock(&mutex);

        execute_task(task_num_p, task_type_p, value_p);
        free(task_num_p);
        free(task_type_p);
        free(value_p);

    }
}