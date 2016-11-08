#ifndef FAT_H
#define FAT_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#define BOOT_SECTOR_LENGTH 512
#define BOOT_SECTOR_OFFSET 0x0
#define BYTES_PER_SECTOR 512
#define UNUSED_FLAG 0xE5
#define NUM_FATS 2
#define MAX_NAME_LENGTH 11
#define SPACE 0x20

// LinkedList structure for modeling tokens
typedef struct word_t {
  char         *token;
  struct word_t *next;
} Word;

typedef struct command_t{
  // First word is the command
  // All words thereafter are the argument tokenized
  // ex: ls d1/d2 := [ls]->[d1]->[d2]
  Word *words;
  char    *string;
  char    *cmd;
  int     length;
} Command;

enum attributes_t{
    DIR_ATTR_READONLY = 1 << 0,
    DIR_ATTR_HIDDEN   = 1 << 1,
    DIR_ATTR_SYSTEM   = 1 << 2,
    DIR_ATTR_VOLUMEID = 1 << 3,
    DIR_ATTR_DIRECTORY= 1 << 4,
    DIR_ATTR_ARCHIVE  = 1 << 5,
    DIR_ATTR_LFN      = 0xF
};

typedef enum {
    CODE_0, // No FAT image provided
    CODE_1, // file could not be opened
    CODE_2, // Error seeking file
    CODE_3, // Error reading file
    CODE_4, // Invalid FAT information
    CODE_5, // Invalid command
    CODE_6, // Directory does not exist
} Error;

typedef struct dir_list_t EntryNode;

typedef struct {
  unsigned char     bootjmp[3];
  unsigned char     oem_name[8];
  unsigned short          bytes_per_sector;
  unsigned char   sectors_per_cluster;
  unsigned short    reserved_sector_count;
  unsigned char   table_count;
  unsigned short    root_entry_count;
  unsigned short    total_sectors_16;
  unsigned char   media_type;
  unsigned short    table_size_16;
  unsigned short    sectors_per_track;
  unsigned short    head_side_count;
  unsigned int    hidden_sector_count;
  unsigned int    total_sectors_32;
                           
  unsigned char   extended_section[54];
} __attribute((packed)) BPB;

typedef struct {
    unsigned char name[8];
    unsigned char ext[3];
    uint8_t attributes;
    unsigned char reserved[10];
    unsigned short modify_time;
    unsigned short modify_date;
    unsigned short starting_cluster;
    unsigned long size;
} __attribute((packed)) Fat16Entry;

typedef struct dir_list_t {
    Fat16Entry *entry;

    //if a directory
    bool isDirectory;
    EntryNode *children;
    
    bool isRoot;
    //if part of a chain
    struct dir_list_t *next;
} EntryNode;

typedef struct {
  FILE *fat_file;
  BPB *bpb;
  EntryNode *current;
} Cursor;

/************ Helpers ***********/


// Displays an error message and kills program if fatal
void disp_error(Error code, void *arg, int fatal);

bool read_bytes(FILE *file, unsigned int offset, unsigned int length, void *buf);

EntryNode *fs_ls(Cursor *cursor, Word *args);

void fs_cd(Cursor *cursor, Word *args);

void print_node_name(EntryNode *node);

void display_children(EntryNode *node);

void init_boot_sector(BPB *boot_sector, FILE *file);
//BPB functions


#endif
