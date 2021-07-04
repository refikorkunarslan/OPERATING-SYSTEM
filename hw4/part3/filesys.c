
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
  int         file_length;
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
} filedesc_t;

void initiliaze();
void write_disk(const char * file_name);
void save_file();

filedesc_t *myfopen(char *path, char *mode);
char myfgetc(filedesc_t *file);
int myfputc(char character, filedesc_t *file);
int myfclose(filedesc_t *file);
void p_block(int block_index, char type);
void print_fat(int length);
void create_file();
void print_directory_structure(int current_dir_block, int indent);
void mymkdir(char *path);
char **mylistdir(char *path);
int dir_index_for_path(char *path);
void p_dirList(char **list);
void mychdir(char *path);
void current();
char **path_to_array(char *path);
char *last_entry_in_path(char **path);
int numPathEntry(char **path);
void myremove( char * path);
int file_entry_index(char *filename);
void myrmdir(char *path);


diskblock_t virtual_disk[MAXBLOCKS]; 
fatentry_t FAT[MAXBLOCKS];            
fatentry_t root_dir_index = 0;        
direntry_t *current_dir = NULL; 
fatentry_t current_dir_index = 0;

void write_disk(const char *file_name)
{
  FILE *dest = fopen(file_name, "w");
  fwrite(virtual_disk, sizeof(virtual_disk), 1, dest);
  fclose(dest);
}

void read_disk(const char *file_name)
{
  FILE *dest = fopen(file_name, "r");
  fclose(dest);
}

void p_block(int block_index, char type)
{
  if (type == 'd') {
    printf("virtualdisk[%d] = ", block_index);
    for (int i = 0; i < BLOCKSIZE; i++)
    {
      printf("%c", virtual_disk[block_index].data[i]);
    }
    printf("\n");
  }
  else if (type == 'f') {
    printf("virtualdisk[%d] = ", block_index);
    for(int i = 0; i < FATENTRYCOUNT; i++) {
      printf("%d", virtual_disk[block_index].fat[i]);
    }
    printf("\n");
  }
  else if (type == 'r') {
    printf("virtualdisk[%d] = \n", block_index);
    printf("is_dir: %d\n", virtual_disk[block_index].dir.is_dir);
    printf("next_entry: %d\n", virtual_disk[block_index].dir.next_entry);
    for(int i = 0; i < DIRENTRYCOUNT; i++){
      if(strlen(virtual_disk[block_index].dir.entrylist[i].name) == 0){
        printf("Empty direntry_t\n");
      }
      else {
        printf("%d: ", i);
        printf("entrylength: %d, ", virtual_disk[block_index].dir.entrylist[i].entrylength);
        printf("id_dir: %d, ", virtual_disk[block_index].dir.entrylist[i].is_dir);
        printf("unused: %d, ", virtual_disk[block_index].dir.entrylist[i].unused);
        printf("modtime: %ld, ", virtual_disk[block_index].dir.entrylist[i].modtime);
        printf("file_length: %d, ", virtual_disk[block_index].dir.entrylist[i].file_length);
        printf("first_block: %d, ", virtual_disk[block_index].dir.entrylist[i].first_block);
        printf("name: '%s'\n", virtual_disk[block_index].dir.entrylist[i].name);
      }
    }
  }
  else {
    printf("Invalid Type");
  }
}

void print_fat(int length)
{
  for(int i = 0; i < length; i++){
    printf("%d, ", FAT[i]);
  }
  printf("\n");
}

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
  for(int x = 1; x <= (MAXBLOCKS /(BLOCKSIZE / sizeof(fatentry_t))); x++) {
    for(int y = 0; y < (BLOCKSIZE / sizeof(fatentry_t)); y++){
      block.fat[y] = FAT[index++];
    }
    memmove(virtual_disk[x].fat, block.fat, BLOCKSIZE);
    write_block(&block, x, 'f');
  }
}

