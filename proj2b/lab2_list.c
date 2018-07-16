//NAME: Joel George
//EMAIL: joelgeorge03@gmail.com
//ID: 004786402

#include "SortedList.h"
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

char rand_string[1024];
char** key_array;
int element_array_index = 0; 
pthread_mutex_t* locks;
pthread_mutex_t el_arr_index_lock;
volatile int* s_locks;
volatile int s_lock = 0;
char sync_mode = 'n';
long long total_blocked_time = 0;

struct args{
  SortedListElement_t** el_array;
  int it_num;
  int max;
  int list_num;
  SortedListElement_t** list_array;
}argument_holder;

void sigseghandler();
int print_error();
int error_check(int return_val, int success_val);
int process_options(struct option* option_list, int *option_index, int argc, char* argv[], int *thread_num, int *it_num, char** y_mode, int* list_num);
int process_y_mode(char* y_mode, char** y_usage);
void process_usage(char** usage, char** y_usage, int list_num);
SortedList_t* create_list_item(const char* key);
void rand_string_gen(char** holder);
void element_creation(SortedListElement_t** list, int thread_num, int it_num);
void *thread_function(void *arg_ptr);
void thread_creator(int thread_num, int iterations, pthread_t *thread_array, SortedListElement_t** element_array, SortedListElement_t** list, int list_num);
void sublist_creator(int list_num, SortedListElement_t** list);
void list_creation(int list_num, SortedList_t** list_array);
int hash(const char* key);

int main(int argc, char* argv[]){
  //for option parsing
  int option_index = 0;

  //for threads
  int thread_num = 1;
  pthread_t* thread_array;

  //for iterations
  int it_num = 1;

  //for sync
  char* usage = malloc(14*sizeof(char));
  if(!usage){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(1);
  }

  //for yield
  char* y_mode = malloc(4*sizeof(char));
  if(!y_mode){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(1);
  }

  char* y_usage = malloc(5*sizeof(char));
  if(!y_usage){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(1);
  }


  //for lists
  SortedList_t** list_array;
  SortedListElement_t** element_array;
  int list_num = 1;

  //CSV info
  long long run_time_val = 0;
  int op_num = 0;
  long long time_per_op = 0;


  static struct option option_list[] = {
    {"threads", required_argument, 0, 't'},
    {"iterations", required_argument, 0, 'i'},
    {"sync", required_argument, 0, 's'},
    {"yield", required_argument, 0, 'y'},
    {"lists", required_argument, 0, 'l'},
    {0, 0, 0, 0}
  };

  process_options(option_list, &option_index, argc, argv, &thread_num, &it_num, &y_mode, &list_num);
  opt_yield = process_y_mode(y_mode, &y_usage);
  process_usage(&usage, &y_usage, list_num);

  list_array = malloc(list_num*sizeof(SortedList_t*));
  if(!list_array){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(1);
  }
  list_creation(list_num, list_array);

  thread_array = malloc(thread_num*sizeof(pthread_t));
  if(!thread_array){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(1);
  }

  element_array = malloc(thread_num*it_num*sizeof(SortedListElement_t*));
  if(!element_array){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(1);
  }

  element_creation(element_array, thread_num, it_num);

  struct timespec beginning, end;
  clock_gettime(CLOCK_MONOTONIC, &beginning);
  signal(SIGSEGV, sigseghandler);
  thread_creator(thread_num, it_num, thread_array, element_array, list_array, list_num);

  int count = 0;
  for(; count < thread_num; count++){
    pthread_join(thread_array[count], NULL);
  }

  clock_gettime(CLOCK_MONOTONIC, &end);

  run_time_val = end.tv_nsec - beginning.tv_nsec;
  op_num = thread_num*it_num*3;
  time_per_op = run_time_val/op_num;
  if(sync_mode == 'n')
    total_blocked_time = 0;
  long long time_per_lock = (total_blocked_time)/(2+2*it_num+list_num); //TODO: complete
  printf("%s%s%d%s%d%s%d%s%d%s%llu%s%llu%s%llu\n",usage, ",", thread_num, ",", it_num, ",", list_num, ",", op_num, ",", run_time_val, ",", time_per_op, ",", time_per_lock);
  free(y_mode);
  free(y_usage);
  free(usage);
  free(element_array);
  free(thread_array);
  free(list_array);
  if(sync_mode == 'm'){
    int i = 0;
    for(; i < list_num; i++){
      pthread_mutex_destroy(&locks[i]);
    }
    pthread_mutex_destroy(&el_arr_index_lock);
    free(locks);
  }
  exit(0);
}

void sigseghandler(){
  fprintf(stderr, "SIGSEGV was caught - a bad memory access was attempted. \n\n");
  exit(2);
}

int print_error(){
  fprintf(stderr, "Error:%s\n", strerror(errno));
  exit(1);
}

int error_check(int return_val, int success_val){
  if(return_val != success_val){
    print_error();
    exit(1);
  }
  return 1;
}


