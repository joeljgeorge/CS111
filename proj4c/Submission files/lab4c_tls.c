//NAME: Joel George
//EMAIL: joelgeorge03@gmail.com
//ID: 004786402

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <poll.h>
#include <sys/wait.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#ifdef DUMMY
#define UNUSED(x) (void)(x)
typedef int mraa_gpio_context;
typedef int mraa_aio_context;
mraa_gpio_context mraa_gpio_init(int p) {(void) (p); return 0;}
#define MRAA_GPIO_IN (0)
void mraa_gpio_dir(mraa_gpio_context c, int d) {(void) (c); (void) (d);};
int mraa_gpio_read(mraa_gpio_context c) {(void) c; return 0;}
void mraa_gpio_close(mraa_gpio_context c) {(void) (c);}
int mraa_aio_init(mraa_aio_context c) {(void) (c); return 0;}
int mraa_aio_read(mraa_gpio_context c) {(void) c; return 500;}
void mraa_aio_close(mraa_gpio_context c) {(void) (c);}
#else
#include <mraa.h>
#include <mraa/aio.h>
#include <mraa/gpio.h>
#endif

sig_atomic_t volatile run_flag = 1;
struct sockaddr_in server_address;
struct hostent* server;

const int B = 4275;
const int R0 = 100000;
char temp_type = 'F';
int period = 1;
int start = 1;
int off = 0;
char* log_file = NULL;
char* host_name = NULL;
char* id_num = NULL;
int port_num = 0;
int sockfd = 0;
int ofd = 0;
//taken from tutorial: http://fm4dd.com/openssl/sslconnect.htm
X509 *cert = NULL;
X509_NAME *certname = NULL;
const SSL_METHOD *method;
SSL_CTX *ctx;
SSL *ssl;
BIO *certbio = NULL;
BIO *outbio = NULL;

