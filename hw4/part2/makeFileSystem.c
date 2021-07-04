#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#define TRUE 1
#define FALSE 0
#define MAXBLOCKS 1024
#define BLOCKSIZE 1024
#define FATENTRYCOUNT (BLOCKSIZE / sizeof(fatentry_t))
#define DIRENTRYCOUNT ((BLOCKSIZE - (2*sizeof(int))) / sizeof(direntry_t))
#define MAXNAME 256
#define MAXPATHLENGTH 1024

#define UNUSED -1
#define ENDOFCHAIN 0


#define DATA 0
#define DIR 2

typedef unsigned char Byte;

typedef short fatentry_t;


typedef struct direntry {
  int         entrylength;  
  Byte        is_dir; 
  Byte        unused;
  time_t      modtime;
  int         fileLength;
  fatentry_t  first_block;
  char   name [MAXNAME];
} direntry_t;


typedef fatentry_t fatblock_t[FATENTRYCOUNT];


typedef struct dirblock {
  int is_dir;
  int next_entry;
  direntry_t entrylist[DIRENTRYCOUNT]; 
} dirblock_t;


typedef Byte datablock_t[BLOCKSIZE];


typedef union block {
  datablock_t data;
  dirblock_t  dir;
  fatblock_t  fat;
} diskblock_t;


extern diskblock_t virtual_disk[MAXBLOCKS];



typedef struct filedescriptor {
  int         pos;           
  char        mode[3]; 
  Byte        writing;
  fatentry_t  blockno;
  diskblock_t buffer;
} my_file_t;


diskblock_t virtual_disk[MAXBLOCKS]; 
fatentry_t FAT[MAXBLOCKS];            
fatentry_t root_dir_index = 0;         
direntry_t *current_dir = NULL; 
fatentry_t current_dir_index = 0;
void write_block(diskblock_t *block, int block_address, char type)
{
  if (type == 'd') { 
    memmove(virtual_disk[block_address].data, block->data, BLOCKSIZE);
  }
  else if (type == 'f') { 
    memmove(virtual_disk[block_address].fat, block->fat, BLOCKSIZE);
  }
  else if (type == 'r') { 
    memmove(virtual_disk[block_address].data, block->data, BLOCKSIZE);
  }
  else {
    printf("Invalid Type");
  }
}


void copy_fat(fatentry_t *FAT)
{
  diskblock_t block;
  int index = 0;
  for(int x = 1; (unsigned) x <= (MAXBLOCKS /(BLOCKSIZE / sizeof(fatentry_t))); x++) {
    for(int y = 0; y < (BLOCKSIZE / sizeof(fatentry_t)); y++){
      block.fat[y] = FAT[index++];
    }
    memmove(virtual_disk[x].fat, block.fat, BLOCKSIZE);
    write_block(&block, x, 'f');
  }
}

void initiliaze(char *name)
{
  diskblock_t block;
  int required_fat_space = (MAXBLOCKS / FATENTRYCOUNT);

  for (int i = 0; i < BLOCKSIZE; i++) {
    block.data[i] = '\0';
  }

  memcpy(block.data, name, strlen(name));
  memmove(virtual_disk[0].data, block.data, BLOCKSIZE);

  FAT[0] = ENDOFCHAIN;
  FAT[1] = 2;
  FAT[2] = ENDOFCHAIN;
  FAT[3] = ENDOFCHAIN;
  for(int i = 4; i < MAXBLOCKS; i++){
    FAT[i] = UNUSED;
  }
  copy_fat(FAT);

  diskblock_t root_block;
  int root_block_index = required_fat_space + 1;
  root_block.dir.is_dir = TRUE;
  root_block.dir.next_entry = 0;

  direntry_t *blank_entry = malloc(sizeof(direntry_t));
  blank_entry->unused = TRUE;
  blank_entry->fileLength = 0;
  for(int i = 0; i < DIRENTRYCOUNT; i ++){
    root_block.dir.entrylist[i] = *blank_entry;
  }

  write_block(&root_block, root_block_index, 'd');

  root_dir_index = root_block_index;
  current_dir_index = root_dir_index;

  blank_entry->first_block = root_dir_index;
  current_dir = blank_entry;
}

