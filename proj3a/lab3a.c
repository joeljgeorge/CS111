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

void bit_comparator(char* bit_holder, int comp_val, int block_num, int inode){
  if(!(bit_holder[0] & comp_val)){
    if(!inode)
      fprintf(stderr, "BFREE,%d\n", block_num);
    else
      fprintf(stderr, "IFREE, %d\n", block_num);
  }
}

void superblock(){
  char sblock_info[1025];
  int result = pread(1, sblock_info, 1024, 1024);
  if(result == -1){
    fprintf(stderr, "Error: %s\n", strerror(errno));
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
  
  fprintf(stderr, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", s_blocks, s_inodes_count, s_block_size, s_inode_size, s_blocks_p_group, s_inodes_p_group, s_nr_inode);
  //TODO: fix this to make it print to stdout instead
}

void group(){
  //TODO: code to allow for multiple groups (check total num of blocks divided by blocks per group)
  char block_group_descriptor_info[32];
  int group_num = 0;
  int bg_bitmap = 0;
  int inode_bitmap = 0;
  int num_blocks = 0;
  int num_inodes = 0;
  int free_inodes = 0;
  int free_blocks = 0;
  //TODO: check if search for blocks must be more rigorous
  if(s_blocks > s_blocks_p_group)
    num_blocks = s_blocks_p_group;
  else
    num_blocks = s_blocks;
  if(s_inodes_count > s_inodes_p_group)
    num_inodes = s_inodes_p_group;
  else
    num_inodes = s_inodes_count;
  
  int result = pread(1, block_group_descriptor_info, 32, 2048);
  if(result == -1){
    fprintf(stderr, "Error: %s\n", strerror(errno));
  }
  data_parser(block_group_descriptor_info, 0, &bg_bitmap, 4);
  data_parser(block_group_descriptor_info, 4, &inode_bitmap, 4);
  data_parser(block_group_descriptor_info, 12, &free_blocks, 2);
  data_parser(block_group_descriptor_info, 8, &inode_table, 4);
  data_parser(block_group_descriptor_info, 14, &free_inodes, 2);

  fprintf(stderr, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", group_num, num_blocks, num_inodes, free_blocks, free_inodes, bg_bitmap, inode_bitmap, inode_table);
  int i = 0;
  int offset = bg_bitmap*s_block_size;
  int block_num = 1;
  char bit_holder[1];
  //TODO: make this neater; turn into auxillary function
  for(; block_num < num_blocks; i++){
    pread(1, bit_holder, 1, offset+i);
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
    pread(1, bit_holder, 1, offset+i);
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
  s_inodes_count;
  int offset = inode_table*s_block_size;
  int i = 1;
  char bit_holder[128];
  char block_num[480];
  int inode_mode = 0;
  int inode_link_count = 0;
  char file_type;
  for(; i <= s_inodes_count; i++){
    fprintf(stderr, "Offset: %d\n", offset);
    pread(1, bit_holder, 128, offset+((i-1)*128));
    data_parser(bit_holder, 0, &inode_mode, 2);
    data_parser(bit_holder, 26, &inode_link_count, 2);
    if(inode_mode != 0 && inode_link_count != 0){
      int owner, group_num, file_size, i_num_blocks, access_time, creation_time, modification_time;
      if(inode_mode & 0x8000)
	file_type = 'f';
      else if(inode_mode & 0x4000)
	file_type = 'd';
      else if(inode_mode & 0xA000)
	file_type = 's';
      else
	file_type = '?';
      int low_bits_mode = (inode_mode & 0x0fff);//TODO: Fix this

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
      a_g_time = gmtime(&a_time);
      int a_year = a_g_time->tm_year + 1900;
      int a_month = a_g_time->tm_mon + 1;
      int a_hour =  a_g_time->tm_hour + 1;

      c_g_time = gmtime(&c_time);
      int c_year = c_g_time->tm_year + 1900;
      int c_month = c_g_time->tm_mon + 1;
      int c_hour =  c_g_time->tm_hour + 1;

      m_g_time = gmtime(&m_time);
      int m_year = m_g_time->tm_year + 1900;
      int m_month = m_g_time->tm_mon + 1;
      int m_hour =  m_g_time->tm_hour + 1;
      
      fprintf(stderr, "INODE,%d,%c,%o,%d,%d,%d\n", i, file_type, low_bits_mode, owner, group_num, inode_link_count);
      fprintf(stderr, "%02d/%02d/%02d %02d:%02d:%02d,", c_month, c_g_time->tm_mday, c_year, c_hour, c_g_time->tm_min, c_g_time->tm_sec);
      fprintf(stderr, "%02d/%02d/%02d %02d:%02d:%02d,", m_month, m_g_time->tm_mday, m_year, m_hour, m_g_time->tm_min, m_g_time->tm_sec);
      fprintf(stderr, "%02d/%02d/%02d %02d:%02d:%02d,", a_month, a_g_time->tm_mday, a_year, a_hour, a_g_time->tm_min, a_g_time->tm_sec);
      fprintf(stderr, "%d, %d\n", file_size, i_num_blocks);
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
  
  int fds = open(file_name, O_RDONLY);//TODO: check if file was opened
  fprintf(stderr, "This is fds: %d\n", fds);

  close(1);
  dup(fds);
  close(fds);

  superblock();
  group();
  inode();
}
