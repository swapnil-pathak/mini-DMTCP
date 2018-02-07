#include <signal.h>
#include <unistd.h>
#include <ucontext.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define BASE_16 16

struct MemRegion
{
  void *startaddr;
  void *endaddr;
  int isReadable;
  int isWriteable;
  int isExecutable;
  long size;
  char name[150];
};

int flag = 0;
unsigned long long expo(int power);

unsigned long long int conv_to_long(char *conv_str, int len);
void create_mychkpt();
void myhandler(int signl);

__attribute__((constructor)) void constructor() {
  signal(SIGUSR2, myhandler);
}

void myhandler(int signl) {
  signal(signl, SIG_DFL);
  
  create_mychkpt();
  return;	
}

void create_mychkpt() {
  ucontext_t ucp;
  getcontext(&ucp);
  struct MemRegion *memr = malloc(sizeof(struct MemRegion));
  char data[3000];
  int curr_char = 0;
  int chars_read = 0;
  int lines_in_maps = 0;
  int fd_maps = open("/proc/self/maps", O_RDONLY);

  while (1) {
    int read_op = read(fd_maps, &data[curr_char], 1);
    chars_read += read_op;
    curr_char += 1;
    if (read_op <= 0) break; //compare with -1
  }
  close(fd_maps);
  int fd_chkpt = open("myckpt", O_CREAT | O_WRONLY | O_APPEND, 0666);
  if(flag != 0) {
    return;
  } 
  flag = 1;
  
  
  int i;
  write(fd_chkpt, &ucp, sizeof(ucp));
  
  for (i = 0; i < chars_read; i++) {
    if (data[i] == '\n') { //compare with \0 and -1
      lines_in_maps++;
    }
  }
  

  
  int field = 0;
  int start = 0;
  char temp[100];
  int perms = 0;
  int data_pos = 0;

  for (i = 0; i < chars_read; i++) {
  
  
  
    if (data[i] == '-') {
      field = i - start;
      int f,z;
      for (f = start, z = 0; z < field; f++, z++) {
        temp[z] = data[f];
      }
      memr->startaddr = (void *)conv_to_long(temp, field);
      for (f = start + field + 1, z = 0; z < field; f++, z++) {
        temp[z] = data[f];
      }
      memr->endaddr = (void *)conv_to_long(temp, field);
      perms = start + field + 1 + field + 1;
      if (data[perms] == 'r') {
        memr->isReadable = 1;
      } else {
        memr->isReadable = 0;
      }

      
      if (data[perms + 1] == 'w') {
        memr->isWriteable = 1;
      } else {
        memr->isWriteable = 0;
      }
      
      if (data[perms + 2] == 'x') {
        memr->isExecutable = 1;
      } else {
        memr->isExecutable = 0;
      }

      char temp_name[150] = "";
        
      data_pos = start + field + field + 2 + 5;
      int cnt = 0;
      for (f = data_pos, z = 0; data[f] != '\n';f++) {
        temp_name[z++] = data[f];
        cnt++;
      }
      temp[z] = '\0';
      int new_i = data_pos + cnt;
   
      i = new_i ;
   
      strcpy(memr->name, temp_name);
    
      memr->size = memr->endaddr - memr->startaddr;
      
      if (memr->isReadable) {
      
        if(strstr(memr->name,"vsyscall") == NULL && strstr(memr->name,"vdso") == NULL && strstr(memr->name,"vvar") == NULL) {
        

          write(fd_chkpt, memr, sizeof(struct MemRegion));      
          write(fd_chkpt, memr->startaddr, memr->size);
        }
      }



      
    }
    
    if (data[i] == '\n') {
        start = i + 1;

    }
    
  }

  int op = close(fd_chkpt);
  if (op < 0) {
    printf("abc");
  }
  printf("Image created\n");
}

unsigned long long int conv_to_long(char *conv_str, int len) {
  unsigned long long retval = 0;
  int convchars = len;
  int i;
  for (i = 0; i < convchars ; i++) {
    char this_char = conv_str[i];
    unsigned long long x = 0;
    int power = convchars - 1 - i;
    if (this_char - '0' < 10)
      x = this_char - '0';
    else
      x = 10 + (this_char - 'a');
    retval += x * expo(power);
  }
  return retval;

}

unsigned long long expo(int power) {
  unsigned long long rethtl = 1;
  while (power > 0) {
    --power;
    rethtl = rethtl * BASE_16;
  }
  return rethtl;
}
