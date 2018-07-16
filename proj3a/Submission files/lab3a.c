//NAME: Joel George
//EMAIL: joelgeorge03@gmail.com
//ID: 004786402
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include "ext2_fs.h"

int fds;
char* file_name; 
int s_inodes_count;
int s_blocks;
int s_log_block_size;
int s_block_size;
int s_inode_size;
int s_blocks_p_group;
int s_inodes_p_group;
int s_nr_inode;
int inode_table;

void doubly_indirect_parser(int offset, int* logical_block_offset, int parent_node_num, int input_array, int directory_parsing);
void triply_indirect_parser(int offset, int* logical_block_offset, int parent_node_num, int directory_parsing);

void data_parser(char* sblock_info, int offset, int* final_val, int length){
  int i = offset;
  int iterator = i;
  *final_val = 0;
  int multiplier = 1;
  for(; iterator < i + length; iterator++){
    //    fprintf(stderr, "Index: %d\n", iterator);
    //    fprintf(stderr, "%d", sblock_info[iterator]);
    *final_val += ((int)sblock_info[iterator])*multiplier;
    multiplier*=256;
  }
}

void dir_ent_parsing(int block_num, int parent_num){
  int offset = block_num*s_block_size;
  char* dir_data = malloc(s_block_size*sizeof(char));
  pread(fds, dir_data, s_block_size, offset);
  
  int i = 0;
  int current_inode;
  int name_len = 0;
  int rec_len = 0;
  for(; i < s_block_size && current_inode!=0; i+=rec_len){
    data_parser(dir_data, i, &current_inode, 4);
    if(current_inode){
      data_parser(dir_data, i+6, &name_len, 1);
      data_parser(dir_data, i+4, &rec_len, 2);
      int j = 8+i;
      int name_index = 0;
      char* name = malloc(name_len*sizeof(char));
      if(!name){
	printf("Error: %s\n", strerror(errno));
	exit(2);
      }
      for(; j < 8+i+name_len; j++, name_index++){
	name[name_index] = dir_data[j];
      }
      name[name_len] = '\0';
      printf("DIRENT,%d,%d,%d,%d,%d,\'%s\'\n", parent_num, i, current_inode, rec_len, name_len,name);
      free(name);
    }
  }
  free(dir_data);
}

void directory_entry(char* inode_info, int parent_num, int offset){
  int i = 0;
  int block_num;

  for(; i < 12;i++){
    data_parser(inode_info, (i*4), &block_num, 4);
    if(block_num){
      dir_ent_parsing(block_num, parent_num);
    }
  }
  doubly_indirect_parser(offset, 0, parent_num, 0, 1);
  triply_indirect_parser(offset, 0, parent_num, 1);
  
}

void doubly_indirect_parser(int offset, int* logical_block_offset, int parent_node_num, int input_array, int directory_parsing){
  int parent_inode = parent_node_num;
  char bit_holder[32];
  int second_direct_array = 0;
  if(!input_array){
   pread(fds, bit_holder, 4, offset + 52);
   data_parser(bit_holder, 0, &second_direct_array, 32);
  }
  else
    second_direct_array = input_array;
  int k = 0;
  int second_direct_block_num;
  int direct_block_num_low;
  int triggered = 0;
  for(;k < 256;k++){
    if(second_direct_array){
      pread(fds, bit_holder, 4, second_direct_array*s_block_size);
      data_parser(bit_holder, 0, &second_direct_block_num, 32);
      int j = 0;
      if(second_direct_block_num && !directory_parsing){
	printf("INDIRECT,%d,2,%d,%d,%d\n",parent_inode,*logical_block_offset, second_direct_array, second_direct_block_num);
      }
	pread(fds, bit_holder, 4, second_direct_block_num*s_block_size);
	data_parser(bit_holder, 0, &direct_block_num_low, 32);
	for(;j < 256;j++){//TODO: change inode table parsing so it doesn't rely on empty entry - change 256 to block size / 4
	  if(direct_block_num_low && !directory_parsing){
	    printf("INDIRECT,%d,1,%d,%d,%d\n",parent_inode,*logical_block_offset,second_direct_block_num, direct_block_num_low);
	  if(directory_parsing)
	     dir_ent_parsing(direct_block_num_low, parent_inode);
	  }
	  pread(fds, bit_holder, 4, second_direct_block_num*s_block_size + 4*j);
	  data_parser(bit_holder, 0, &direct_block_num_low, 32);
	  if(j && logical_block_offset)
	    (*logical_block_offset)++;
	}
    }
    else if(logical_block_offset){
      (*logical_block_offset)+=256;
      triggered = 1;
    }
    pread(fds, bit_holder, 4, second_direct_array*s_block_size+4);
    data_parser(bit_holder, 0, &second_direct_array, 32);
  }
  if(triggered)
    (*logical_block_offset)++;
}