int process_options(struct option* option_list, int *option_index, int argc, char* argv[], int *thread_num, int *it_num, char** y_mode, int* list_num){
  int current_character;
  while(1){
    current_character = getopt_long(argc, argv, ":t:i:s:y:", option_list, option_index);
    if(current_character == -1)
      return 0;
    switch(current_character){
    case 't':
      *thread_num = (int) strtol(optarg, NULL, 10);
      break;
    case 'i':
      *it_num = (int) strtol(optarg, NULL, 10);
      break;
    case 's':
      sync_mode = optarg[0];
      break;
    case 'y':
      strcpy(*y_mode, optarg);
      break;
    case 'l':
      *list_num = (int) strtol(optarg, NULL, 10);
      break;
    case ':':
      fprintf(stderr, "Arguments --threads, --iterations, and --sync require arguments.\n");
      exit(1);
      break;
    case '?':
      fprintf(stderr, "The only valid arguments are --threads, --iterations, --sync, and --yield.\n");
      exit(1);
      break;
    }
  }
  return 0;
}

int process_y_mode(char* y_mode, char** y_usage){
  char* insert = strchr(y_mode, 'i');
  char* delete = strchr(y_mode, 'd');
  char* lookup = strchr(y_mode, 'l');
  int yield_options = 0;
  if(insert){
    yield_options = yield_options | 0x01;
    strcat(*y_usage, "i\0");
  }
  if(delete){
    yield_options = yield_options | 0x02;
    strcat(*y_usage, "d\0");
  }
  if(lookup){
    yield_options = yield_options | 0x04;
    strcat(*y_usage, "l\0");
  }
  if(!insert && !delete && !lookup){
    strcat(*y_usage, "none");
  }
  return yield_options;
}

void process_usage(char** usage, char** y_usage, int list_num){
  char base[14];
  strcpy(base, "list-");
  strcat(base, *y_usage);
  switch(sync_mode){
  case 'n':
    strcat(base, "-none\0");
    strcpy(*usage, base);
    break;
  case 'm':{
    strcat(base, "-m\0");
    strcpy(*usage, base);
    //TODO: make sure to free up mutex locks
    int err_val = pthread_mutex_init(&el_arr_index_lock, NULL);
    error_check(err_val, 0);
    locks = malloc(list_num*sizeof(pthread_mutex_t));
    if(!locks){
      fprintf(stderr, "Error: %s\n", strerror(errno));
      exit(1);
    }
    int i = 0;
    for(; i < list_num; i++){
      int error_val = pthread_mutex_init(&locks[i], NULL);
      error_check(error_val, 0);
    }
    break;
  }
  case 's':{
    //TODO: free up s_locks
    s_locks = malloc(list_num*sizeof(volatile int));
    if(!s_locks){
      fprintf(stderr, "Error: %s\n", strerror(errno));
      exit(1);
    }
    int i = 0;
    for(; i < list_num; i++){
      s_locks[i] = 0;
    }
    strcat(base, "-s\0");
    strcpy(*usage, base);
    break;
  }
  default:
    fprintf(stderr, "Error: invalid --sync argument. The only valid arguments for --sync are m and s.\n");
    break;
 }
}

SortedList_t* create_list_item(const char* key){
  SortedListElement_t* element = malloc(1*sizeof(SortedListElement_t));
  if(!element){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(1);
  }
  element->key = key;
  return element;
}

void rand_string_gen(char** holder){
  char characters[63] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
			 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
			 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 
			 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
			 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
			 'y', 'z', '\0'};
  int rand_index = 0;
  while(rand_index < 1023){
    int index = (int) (((double)rand()/RAND_MAX)*62);
    if(index >= 0 && index <= 62){
      rand_string[rand_index] = characters[index];
      rand_index++;
    }
  }
  rand_string[1023] = '\0';
  *holder = rand_string;
}

void element_creation(SortedListElement_t** list, int thread_num, int it_num){
  int max = thread_num*it_num;
  char* key;
  key_array = malloc(max*sizeof(char*));
  int index = 0;

  SortedListElement_t* element;
  for(;index < max; index++){
    rand_string_gen(&key);
    key_array[index] = malloc(1024*sizeof(char));
    strcpy(key_array[index], key);
    element = create_list_item(key_array[index]);
    list[index] = element;
  }
}

