#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<ucontext.h>
#include<sys/mman.h>

#define ADDR 0x5300000
#define SIZE 0x100000
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


void restore_memory();
unsigned long long expo(int power);
unsigned long long int conv_to_long(char *conv_str, int len);


ucontext_t ucp;


char chkpt_img[1000];
int fd, fd_maps;
char input_c;

int main(int argc, char *argv[]) {
       
  if (argc == 1) {
    printf("Not valid number of arguments");
  }
  void *ptr, *esp;

  sprintf(chkpt_img, "%s", argv[1]);
  
  
  ptr = mmap((void *)ADDR, SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  esp = ptr + SIZE;
  asm volatile ("mov %0,%%rsp" : : "g" (esp) : "memory");
  
  restore_memory();
  return 0;
}

void restore_memory() {

  int errno;
  unsigned long long length;

  struct MemRegion *ptr_mr = malloc(sizeof(struct MemRegion));
  
  
  fd_maps = open("/proc/self/maps",O_RDONLY);
  if (fd_maps < 0) exit(1);

  int read_op = 0;
  int curr_char = 0;
  int chars_read = 0;
  char data[3000];
  while (1) {
    int read_op = read(fd_maps, &data[curr_char], 1);
    chars_read += read_op;
    curr_char += 1;
    if (read_op <= 0) break;
  }

  close(fd_maps);
  int lines_in_maps = 0;
  int i;
  for (i = 0; i < chars_read; i++) {
    
    if (data[i] == '\n') { 
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


      ptr_mr->startaddr = (void *)conv_to_long(temp, field);

      for (f = start + field + 1, z = 0; z < field; f++, z++) {
        temp[z] = data[f];
      }

      ptr_mr->endaddr = (void *)conv_to_long(temp, field);
      
      
      perms = start + field + 1 + field + 1;
      if (data[perms] == 'r') {
        ptr_mr->isReadable = 1;
      } else {
        ptr_mr->isReadable = 0;
      }
    
      if (data[perms + 1] == 'w') {
        ptr_mr->isWriteable = 1;
      } else {
        ptr_mr->isWriteable = 0;
      }
      
      if (data[perms + 2] == 'x') {
        ptr_mr->isExecutable = 1;
      } else {
        ptr_mr->isExecutable = 0;
      }
      char temp_name[150] = "";
      
      int cnt = 0;  
      data_pos = start + field + field + 2 + 5;
      for (f = data_pos, z = 0; data[f] != '\n';f++) {
        temp_name[z++] = data[f];
        cnt++;
 
      }
  
      temp_name[z] = '\0';
      
      int new_i = data_pos + cnt;
      i = new_i;
      strcpy(ptr_mr->name, temp_name);

      ptr_mr->size = ptr_mr->endaddr - ptr_mr->startaddr;

      if (strstr(ptr_mr->name,"[stack]") != NULL) break;
      
    }
      
    
    if (data[i] == '\n') {
        start = i + 1;

    }
  
  } //loop ends here
  
  munmap(ptr_mr->startaddr, ptr_mr->size);
  
  struct MemRegion ptr_mr1;
  
  fd = open(chkpt_img, O_RDONLY);
  if (fd < 0) {
    exit(1);
  }
 
  read(fd, &ucp, sizeof(ucontext_t));  
  while (read(fd, &ptr_mr1, sizeof(ptr_mr1)) > 0) {
    
    length = ptr_mr1.endaddr - ptr_mr1.startaddr;
       
    void *mapped = mmap(ptr_mr1.startaddr, length, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE, -1, 0);
    
    if (mapped == (void *)-1) {
      printf("Error when mapping memory region %p %p (error:%s)\n", ptr_mr1.startaddr, ptr_mr1.endaddr, strerror(errno));
      break;
    }
    
    
    read(fd, ptr_mr1.startaddr, length);
    
    int permissions = 0;

    if (ptr_mr1.isReadable) permissions |= PROT_READ;
    if (ptr_mr1.isWriteable) permissions |= PROT_WRITE;
    if (ptr_mr1.isExecutable) permissions |= PROT_EXEC;
 
    int mprot_res = mprotect(ptr_mr1.startaddr, length, permissions);
 
    if (mprot_res < 0) {
      printf("Error when setting protection on memory region %p %p " "(error:%s)\n", ptr_mr1.startaddr, ptr_mr1.endaddr, strerror(errno));
      break;
    }
    
    
  }
  close(fd);
  
  int retval;

    
  setcontext(&ucp);
  
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