void triply_indirect_parser(int offset, int* logical_block_offset, int parent_node_num, int directory_parsing){
  int parent_inode = parent_node_num;
  char bit_holder[32];
  int triple_direct_array = 0;
  int triple_direct_block_num = 0;
  pread(fds, bit_holder, 4, offset + 56);
  data_parser(bit_holder, 0, &triple_direct_array, 32);
  int i = 0;
  for(;i < 256; i++){
    if(triple_direct_array){      
      pread(fds, bit_holder, 4, triple_direct_array*s_block_size);
      data_parser(bit_holder, 0, &triple_direct_block_num, 32);
      if(!directory_parsing){
	printf("INDIRECT,%d,3,%d,%d,%d\n",parent_inode,*logical_block_offset,triple_direct_array, triple_direct_block_num);
      }
      if(triple_direct_block_num){
	doubly_indirect_parser(offset, logical_block_offset, parent_node_num, triple_direct_block_num, directory_parsing);
      }
    }
    pread(fds, bit_holder, 4, triple_direct_array*s_block_size+4);
    data_parser(bit_holder, 0, &triple_direct_array, 32);
  }
}
void inode_blocks_parser(char* inode_info, int parent_inode, int offset){
  int i = 0;
  int block_num;
  char bit_holder[32];
  for(; i < 15;i++){
    data_parser(inode_info, (i*4), &block_num, 4);
    printf(",%d", block_num);
  }
  printf("\n");
  
  //for single indirect 
  i = 12;
  int direct_array = 0;
  int logical_block_offset = 12;
  pread(fds, bit_holder, 4, offset + 48);
  data_parser(bit_holder, 0, &direct_array, 32);
  if(direct_array){
    int j = 0;
    int direct_block_num = 0;
    pread(fds, bit_holder, 4, direct_array*1024);
    data_parser(bit_holder, 0, &direct_block_num, 32);
    for(;j < 256;j++){
      if(direct_block_num){
	printf("INDIRECT,%d,1,%d,%d,%d\n",parent_inode,logical_block_offset,direct_array, direct_block_num);
      }
      pread(fds, bit_holder, 4, (direct_array*s_block_size + 4*(j+1)));
      data_parser(bit_holder, 0, &direct_block_num, 32);
      logical_block_offset++;
    }
  }
  
  //for doubly indirect
  doubly_indirect_parser(offset, &logical_block_offset, parent_inode, 0, 0);
  triply_indirect_parser(offset, &logical_block_offset, parent_inode, 0);
  
  
}
  


void bit_comparator(char* bit_holder, int comp_val, int block_num, int inode){
  if(!(bit_holder[0] & comp_val)){
    if(!inode)
      printf("BFREE,%d\n", block_num);
    else
      printf("IFREE,%d\n", block_num);
  }
}

void superblock(){
  char sblock_info[1025];
  int result = pread(fds, sblock_info, 1024, 1024);
  if(result == -1){
    printf("Error: %s\n", strerror(errno));
  }
  sblock_info[1024] = '\0';
  
  data_parser(sblock_info, 0, &s_inodes_count, 4);
  data_parser(sblock_info, 4, &s_blocks, 4);
  data_parser(sblock_info, 24, &s_log_block_size, 4);
  s_block_size = 1024 << s_log_block_size;
  if(s_block_size < 0)
    s_block_size*=-1;
  data_parser(sblock_info, 88, &s_inode_size, 2);
  if(s_inode_size < 0)
    s_inode_size*=-1;
  data_parser(sblock_info, 32, &s_blocks_p_group, 4);
  data_parser(sblock_info, 40, &s_inodes_p_group, 4);
  data_parser(sblock_info, 84, &s_nr_inode, 4);;
  
  printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", s_blocks, s_inodes_count, s_block_size, s_inode_size, s_blocks_p_group, s_inodes_p_group, s_nr_inode);
  
}

void group(){
  char block_group_descriptor_info[32];
  int group_num = 0;
  int bg_bitmap = 0;
  int inode_bitmap = 0;
  int num_blocks = 0;
  int num_inodes = 0;
  int free_inodes = 0;
  int free_blocks = 0;

  if(s_blocks > s_blocks_p_group)
    num_blocks = s_blocks_p_group;
  else
    num_blocks = s_blocks;
  if(s_inodes_count > s_inodes_p_group)
    num_inodes = s_inodes_p_group;
  else
    num_inodes = s_inodes_count;
  
  int result = pread(fds, block_group_descriptor_info, 32, 2048);
  if(result == -1){
    printf("Error: %s\n", strerror(errno));
  }
  data_parser(block_group_descriptor_info, 0, &bg_bitmap, 4);
  data_parser(block_group_descriptor_info, 4, &inode_bitmap, 4);
  data_parser(block_group_descriptor_info, 12, &free_blocks, 2);
  data_parser(block_group_descriptor_info, 8, &inode_table, 4);
  data_parser(block_group_descriptor_info, 14, &free_inodes, 2);

  printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", group_num, num_blocks, num_inodes, free_blocks, free_inodes, bg_bitmap, inode_bitmap, inode_table);
  int i = 0;
  int offset = bg_bitmap*s_block_size;
  int block_num = 1;
  char bit_holder[1];
  //TODO: make this neater; turn into auxillary function
  for(; block_num < num_blocks; i++){
    pread(fds, bit_holder, 1, offset+i);
    bit_comparator(bit_holder, 1, block_num, 0);
    bit_comparator(bit_holder, 2, block_num+1, 0);
    bit_comparator(bit_holder, 4, block_num+2, 0);
    bit_comparator(bit_holder, 8, block_num+3, 0);
    bit_comparator(bit_holder, 16, block_num+4, 0);
    bit_comparator(bit_holder, 32, block_num+5, 0);
    bit_comparator(bit_holder, 64, block_num+6, 0);
    bit_comparator(bit_holder, 128, block_num+7, 0);
    block_num+=8;
  }
  offset = inode_bitmap*s_block_size;
  i = 0;
  int inode_num = 1;
  for(; inode_num < num_inodes; i++){
    pread(fds, bit_holder, 1, offset+i);
    bit_comparator(bit_holder, 1, inode_num, 1);
    bit_comparator(bit_holder, 2, inode_num+1, 1);
    bit_comparator(bit_holder, 4, inode_num+2, 1);
    bit_comparator(bit_holder, 8, inode_num+3, 1);
    bit_comparator(bit_holder, 16, inode_num+4, 1);
    bit_comparator(bit_holder, 32, inode_num+5, 1);
    bit_comparator(bit_holder, 64, inode_num+6, 1);
    bit_comparator(bit_holder, 128, inode_num+7, 1);
    inode_num+=8;
  }

}

