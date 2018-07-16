#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <mraa/aio.h>
#include <mraa/gpio.h>
#include <poll.h>
#include <sys/wait.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>

sig_atomic_t volatile run_flag = 1;

const int B = 4275;
const int R0 = 100000;
char temp_type = 'F';
int period = 1;
int start = 1;
int off = 0;
char* log_file = NULL;

void read_input(char** buffer, int* read_size);
void cmd_interpreter(char* cmd, int cmd_length);

void delay(){
  int milli_seconds = period*1000000;
  clock_t start = clock();
  while(clock() < (start + milli_seconds));
}

void do_when_interrupted(int sig){
  
  if(sig == SIGINT)
    run_flag = 0;
}

void convert_to_fahr(float* temp_val){
  *temp_val = (*temp_val)*(1.8)+32;
}

void process_cmd(char* cmd, int true_len){
  char* substring = malloc((true_len)*sizeof(char));
   if(true_len == 7){//finding SCALE value
    strncpy(substring, cmd, 6);
    substring[6] = '\0';
    if(!strcmp(substring, "SCALE=")){
      if(cmd[6] == 'F' || cmd[6] == 'C'){
	temp_type = cmd[6];
	return;
      }
    }
  }
  if(true_len > 7){
    strncpy(substring, cmd, 7);
    substring[7] = '\0';
    if(!strcmp(substring, "PERIOD=")){
      int i = 7;
      for(; i < true_len; i++){
	if(!isdigit(cmd[i])){
	  return;
	}
      }
      strncpy(substring, cmd+7, true_len - 7);
      int period_val = (int) strtol(substring, NULL, 10);
      period = period_val;
    }
  }
  if(true_len == 4){
    strncpy(substring, cmd, 4);
    substring[4] = '\0';
    if(!strcmp(substring, "STOP")){
      start = 0;
    }
  }
  if(true_len == 5){
    strncpy(substring, cmd, 5);
    substring[5] = '\0';
    if(!strcmp(substring, "START")){
      start = 1;
    }
  }
  if(true_len == 3){
    strncpy(substring, cmd, 3);
    substring[3] = '\0';
    if(!strcmp(substring, "OFF")){
      off = 1;
    }
  }
  free(substring);
}

void cmd_interpreter(char* cmd, int cmd_length){
  char* substring = malloc(cmd_length*sizeof(char));
  int i = 0;
  int true_len = 0;
  int index = 0;
  for(; i < cmd_length; i++){
    substring[index] = cmd[i];
    if(substring[index] == '\n'){
      substring[index] = '\0';
      true_len = index;
      index = 0;
      process_cmd(substring, true_len);      
    }
    else
      index++;
  }
  free(substring);

}

void temp_reading(mraa_aio_context temperature, struct pollfd fds[], mraa_gpio_context button){
  int temp_reading;
  int button_val;
  char* buffer;
  while(run_flag){
    temp_reading = mraa_aio_read(temperature);
    button_val = mraa_gpio_read(button);
    if(button_val){
      off = 1;
    }
    float R = (1023.0/temp_reading) - 1.0;
    R = R0*R;
    float temp_val = (1.0/(log(R/R0)/B+1/298.15))-273.15;
    
    if(temp_type == 'F'){
      convert_to_fahr(&temp_val);
    }
    //generating timestamp
    char* time_string = malloc(9*sizeof(char));
    time_t rawtime;
    struct tm *time_holder;
    time(&rawtime);
    time_holder = localtime(&rawtime);
    strftime(time_string, 9, "%X", time_holder);
    char *snum = malloc(3*sizeof(char));
    sprintf(snum, "%f", temp_val);
    if(start){
      write(1, time_string, 8);
      write(1, " ", 1);
      if(!off){
	write(1, snum, 4);
	write(1, "\n", 1);
      }
      else{
	write(1, "SHUTDOWN\n", 9);
	exit(0);
      }	
    }
    else{
      if(off){
	write(1, time_string, 9);
	write(1, " ", 1);
	write(1, "SHUTDOWN\n", 9);
	exit(0);
      }
    }
    free(time_string);
    free(snum);
    delay();

    poll(fds,1,0);
    if(fds[0].revents & POLLIN){
      int read_size = 0;
      read_input(&buffer, &read_size);
      if(log_file)
	write(1, buffer, read_size);
      cmd_interpreter(buffer, read_size);
    }
  }
}

void process_arg(int argc, char *argv[], struct option options[]){
  int current_option;
  int option_index = 0;
  while(1){
    current_option = getopt_long(argc, argv, ":p:s:l:", options, &option_index);
    if(current_option == -1)
      return;
    switch(current_option){
    case 'p':
      period = (int) strtol(optarg, NULL, 0);
      break;
    case 's':
      temp_type = optarg[0];
      break;
    case 'l':
      log_file = malloc(500*sizeof(char));
      strcpy(log_file, optarg);
      break;
    case ':':
      fprintf(stderr, "Arguments --period, --scale, and --log require arguments.");
      exit(1);
      break;
    case '?':
      fprintf(stderr, "The only valid arguments are --period, --scale, and --log");
      exit(1);
      break;
    }
  }
}

void redirect_input(char* log_file){
  if(log_file){
    int ofd = creat(log_file, O_RDWR);
    if(ofd >= 0){
      close(1);
      dup(ofd);
      close(ofd);
    }
  }
}

void read_input(char** buffer, int* read_size){
  *buffer = malloc(2000*sizeof(char));
  int buffer_size = 256;
  int read_status = 1;
  read_status = read(0, *buffer+*read_size, buffer_size);    
  if(read_status < 0){
    fprintf(stderr, "Error in reading stdin: %s\n", strerror(errno));
    return;
  }
  else if(read_status == 0){
    return;
  }
  else{
    *read_size+=read_status;
  }
}



int main(int argc, char *argv[]){
  //Setting up temperature reading variable
  mraa_aio_context temperature;
  temperature = mraa_aio_init(1);

  mraa_gpio_context button;
  button = mraa_gpio_init(62);
  mraa_gpio_dir(button, MRAA_GPIO_IN);
  
  //For polling
  struct pollfd fds[1];
  fds[0].fd = 0;
  fds[0].events = POLLIN | POLLHUP | POLLERR;
  
  //Options
  static struct option options[] = {
    {"period", required_argument, 0, 'p'},
    {"scale", required_argument, 0, 's'},
    {"log", required_argument, 0, 'l'},
    {0, 0, 0, 0}
  };
  
  process_arg(argc, argv, options);
  redirect_input(log_file);	       
  signal(SIGINT, do_when_interrupted);
  temp_reading(temperature, fds, button);

  mraa_aio_close(temperature);
  mraa_gpio_close(button);
  
  return 0;
}