void init_block(diskblock_t *block)
{
  for (int i = 0; i < BLOCKSIZE; i++) {
    block->data[i] = '\0';
  }
}

void init_dir_block(diskblock_t *block){
  block->dir.is_dir = TRUE;
  block->dir.next_entry = 1;

  direntry_t *blank_entry = malloc(sizeof(direntry_t));
  blank_entry->unused = TRUE;
  blank_entry->fileLength = 0;
  for(int i = 0; i < DIRENTRYCOUNT; i ++){
    block->dir.entrylist[i] = *blank_entry;
  }
}

int next_unallocated_block()
{
  for(int i = 0; i < MAXBLOCKS; i++){
    if (FAT[i] == UNUSED){
      FAT[i] = ENDOFCHAIN;
      copy_fat(FAT);
      return i;
    }
  }
  return -1; 
}
int next_unallocated_dir_entry(){
 

  int next_entry = virtual_disk[current_dir_index].dir.next_entry;
  if(next_entry > DIRENTRYCOUNT - 1){
    virtual_disk[current_dir_index].dir.next_entry = 0;

    int new_dir_block_index = next_unallocated_block();
    FAT[current_dir_index] = new_dir_block_index;
    FAT[new_dir_block_index] = ENDOFCHAIN;
    copy_fat(FAT);

    current_dir_index = new_dir_block_index;

    diskblock_t new_dir_block = virtual_disk[new_dir_block_index];
    new_dir_block.dir.is_dir = TRUE;
    new_dir_block.dir.next_entry = 0;

    direntry_t *blank_entry = malloc(sizeof(direntry_t));
    blank_entry->unused = TRUE;
    blank_entry->fileLength = 0;
    for(int i = 0; i < DIRENTRYCOUNT; i ++){
      new_dir_block.dir.entrylist[i] = *blank_entry;
    }
    write_block(&new_dir_block, new_dir_block_index, 'd');

    current_dir_index = new_dir_block_index;
  }
  return virtual_disk[current_dir_index].dir.next_entry++;;
}

int file_entry_index(char *filename){
  current_dir_index = current_dir->first_block;
  while(1){
    for(int i = 0; i < DIRENTRYCOUNT; i++){
      if (memcmp(virtual_disk[current_dir_index].dir.entrylist[i].name, filename, strlen(filename) + 1) == 0)
        return i;
    }

    if(FAT[current_dir_index] == ENDOFCHAIN) break;
    current_dir_index = FAT[current_dir_index];
  }
  return -1;
}

void movend(my_file_t *file){
  while(1) {
    if(FAT[file->blockno] == ENDOFCHAIN) break;
    file->blockno = FAT[file->blockno];
  }

  file->buffer = virtual_disk[file->blockno];
  while(1) {
    if(file->buffer.data[file->pos+++1] == '\0') break;
  }

  if (file->buffer.data[0] == '\0') file->pos = 0;
}

int add_block_to_directory(int index, char *name, int is_dir) {
  int entry_index = next_unallocated_dir_entry();

  diskblock_t file_dir_block = virtual_disk[current_dir_index];

  direntry_t *file_dir = &file_dir_block.dir.entrylist[entry_index];

  memcpy(file_dir->name, name, strlen(name));
  file_dir->is_dir = is_dir;
  file_dir->unused = FALSE;
  file_dir->first_block = index;

  write_block(&file_dir_block, current_dir_index, 'd');

  return entry_index;
}





int main(int argc,char *argv[]){
  
diskblock_t *block=(diskblock_t *)malloc(sizeof(diskblock_t *));
       initiliaze(argv[2]);
       init_dir_block(block);

      next_unallocated_block();

     next_unallocated_dir_entry();
     
    file_entry_index(argv[2]);
      
    
  

    return 0;
}