void *thread_function(void *arg_ptr){

  int iterations = (*((struct args*)arg_ptr)).it_num;
  SortedListElement_t** el_array =  (*((struct args*)arg_ptr)).el_array;
  SortedList_t** list_array = (*((struct args*)arg_ptr)).list_array;
  int list_num = (*((struct args*)arg_ptr)).list_num;
  int max = (*((struct args*)arg_ptr)).max;
  long long thread_total_blocked_time = 0; 
  int i = 0;

  struct timespec beginning, end;
  clock_gettime(CLOCK_MONOTONIC, &beginning);

  if(sync_mode == 'm')
    pthread_mutex_lock(&el_arr_index_lock);
  if(sync_mode == 's')
    while(__sync_lock_test_and_set(&s_lock, 1));
  
  clock_gettime(CLOCK_MONOTONIC, &end);  
  thread_total_blocked_time += end.tv_nsec - beginning.tv_nsec;

  int base_index = element_array_index;
  int max_index_per_thread = base_index + iterations;
  element_array_index+=iterations;

  if(sync_mode == 'm')
    pthread_mutex_unlock(&el_arr_index_lock);
  if(sync_mode == 's')
    __sync_lock_release(&s_lock);


  //release locks

  for(; i < iterations && (base_index + i) < max; i++){
    int list_val = (hash(el_array[base_index + i]->key))%list_num;
    clock_gettime(CLOCK_MONOTONIC, &beginning);
    if(sync_mode == 'm'){
      pthread_mutex_lock(&locks[list_val]);
    }
    if(sync_mode == 's')
      while(__sync_lock_test_and_set(&s_locks[list_val], 1));
    clock_gettime(CLOCK_MONOTONIC, &end);
    thread_total_blocked_time += end.tv_nsec - beginning.tv_nsec;

    SortedList_insert(list_array[list_val], el_array[base_index + i]);
    if(sync_mode == 'm')
      pthread_mutex_unlock(&locks[list_val]);
    if(sync_mode == 's')
      __sync_lock_release(&s_locks[list_val]);
    
  }


  i = 0;
  int list_length = 0;
  for(; i < list_num; i++){
    clock_gettime(CLOCK_MONOTONIC, &beginning);
    if(sync_mode == 'm')
      pthread_mutex_lock(&locks[i]);
    if(sync_mode == 's')
      while(__sync_lock_test_and_set(&s_locks[i], 1));
    clock_gettime(CLOCK_MONOTONIC, &end);
    thread_total_blocked_time += end.tv_nsec - beginning.tv_nsec;

    int sublist_len = SortedList_length(list_array[i]);
    if(sublist_len == -1){
      fprintf(stderr, "Inconsistency found when calculating length!");
      exit(2);
    }
    else
      list_length+=sublist_len;
    if(sync_mode == 'm')
      pthread_mutex_unlock(&locks[i]);
    if(sync_mode == 's')
      __sync_lock_release(&s_locks[i]);
  }
  for(; base_index < max_index_per_thread && base_index < max; base_index++){
    int list_val = (hash(el_array[base_index]->key))%list_num;
    clock_gettime(CLOCK_MONOTONIC, &beginning);
    if(sync_mode == 'm')
      pthread_mutex_lock(&locks[list_val]);
    if(sync_mode == 's')
      while(__sync_lock_test_and_set(&s_locks[list_val], 1)); 
    clock_gettime(CLOCK_MONOTONIC, &end);
    thread_total_blocked_time += end.tv_nsec - beginning.tv_nsec;
    SortedList_t* element = SortedList_lookup(list_array[list_val], key_array[base_index]);
    if(!element){
      fprintf(stderr, "Element not found!");
      exit(2);
    }
    if(SortedList_delete(el_array[base_index])){
      fprintf(stderr, "Inconsistency found when deleting!");
      exit(2);
    }
    if(sync_mode == 'm')
      pthread_mutex_unlock(&locks[list_val]);
    if(sync_mode == 's')
      __sync_lock_release(&s_locks[list_val]);
  }
  clock_gettime(CLOCK_MONOTONIC, &beginning);

  if(sync_mode == 'm')
    pthread_mutex_lock(&el_arr_index_lock);
  if(sync_mode == 's')
    while(__sync_lock_test_and_set(&s_lock, 1));

  clock_gettime(CLOCK_MONOTONIC, &end);
  thread_total_blocked_time += end.tv_nsec - beginning.tv_nsec;
  total_blocked_time+=thread_total_blocked_time;
  
  if(sync_mode == 'm')
    pthread_mutex_unlock(&el_arr_index_lock);
  if(sync_mode == 's')
    __sync_lock_release(&s_lock);


  return NULL;
}

void thread_creator(int thread_num, int iterations, pthread_t *thread_array, SortedListElement_t** element_array, SortedListElement_t** list_array, int list_num){
  int i = 0;

  argument_holder.el_array = element_array;
  argument_holder.it_num = iterations;
  argument_holder.max = thread_num*iterations;
  argument_holder.list_array = list_array;
  argument_holder.list_num = list_num;
 
  int error_val;

  for(; i < thread_num; i++){
    error_val = pthread_create(&thread_array[i], NULL, thread_function, (void*) &argument_holder);
    error_check(error_val, 0);
  }
}

void list_creation(int list_num, SortedList_t** list_array){
  int i = 0;
  for(; i < list_num; i++){
    SortedList_t* list = malloc(sizeof(SortedList_t));
    list->key = NULL;
    list->next = list;
    list->prev = list;
    list_array[i] = list;
  }
  return; 
}

int hash(const char* key){
  int hash_value = 0;
  int index = 0;
  int current_c = key[index];
  while(current_c != '\0'){
    hash_value += current_c;
    index++;
    current_c = key[index];
  }
  return hash_value; 
}