void initiliaze(char *volume_name)
{
  diskblock_t block;
  int required_fat_space = (MAXBLOCKS / FATENTRYCOUNT);

  for (int i = 0; i < BLOCKSIZE; i++) {
    block.data[i] = '\0';
  }

  memcpy(block.data, volume_name, strlen(volume_name));
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
  blank_entry->file_length = 0;
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
  blank_entry->file_length = 0;
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
    blank_entry->file_length = 0;
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

void movend(filedesc_t *file){
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

diskblock_t create_block(int index, int type) {
  diskblock_t block = virtual_disk[index];

 
  if(type == DIR) init_dir_block(&block);
  if(type == DATA) init_block(&block);

  write_block(&block, index, 'd'); 

  return block;
}

filedesc_t *myfopen(char *path, char *mode)
{
  int initial_current_dir_index = current_dir_index;
  int initial_current_dir_first_block = current_dir->first_block;

  if(numPathEntry(path_to_array(path)) > 1){
    mychdir(path);
  }

  char filename[MAXNAME];
  strcpy(filename, last_entry_in_path(path_to_array(path)));
  strcat(filename, "\0");

  int dir_entry_index = file_entry_index(filename);

  if(dir_entry_index == -1 && strncmp(mode, "r", 1) != 0){
    printf("File did not exist. Creating new file: %s\n", path);

    int block_index = next_unallocated_block();
    create_block(block_index, DATA);

    dir_entry_index = add_block_to_directory(block_index, filename, FALSE);
  }

  direntry_t dir_entry = virtual_disk[current_dir_index].dir.entrylist[dir_entry_index];

  filedesc_t *file = malloc(sizeof(filedesc_t));
  file->pos = 0;
  file->writing = 0;
  memcpy(file->mode, mode, strlen(mode));
  file->blockno = dir_entry.first_block;
  file->buffer = virtual_disk[dir_entry.first_block];

  if(strncmp(file->mode, "a", 1) == 0){
    movend(file);
  }

  current_dir_index = initial_current_dir_index;
  current_dir->first_block = initial_current_dir_first_block;

  return file;
}

char myfgetc(filedesc_t *file)
{
  int position = file->pos;
  file->pos++;
  if ((file->buffer.data[position] == '\0') && (FAT[file->blockno] != ENDOFCHAIN)){
    file->pos = 1;
    file->blockno = FAT[file->blockno];
    file->buffer = virtual_disk[file->blockno];
    return file->buffer.data[file->pos - 1];
  }
  else if ((file->pos > BLOCKSIZE) && FAT[file->blockno] == ENDOFCHAIN) {
    return EOF;
  }
  else {
    return file->buffer.data[position];
  }
}

int myfputc(char character, filedesc_t *file)
{
  if (strncmp(file->mode, "r", 1) == 0) return 1;
  if (file->pos == BLOCKSIZE){
    file->pos = 0;
    if(FAT[file->blockno] == ENDOFCHAIN) {
      int block_index = next_unallocated_block();
      FAT[file->blockno] = block_index;
      copy_fat(FAT);

      file->blockno = block_index;
      file->buffer = create_block(block_index, DATA);
    }
    else {
      file->blockno = FAT[file->blockno];
      file->buffer = virtual_disk[file->blockno];
    }
  }

  file->buffer.data[file->pos] = character;
  write_block(&file->buffer, file->blockno, 'd');

  file->pos++;

  return 0;
}

int myfclose(filedesc_t *file)
{
  free(file);
  return 0;
}

void print_directory_structure(int current_dir_block, int indent){
  char string[] = "\0\0\0\0\0\0";
  int x;
  for(x = 0; x < indent; x++){
    string[x] = '\t';
  }

  while(1){
    for(int i = 0; i < DIRENTRYCOUNT; i++){
      printf("%s", string);
      printf("%d: ", i);
      if(strlen(virtual_disk[current_dir_block].dir.entrylist[i].name) == 0){
        printf("empty\n");
      }
      else {
        printf("name: '%s', ", virtual_disk[current_dir_block].dir.entrylist[i].name);
        printf("first_block: %d, ", virtual_disk[current_dir_block].dir.entrylist[i].first_block);
       printf("is_dir: %d, ", virtual_disk[current_dir_block].dir.entrylist[i].is_dir);
      printf("unused: %d, ", virtual_disk[current_dir_block].dir.entrylist[i].unused);
      printf("modtime: %ld, ", virtual_disk[current_dir_block].dir.entrylist[i].modtime);
      printf("file_length: %d, ", virtual_disk[current_dir_block].dir.entrylist[i].file_length);
      printf("entrylength: %d\n", virtual_disk[current_dir_block].dir.entrylist[i].entrylength);

        if (virtual_disk[current_dir_block].dir.entrylist[i].is_dir == 1){
          print_directory_structure(virtual_disk[current_dir_block].dir.entrylist[i].first_block, indent + 1);
        }
      }
    }

    if(FAT[current_dir_block] == ENDOFCHAIN) break;
    current_dir_block = FAT[current_dir_block];
  }
}

void mymkdir(char *path) {
  int initial_current_dir_index = current_dir_index;
  int initial_current_dir_first_block = current_dir->first_block;

  
  if (path[0] == '/') {
    current_dir_index = root_dir_index;
    current_dir->first_block = root_dir_index;
  }

  
  char str[strlen(path)];
  strcpy(str, path);

  char *dir_name = strtok(str, "/");

  while (dir_name) {
      int sub_dir_block_index = next_unallocated_block();
      create_block(sub_dir_block_index, DIR);

      virtual_disk[sub_dir_block_index].dir.entrylist[0].unused = FALSE;
      virtual_disk[sub_dir_block_index].dir.entrylist[0].first_block = sub_dir_block_index;
      strcpy(virtual_disk[sub_dir_block_index].dir.entrylist[0].name, "..");

      add_block_to_directory(sub_dir_block_index, dir_name, TRUE);

      current_dir_index = sub_dir_block_index;
      current_dir->first_block = sub_dir_block_index;

      dir_name = strtok(NULL, "/");
  }

  current_dir_index = initial_current_dir_index;
  current_dir->first_block = initial_current_dir_first_block;
}

void p_dirList(char **list){
  for (int i = 0; i< 10; i++){
    if(strcmp(list[i],"ENDOFDIR") == 0) break;
    printf("%s\n", list[i]);
  }
}

char **mylistdir(char *path) {
  int initial_current_dir_index = current_dir_index;
  int initial_current_dir_first_block = current_dir->first_block;

  if (strcmp(path, "root") == 0 || strcmp(path, "") == 0){
    printf("Whoa, listing root!\n");
    current_dir_index = root_dir_index;
  } else {
    int location = dir_index_for_path(path);
    if (location == -1) {
    
      char **file_list = malloc(1 * MAXNAME * sizeof(char));
      for (int i = 0; i < 2; i++)
          file_list[i] = malloc(sizeof(**file_list) * 30);
      strcpy(file_list[0], "Directory does not exist.");
      strcpy(file_list[1], "ENDOFDIR");
      return file_list;
    }
    current_dir_index = location;
  }

  int max_entries = 10;
  char **file_list = malloc(max_entries * MAXNAME * sizeof(char));
  for (int i = 0; i < max_entries; i++)
      file_list[i] = malloc(sizeof(**file_list) * 30);

  int print_count = 0;
  while(1){
    for(int i = 0; i < DIRENTRYCOUNT; i++){
      if(strlen(virtual_disk[current_dir_index].dir.entrylist[i].name) != 0){
        strcpy(file_list[print_count], virtual_disk[current_dir_index].dir.entrylist[i].name);
        print_count++;

        if (print_count >= max_entries) break;
      }
    }
    if (print_count >= max_entries) break;

    if(FAT[current_dir_index] == ENDOFCHAIN) break;
    current_dir_index = FAT[current_dir_index];
  }

  current_dir_index = initial_current_dir_index;
  current_dir->first_block = initial_current_dir_first_block;

  char **file_list_final = malloc((print_count + 1) * MAXNAME * sizeof(char));
  for (int i = 0; i < print_count; i++){
    file_list_final[i] = file_list[i];
  }
  file_list_final[print_count] = "ENDOFDIR";

  return file_list_final;
}

int dir_index_for_path(char *path){
  int initial_current_dir_index = current_dir_index;
  int initial_current_dir_first_block = current_dir->first_block;

  if (path[0] == '/') {
    current_dir_index = root_dir_index;
    current_dir->first_block = root_dir_index;
  }

  char str[strlen(path)];
  strcpy(str, path);

  int location;
  int next_dir = current_dir_index;
  char *dir_name = strtok(str, "/");
  while (dir_name) {
      location = file_entry_index(dir_name);
      if (location == - 1) return -1;

      if (virtual_disk[current_dir_index].dir.entrylist[location].is_dir == 0) break;
      next_dir = virtual_disk[current_dir_index].dir.entrylist[location].first_block;

      current_dir_index = next_dir;
      current_dir->first_block = next_dir;

      dir_name = strtok(NULL, "/");
      if (dir_name == NULL) break;
  }

  current_dir_index = initial_current_dir_index;
  current_dir->first_block = initial_current_dir_first_block;

  return next_dir;
}

char **path_to_array(char *path) {
  int max_entries = 10;
  char **file_list = malloc(max_entries * 256 * sizeof(char));
  for (int i = 0; i < max_entries; i++)
      file_list[i] = malloc(sizeof(**file_list) * 30);

  char str[strlen(path)];
  strcpy(str, path);

  char *dir_name = strtok(str, "/");
  file_list[0] = dir_name;
  int count = 1;
  while (dir_name) {
      dir_name = strtok(NULL, "/");
      if (dir_name == NULL) break;
      file_list[count] = dir_name;
      count++;
  }

  return file_list;
}

char *last_entry_in_path(char **path){
  int count = 0;
  for (int i = 0; i < 10; i++){
    if(strlen(path[i]) == 0) break;
    count++;
  }
  return path[count-1];
}

int numPathEntry(char **path){
  int count = 0;
  for (int i = 0; i < 10; i++){
    if(strlen(path[i]) == 0) break;
    count++;
  }
  return count;
}
void mychdir(char *path){
  if (strcmp(path, "root") == 0){
    printf("Returning to root...\n");
    current_dir_index = root_dir_index;
    current_dir->first_block = root_dir_index;
    return;
  }

  if (path[0] == '/') {
    current_dir_index = root_dir_index;
    current_dir->first_block = root_dir_index;
  }

  int new_dir = dir_index_for_path(path);
  if (new_dir == -1) return;
  current_dir_index = new_dir;
  current_dir->first_block = new_dir;
}

void myremove(char *path){
  int initial_current_dir_index = current_dir_index;
  int initial_current_dir_first_block = current_dir->first_block;

  mychdir(path);

  char filename[MAXNAME];
  strcpy(filename, last_entry_in_path(path_to_array(path)));
  strcat(filename, "\0");

  int location = file_entry_index(filename);

  if (location == -1) {
    printf("File not found.\n");
    return;
  }

  int block_index = virtual_disk[current_dir_index].dir.entrylist[location].first_block;
  int next_block_index;
  while(1){
    next_block_index = FAT[block_index];
    FAT[block_index] = UNUSED;
    if (next_block_index == ENDOFCHAIN) break;
    block_index = next_block_index;
  }
  copy_fat(FAT);

  direntry_t *dir_entry = &virtual_disk[current_dir_index].dir.entrylist[location];
  dir_entry->first_block = 0;
  dir_entry->unused = 1;
  int length = strlen(dir_entry->name);
  for (int i = 0; i < length; i++){
    dir_entry->name[i] = '\0';
  }

  current_dir_index = initial_current_dir_index;
  current_dir->first_block = initial_current_dir_first_block;
}

void myrmdir(char *path){
  if (path[0] == '/'){
    mychdir("root");
  }

  if (strcmp(mylistdir(path)[1], "ENDOFDIR") == 0) {
    int initial_current_dir_index = current_dir_index;
    int initial_current_dir_first_block = current_dir->first_block;

    char *target = strstr(path, last_entry_in_path(path_to_array(path)));
    int position = target - path;
    char parent_path[position+1];
    for(int i = 0; i < position + 1; i++){
      parent_path[i] = '\0';
    }

    strncpy(parent_path, path, position);
    if(strcmp(parent_path,"") != 0) {
      mychdir(parent_path);
    }
    int entrylist_target = file_entry_index(last_entry_in_path(path_to_array(path)));

    int block_chain_target = virtual_disk[current_dir_index].dir.entrylist[entrylist_target].first_block;

    direntry_t *dir_entry = &virtual_disk[current_dir_index].dir.entrylist[entrylist_target];
    dir_entry->first_block = 0;
    dir_entry->unused = 1;
    int length = strlen(dir_entry->name);
    for (int i = 0; i < length; i++){
      dir_entry->name[i] = '\0';
    }

    int next_block_chain_target;
    while(1){
      next_block_chain_target = FAT[block_chain_target];
      FAT[block_chain_target] = UNUSED;
      if (next_block_chain_target == ENDOFCHAIN) break;
      block_chain_target = next_block_chain_target;
    }
    copy_fat(FAT);

    current_dir_index = initial_current_dir_index;
    current_dir->first_block = initial_current_dir_first_block;
  } else {
    printf("That directory has content and cannot be deleted.\n");
  }
}
int main(int argc, char *argv[]) 
{
  initiliaze(argv[1]);
  if(strcmp(argv[2],"mkdir")==0)
  {
    mymkdir(argv[3]);
  }
  if(strcmp(argv[2],"rmdir")==0)
  {
     myrmdir(argv[3]);
     }
  if(strcmp(argv[2],"write")==0)
  {
      write_disk(argv[3]);
  }
  if(strcmp(argv[2],"del")==0)
  {
      myremove(argv[3]);
  }

  print_directory_structure(3, 0);
 
  return 0;
}