void inode(){
  int offset = inode_table*s_block_size;
  int i = 1;
  char bit_holder[128];
  int inode_mode = 0;
  int inode_link_count = 0;
  char file_type;
  for(; i <= s_inodes_count; i++){//only dealing with one group, so we can just iterate through s_inodes_count
    int current_offset = offset+((i-1)*128);
    pread(fds, bit_holder, 128, current_offset);
    data_parser(bit_holder, 0, &inode_mode, 2);
    data_parser(bit_holder, 26, &inode_link_count, 2);
    if(inode_mode != 0 && inode_link_count != 0){
      int owner, group_num, file_size, i_num_blocks, access_time, creation_time, modification_time;
      if(inode_mode & 0x8000)
	file_type = 'f';
      else if(inode_mode & 0x4000){
	file_type = 'd';
      }
      else if(inode_mode & 0xA000)
	file_type = 's';
      else
	file_type = '?';
      int low_bits_mode = (inode_mode & 0x0fff);

      data_parser(bit_holder, 2, &owner, 2);
      data_parser(bit_holder, 24,&group_num, 2);
      data_parser(bit_holder, 8, &access_time, 4);
      data_parser(bit_holder, 12, &creation_time, 4);
      data_parser(bit_holder, 16, &modification_time, 4);
      data_parser(bit_holder, 4, &file_size, 4);
      
      data_parser(bit_holder, 28, &i_num_blocks, 4);
      time_t a_time = access_time;
      time_t c_time = creation_time;
      time_t m_time = modification_time;

      struct tm* a_g_time;
      struct tm* c_g_time;
      struct tm* m_g_time;

      c_g_time = gmtime(&c_time);
      int c_year = ((c_g_time->tm_year + 1900)%1000)%100;
      int c_month = c_g_time->tm_mon + 1;
      int c_hour =  c_g_time->tm_hour;
      
      printf("INODE,%d,%c,%o,%d,%d,%d,", i, file_type, low_bits_mode, owner, group_num, inode_link_count);
      printf("%02d/%02d/%02d %02d:%02d:%02d,", c_month, c_g_time->tm_mday, c_year, c_hour, c_g_time->tm_min, c_g_time->tm_sec);
      
      m_g_time = gmtime(&m_time);
      int m_year = ((m_g_time->tm_year + 1900)%1000)%100;
      int m_month = m_g_time->tm_mon + 1;
      int m_hour =  m_g_time->tm_hour;

      printf("%02d/%02d/%02d %02d:%02d:%02d,", m_month, m_g_time->tm_mday, m_year, m_hour, m_g_time->tm_min, m_g_time->tm_sec);
      
      a_g_time = gmtime(&a_time);
      int a_year = ((a_g_time->tm_year + 1900)%1000)%100;
      int a_month = a_g_time->tm_mon + 1;
      int a_hour =  a_g_time->tm_hour;
      
      printf("%02d/%02d/%02d %02d:%02d:%02d,", a_month, a_g_time->tm_mday, a_year, a_hour, a_g_time->tm_min, a_g_time->tm_sec);
      printf("%d,%d", file_size, i_num_blocks);
      char i_blocks[60];
      pread(fds, i_blocks, 60, current_offset+40);
      inode_blocks_parser(i_blocks, i, current_offset+40);
      if(file_type == 'd'){
	directory_entry(i_blocks, i, offset);
      }
      
    }
  }
}


int main(int argc, char* argv[]){

  if(argc != 2){
    printf("There was no file image specified");
    exit(1);
  }
  else{
    file_name = argv[1];
  }
  
  fds = open(file_name, O_RDONLY);
  if(fds == -1){
    fprintf(stderr, "Error:%s\n", strerror(errno));
    exit(1);
  }
  //  close(1);
  //  dup(fds);
  // close(fds);

  superblock();
  group();
  inode();
  return 0;
}
