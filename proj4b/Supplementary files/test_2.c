#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <mraa/aio.h>

sig_atomic_t volatile run_flag = 1;

const int B = 4275;
const int R0 = 100000;

void do_when_interrupted(int sig){

  if(sig == SIGINT)
    run_flag = 0;
}

int main(){
  mraa_aio_context temperature;
  int16_t value;

  temperature = mraa_aio_init(1);

  signal(SIGINT, do_when_interrupted);
  
  while(run_flag){
    value = mraa_aio_read(temperature);
    float R = 1023.0/(value - 1.0);
    float temp_val = (1.0/(log(R/R0)/(B+1)/298.15))-273.15;
    printf("%d\n", temp_val);
    usleep(100000);
  }
  mraa_aio_close(temperature);
  return 0;
}