void read_input(char** buffer, int* read_size);
void cmd_interpreter(char* cmd, int cmd_length);
void socket_connection(struct pollfd fds[]);
void tls_connection(struct pollfd fds[]);

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
   else if(true_len > 7){
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
   else if(true_len == 4){
    strncpy(substring, cmd, 4);
    substring[4] = '\0';
    if(!strcmp(substring, "STOP")){
      start = 0;
    }
  }
   else if(true_len == 5){
    strncpy(substring, cmd, 5);
    substring[5] = '\0';
    if(!strcmp(substring, "START")){
      start = 1;
    }
  }
   else if(true_len == 3){
    strncpy(substring, cmd, 3);
    substring[3] = '\0';
    if(!strcmp(substring, "OFF")){
      off = 1;
    }
  }
   else{
     strncpy(substring, cmd, 3);
     substring[3] = '\0';
     if(!strcmp(substring, "LOG")){
       strncpy(substring, cmd+4, true_len - 4);
       substring[true_len-4] = '\0';
       write(ofd, substring, true_len-4);
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
void write_output(int fds, char* time_string, char* snum){
  char report_string[20];
  char shutdown_string[8] = "SHUTDOWN";
  int i = 0;
  for(; i < 8; i++){
    report_string[i] = time_string[i];
  }
  int j = 0;
  report_string[i] = 32;
  i++;
  if(start){
    if(!off){
      for(; i < 14; i++, j++)
	{
	  report_string[i] = snum[j];
	}
      report_string[i] = '\n';
      if(fds == ofd)
	write(ofd, report_string, i+1);
      else{
	SSL_write(ssl, report_string, i+1);
      }
    }
    else{
      for(; i < 17; i++, j++)
	{
	  report_string[i] = shutdown_string[j];
	}
      report_string[i] = '\n';
      if(fds == ofd){
	write(ofd, report_string, i+1);
	exit(0);
      }
      else{
	SSL_write(ssl, report_string, i+1);
	SSL_shutdown(ssl); 
	SSL_free(ssl);
	X509_free(cert);
	SSL_CTX_free(ctx);
	close(fds);
      }
    }
  }
  else{
    if(off){
      for(; i < 17; i++, j++)
        {
          report_string[i] = shutdown_string[j];
        }
      report_string[i] = '\n';
      write(fds, report_string, i+1);
      if(fds == ofd){
	write(ofd, report_string, i+1);
	exit(0);
      }
      else{
	SSL_write(ssl, report_string, i+1);
	SSL_shutdown(ssl);
	SSL_free(ssl);
	X509_free(cert);
	SSL_CTX_free(ctx);
	close(fds);
      }
    }
  }
}
void temp_reading(mraa_aio_context temperature, struct pollfd fds[]){
  int temp_measurement;
  char* buffer;
  tls_connection(fds);
  while(run_flag){
    temp_measurement = mraa_aio_read(temperature);
    
    float R = (1023.0/temp_measurement) - 1.0;
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
    char *snum = malloc(5*sizeof(char));
    sprintf(snum, "%f", temp_val);
    write_output(sockfd, time_string, snum);
    write_output(ofd, time_string, snum);

    free(time_string);
    free(snum);
    delay();

    int poll_err = poll(fds,1,0);
    if(poll_err < 0){
      fprintf(stderr, "Error when polling.");
      exit(1);
    }
    if(fds[0].revents & POLLIN){
      int read_size = 0;
      read_input(&buffer, &read_size);
      if(log_file){
	write(ofd, buffer, read_size);//Write to the log file
      }
      cmd_interpreter(buffer, read_size);
    }
  }
}

void process_arg(int argc, char *argv[], struct option options[]){
  int current_option;
  int option_index = 0;
  int present_port_num = 0;
  int present_id_num = 0;
  int present_host = 0;
  int present_log = 0;
  
  //searching for port number
  int i = 1;
  for(;i < argc; i++){
    if(argv[i][0]!='-'){
      present_port_num = 1;
      port_num = (int)strtol(argv[i], NULL, 0);
    }
  }
  
  while(1){
    current_option = getopt_long(argc, argv, ":p:s:l:h:i:", options, &option_index);
    if(current_option == -1){
      if(!present_port_num || !present_log || !present_id_num ||
	 !present_host){
	fprintf(stderr, "Missing mandatory arguments. Must include a port number, a log file, an ID number, and a host address.\n");
	exit(1);
      }
      return;
    }
    switch(current_option){
    case 'p':
      period = (int) strtol(optarg, NULL, 0);
      break;
    case 's':
      temp_type = optarg[0];
      break;
    case 'l':
      present_log = 1;
      log_file = malloc(500*sizeof(char));
      strcpy(log_file, optarg);
      break;
    case 'h':
      present_host = 1;
      host_name = malloc(500*sizeof(char));
      strcpy(host_name, optarg);
      break;
    case 'i':
      present_id_num = 1;
      i = 0;
      int num_digits = 9;
      //checking that id num is 9 digits long
      for(; i < num_digits; i++){
	if(optarg[i] == '\0'){
	  fprintf(stderr, "ID number must have 9 digits.\n");
	  exit(1);
	}
      }
      id_num = malloc(9*sizeof(char));
      strcpy(id_num, optarg);
      break;
    case ':':
      fprintf(stderr, "Arguments --period, --scale, and --log require arguments.");
      exit(1);
      break;
    case '?':
      fprintf(stderr, "The only valid arguments are --period, --scale, and --log.\n");
      exit(1);
      break;
    }
  }
}

void redirect_input(char* log_file){
  if(log_file){
    ofd = open(log_file, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if(ofd < 0){
      fprintf(stderr, "Error opening log file.");
      exit(1);
    }
  }
}

void read_input(char** buffer, int* read_size){
  *buffer = malloc(2000*sizeof(char));
  int buffer_size = 512;
  int read_status = 1;
  read_status = SSL_read(ssl, *buffer+*read_size, buffer_size);    
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

void socket_connection(struct pollfd fds[]){
//largely based on source code from:
//http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/client.c
  sockfd = 0;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0){
    fprintf(stderr, "Error when opening the socket.\n");
    exit(1);
  }
  
  fds[0].fd = sockfd;
  fds[0].events = POLLIN | POLLHUP | POLLERR;
  server = gethostbyname(host_name);
  
  if(!server){
    fprintf(stderr, "Error: no such host\n");
    exit(1);
  }

  bzero((char*)&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  bcopy((char*)server->h_addr,
	(char*)&server_address.sin_addr.s_addr,
	server->h_length);
  
  server_address.sin_port = htons(port_num);
  int connect_result = connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
  if(connect_result < 0){
    fprintf(stderr, "Error upon connection.\n");
    exit(1);
  }
}

void tls_connection(struct pollfd fds[]){
  socket_connection(fds);
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();  
  SSL_library_init();

  method = TLSv1_client_method();
  ctx = SSL_CTX_new(method);

  if (ctx == NULL){
    fprintf(stderr, "Unable to create a new SSL context structure.\n");
    exit(2);
  }

  ssl = SSL_new(ctx);

  SSL_set_fd(ssl, sockfd);//TODO: check return value
  if(SSL_connect(ssl) != 1){
    fprintf(stderr, "Error: Could not build a SSL session to: %s.\n", host_name);
    exit(2);
  }
  //send ID msg to server
  char id_msg[13];
  strcpy(id_msg, "ID=");
  strcpy(id_msg+3, id_num);
  id_msg[12] = '\n';
  SSL_write(ssl, id_msg, 13);
  write(ofd, id_msg, 13);
}
int main(int argc, char *argv[]){
  //Setting up temperature reading variable
  mraa_aio_context temperature;
  temperature = mraa_aio_init(1);
 
  //Options
  static struct option options[] = {
    {"period", required_argument, 0, 'p'},
    {"scale", required_argument, 0, 's'},
    {"log", required_argument, 0, 'l'},
    {"host", required_argument, 0, 'h'},
    {"id", required_argument, 0, 'i'},
    {0, 0, 0, 0}
  };

  process_arg(argc, argv, options);
  redirect_input(log_file);
  signal(SIGINT, do_when_interrupted);

  struct pollfd fds[1];
  
  temp_reading(temperature, fds);
  mraa_aio_close(temperature);
  
  return 0;
}
