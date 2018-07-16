//NAME: Joel George
//EMAIL: joelgeorge03@gmail.com
//ID: 004786402

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <poll.h>
#include <sys/wait.h>

struct termios former_attributes;
struct termios new_attributes;


int read_normal(struct termios former_attributes);
int read_shell(int *shell_input);
int read_terminal(int* terminal_input, int* shell_input, int fds, pid_t c_pid);

int main(int argc, char *argv[]){
  //Variables for option parsing
  static int shell_arg = 0;
  int current_option = 0;
  int option_index = 0;
  //
  //For forking
  pid_t fork_status = 0;
  int child_status;
  //
  //For piping 
  int shell_input[2];//shell_input[0] is for reading, [1] is for writing
  int terminal_input[2];
  //
  //For polling
  struct pollfd fds[2];
  fds[0].fd = 0;
  fds[0].events = POLLIN | POLLHUP | POLLERR;


  //
  //Setting terminal
  tcgetattr(0, &former_attributes);
  tcgetattr(0, &new_attributes);
  new_attributes.c_iflag = ISTRIP;
  new_attributes.c_oflag = 0;
  new_attributes.c_lflag = 0;
  tcsetattr(0, TCSANOW, &new_attributes);
  //

  //Option parsing
  static struct option options_list[] = {
    {"shell", no_argument, &shell_arg, 1},
    {0, 0, 0, 0}
  };

  while(1){
    current_option = getopt_long(argc, argv, "", options_list, &option_index);
    if(current_option == '?'){
      fprintf(stderr, "%s", "The only valid argument is --shell. Do not give --shell its own argument.");
      tcsetattr(0, TCSANOW, &former_attributes);
      exit(1);
    }
    break;
  }
  //

  //If shell argument...
  if(shell_arg){
    int pipe_err = pipe(shell_input);
    if(pipe_err == -1){
      fprintf(stderr, "Error when creating pipe: %s\n", strerror(errno));
      tcsetattr(0, TCSANOW, &former_attributes);
      exit(1);
    }
    
    pipe_err = pipe(terminal_input);
    if(pipe_err == -1){
      fprintf(stderr, "Error when creating pipe: %s\n", strerror(errno));
      tcsetattr(0, TCSANOW, &former_attributes);
      exit(1);
    }

    fork_status = fork();
    if(fork_status == -1){
      fprintf(stderr, "Error when forking: %s\n", strerror(errno));
      tcsetattr(0, TCSANOW, &former_attributes);
      exit(1);
    }
    
    switch(fork_status){
    case 0://child process
      {
      int close_err = close(shell_input[1]);//closes writing side of shell pipe
      if(close_err == -1){
	fprintf(stderr, "Error when closing write side of shell input pipe from shell child process: %s\n", strerror(errno));
	tcsetattr(0, TCSANOW, &former_attributes);
	exit(1);
      }
      
      close_err = close(terminal_input[0]);//closes reading side of terminal pipe
      if(close_err == -1){
        fprintf(stderr, "Error when closing read side of terminal input pipe from shell child process: %s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }


      close_err = close(0);//close stdin
      if(close_err == -1){
        fprintf(stderr, "Error when closing stdin from shell child process: %s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }

      int dup_err = dup2(shell_input[0], 0);
      if(dup_err == -1){
	fprintf(stderr, "Error when duplicating read side of pipe from shell child process: %s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }
      
      close_err = close(shell_input[0]);
      if(close_err == -1){
        fprintf(stderr, "Error when closing read side of pipe from shell child process: %s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }

      close_err = close(1);//close stdout
      if(close_err == -1){
        fprintf(stderr, "Error when closing stdout from shell child process: %s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }

      dup_err = dup2(terminal_input[1], 1);
      if(dup_err == -1){
        fprintf(stderr, "Error when duplicating write side of pipe from shell child process: %s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }

      close_err = close(2);//close stderror
      if(close_err == -1){
        fprintf(stderr, "Error when closing stderror from shell child process: %s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }
      
      dup_err = dup2(terminal_input[1], 2);
      if(dup_err == -1){
        fprintf(stderr, "Error when duplicating write side of pipe from shell child process: %s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }
      
      close_err = close(terminal_input[1]);
      if(close_err == -1){
        fprintf(stderr, "Error when closing write side of pipe from shell child process: %s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }

      int exec_err = execl("/bin/bash", "bash", NULL);
      if(exec_err == -1){
	fprintf(stderr, "Error when calling exec: %s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }

      break;
      }
    default://parent process
      {
      int close_err = close(shell_input[0]);//closes input side of pipe
        if(close_err == -1){
        fprintf(stderr, "Error when closing input side of pipe from parent process:%s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }
      close_err = close(terminal_input[1]);//closes output side of pipe
        if(close_err == -1){
        fprintf(stderr, "Error when closing output side of pipe from parent process:%s\n", strerror(errno));
        tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
      }
	
      fds[1].fd = terminal_input[0];
      fds[1].events = POLLIN | POLLHUP | POLLERR;

      while(1){
	int poll_err = poll(fds, 2, 0);
	if(poll_err == -1){
	  fprintf(stderr, "Error when polling inputs: %s\n", strerror(errno));
	tcsetattr(0, TCSANOW, &former_attributes);
        exit(1);
	}
	
	int i = 0;
	while( i < 2){
	  if(fds[i].revents & POLLIN){
	    read_terminal(terminal_input, shell_input, fds[i].fd, fork_status);
	  }
	  if(fds[i].revents & POLLHUP){
	    int waiting_err = waitpid(fork_status, &child_status, 0);
	    if(waiting_err == -1){
	      fprintf(stderr, "Error when waiting for child process: %s\n", strerror(errno));
	      tcsetattr(0, TCSANOW, &former_attributes);
	      exit(1);
	    }
	    
	    int low_order = child_status & 0x007f;
	    int high_order = ((child_status>>8) & 0x00ff);
	    
	    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d", low_order, high_order);
            tcsetattr(0, TCSANOW, &former_attributes);
            exit(0);	      
	  }
	  if(fds[i].revents & POLLERR){
            int waiting_err = waitpid(fork_status, &child_status, 0);
            if(waiting_err == -1){
              fprintf(stderr, "Error when waiting for child process: %s\n", strerror(errno));
              tcsetattr(0, TCSANOW, &former_attributes);
              exit(1);
            }

            int low_order = child_status & 0x007f;
            int high_order = ((child_status>>8) & 0x00ff);

            fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d", low_order, high_order);
            tcsetattr(0, TCSANOW, &former_attributes);
            exit(0);
	  }
	  i++; 
	}
      }
      }
    }
  }
  //
  read_normal(former_attributes);
  exit(0);
}

int read_terminal(int* terminal_input, int* shell_input, int fds, pid_t c_pid){
  char buffer[256];
  int buffer_size = 256;
  int read_status;
  int write_fail_check;
  int send_to_shell = 0;

    read_status = read(fds, buffer, buffer_size);
    if(read_status < 0){
      fprintf(stderr, "Error in reading input from keyboard: %s\n ", strerror(errno));
      exit(1);
    }
    if(read_status == 0){
      int close_err = close(terminal_input[1]);
      if(close_err == -1){
	fprintf(stderr, "Error when closing write side of pipe from terminal process: %s\n", strerror(errno));
	tcsetattr(0, TCSANOW, &former_attributes);
        exit(0);
      }
      return 0;
    }
    int i=0;
    while(i < read_status){
      switch(buffer[i]){
      case 3:{
	int kill_err = kill(c_pid, SIGINT);
	if(kill_err == -1){
	  fprintf(stderr, "Error while sending kill signal to shell process: %s\n", strerror(errno));
	  tcsetattr(0, TCSANOW, &former_attributes);
          exit(0);
	}
	break;
      }
      case 4:{
	int close_err = close(shell_input[1]);
	if(close_err == -1){
	  fprintf(stderr, "Error while closing write side of pipe from terminal process: %s\n", strerror(errno));
	  tcsetattr(0, TCSANOW, &former_attributes);
          exit(0);
	}
	return 0;
      }
      case 10:
      case 13:
	write_fail_check = write(1, "\r\n", 2);
	if(fds == 0)
	  send_to_shell = write(shell_input[1], "\n", 1);
	break;
      default:
	write_fail_check = write(1, &buffer[i], 1);
	if(fds == 0)
	  send_to_shell = write(shell_input[1], &buffer[i], 1);
	break;
      }
      if(write_fail_check == -1)
      {
	fprintf(stderr, "Error when writing to standard output: %s\n", strerror(errno));
	exit(1);
      }
      if(send_to_shell == -1){
	fprintf(stderr, "Error when writing to shell: %s\n", strerror(errno));
	exit(1);
      }
      i++;
    }
    return 0;
}

int read_normal(struct termios former_attributes){
  char buffer[256];
  int buffer_size = 256;
  while(1){
    //TODO: determine where to read from depending on mode
  int read_status = read(0, buffer, buffer_size);
  int write_fail_check;
    if(read_status < 0){
      fprintf(stderr, "Error in reading input: %s\n ", strerror(errno));
      exit(1);
    }
    else if(read_status == 0){
      exit(0);
    }

    int i = 0;
    while(i < read_status){
      switch(buffer[i]){
      case 4:
	//write(1, "^D", 2);
	tcsetattr(0, TCSANOW, &former_attributes);
        exit(0);
      case 10:
      case 13:
	write_fail_check = write(1, "\r\n", 2);
	break;
      default:
	write_fail_check = write(1, &buffer[i], 1);
	break;
      }
      if(write_fail_check == -1)
      {
	fprintf(stderr, "Error when writing to standard output: %s\n", strerror(errno));
	exit(1);
      }
      i++;
    }
  }
}

