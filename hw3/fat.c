#include "fat.h"

// Displays an error message and kills program if fatal
void disp_error(Error code, void *arg, int fatal) {
	printf("Error %d\n", code);
	if (fatal) exit(0);
}

bool read_bytes(FILE *file, unsigned int offset, unsigned int length, void *buf) {
	if (fseek(file, offset, SEEK_SET != 0)) {
		disp_error(CODE_2, NULL, 0);
		return false;
	}
	if (fread(buf, 1, length, file) != length && ferror(file)) {
		disp_error(CODE_3, NULL, 0);
	}
	return true;
}



/********** BPB functions ***********/

uint32_t first_fat_sector(BPB *bpb) {
    return bpb->reserved_sector_count;
}

uint32_t fat_size(BPB *bpb) {
  return bpb->table_size_16;
}

uint32_t fat_address(BPB *bpb) {
	return first_fat_sector(bpb)*bpb->bytes_per_sector;
}

uint32_t root_address(BPB *bpb) {
    return fat_address(bpb) + bpb->table_count * bpb->table_size_16 * bpb->bytes_per_sector;
}

uint32_t data_address(BPB *bpb) {
    return root_address(bpb) + bpb->root_entry_count * 32;
}

uint32_t data_sector_count(BPB *bpb) {
    return bpb->total_sectors_32 - data_address(bpb) / bpb->bytes_per_sector;
}

uint16_t get_next_cluster(FILE *f, BPB *bpb, uint16_t current_cluster) {
	uint16_t retval;
	read_bytes(f, fat_address(bpb) + current_cluster*2, sizeof(retval), &retval);
	return retval;
}

void init_boot_sector(BPB *boot_sector, FILE *file) {
	read_bytes(file, BOOT_SECTOR_OFFSET, sizeof(*boot_sector), boot_sector);
	if(boot_sector->bytes_per_sector != BYTES_PER_SECTOR || boot_sector->table_count != NUM_FATS)
        disp_error(CODE_4, NULL, 1);
}

void print_cluster(FILE *f, BPB *bpb, Fat16Entry *entry) {
    const uint32_t cluster_size = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    uint16_t c;

    for(c = entry->starting_cluster; c < 0xFFF8; c = get_next_cluster(f, bpb, c)) {
        char buf[cluster_size];
        read_bytes(f, data_address(bpb) + (c - 2) * cluster_size, sizeof(buf), buf);
        printf("%.*s", (int)(sizeof(buf)), buf);
    }
}

void print_node_name(EntryNode *node) {
	if (node->isDirectory) printf("D ");
	else printf("F ");

	int i=0;
	while (node->entry->name[i] != '\0' && i < 8) {
		printf("%c",node->entry->name[i++]);
	}
  if (!node->isDirectory) printf(".%c%c%c",node->entry->ext[0]
    , node->entry->ext[1], node->entry->ext[2]);
	printf("\n");
}

void display_children(EntryNode *node) {
	EntryNode *head = node->children;
	while (head != NULL) {
		print_node_name(head);
		head = head->next;
	}
}

void print_name(Fat16Entry* entry) { 
	int  i=0;
	while(entry->name[i] != '\0') {
		printf("%c", entry->name[i++]);
	}
	printf("\n");
}

void print_charss(char *s) {
	printf("[");
	while (*s != '\0') {
		printf("%c",*s);
		s++;
	}
	printf("]");
}

// Formats an entry's name by replacing the first space with '\0'
void format_entry(Fat16Entry *entry) {
	int size=0;
	while (entry->name[size] != SPACE && size<8) {
    size++;
	}
  if (size < 8) entry->name[size] = '\0';
}

// Compares two directories' names
int compare_directory_name (char *s1, char *s2) {
  int i=0;
  while (i < 8) {
    if (s1[i] != s2[i]) return 1;
    if (s1[i] == '\0') return 0;
    i++;
  }
  return 0;
}

// Searches for a directory named name in the children of current
EntryNode *get_entry_for(EntryNode *current, char* name) {
	EntryNode *child = current->children;
	while (child) {
	  if (compare_directory_name(child->entry->name, name) == 0) {
      return child; 
    }
    child = child->next;
	}
  return NULL;
}

uint32_t get_offset(FILE *fat_file, BPB *boot, EntryNode *current) {
  uint32_t offset;
  if (current->isRoot || current->entry->starting_cluster == 0) {
    offset = root_address(boot);
  } else {
    uint32_t cluster = current->entry->starting_cluster;
    uint32_t root_dir_sectors = ((boot->root_entry_count * 32) + (boot->bytes_per_sector - 1)) / boot->bytes_per_sector;
    uint32_t first_data_sector = boot->reserved_sector_count + (boot->table_count * fat_size(boot)) + root_dir_sectors;
    offset = ((cluster - 2) * boot->sectors_per_cluster) + first_data_sector;
    offset *= boot->bytes_per_sector; 
  }
  return offset;
}

EntryNode *fs_ls(Cursor *cursor, Word *args) {
  FILE *fat_file = cursor->fat_file;
  BPB *boot = cursor->bpb;
  EntryNode *current = cursor->current;

  uint32_t offset = get_offset(fat_file, boot, current);
	int i=0;
	EntryNode *head;
	EntryNode *previous=NULL;
	while (true) {
		Fat16Entry *fatEntry = malloc(sizeof(Fat16Entry));
		read_bytes(fat_file, offset + (i++)*32, sizeof(Fat16Entry), fatEntry);

    // Make sure it's a valid entry. Discard if not
		if (fatEntry->name[0] == 0) {
			free(fatEntry);
			break;
		}
		if (fatEntry->name[0] == UNUSED_FLAG || 
			fatEntry->attributes & DIR_ATTR_LFN) {
			free(fatEntry);
			continue;
		}	

    // Wrap the FATEntry in an EntryNode
		EntryNode *node = malloc(sizeof(EntryNode));
		node->entry=fatEntry;
		if (fatEntry->attributes & DIR_ATTR_DIRECTORY) 
			node->isDirectory = 1;
	  format_entry(fatEntry);
		
    // Update linked list
    if (previous != NULL) 
			previous->next = node;
		else
			head = node;

		previous = node;		
	}

  // If there are arguments, recurse down without mutating 
  // the state of the cursor
  if (args) {
		current->children = head;
    EntryNode *next_dir = get_entry_for(current, args->token);
    if (!next_dir) {
      disp_error(CODE_6, NULL, 0);
      free(next_dir);
      return;
    }
    cursor->current = next_dir;
    EntryNode *result = fs_ls(cursor, args->next);
    free(next_dir);
    cursor->current = current;
    return result;
	}
  
  return head;
}

void fs_cd(Cursor *cursor, Word *path) {
  if (!cursor->current->children) 
    cursor->current->children = fs_ls(cursor, NULL);

  EntryNode *next_dir = get_entry_for(cursor->current, path->token);
  if (!next_dir) {
    disp_error(CODE_6, NULL, 0);
    return;
  }
  cursor->current = next_dir;
  
  // Update cursor path
  if (strcmp(path->token, "..") == 0) {
  /*  Word *prev = path;
    Word *next = path->next;
    while (next) {
      prev = next;
      next = next->next; 
    }
    prev->next=NULL;*/
  }
  
  else {
    print_word(cursor->path);
    Word *head = cursor->path;
    while (head->next) {head = head->next;}
    head->next = path;
    head->next->next = NULL;
    print_word(cursor->path);
  }

  if (path->next) fs_cd(cursor, path->next);
}

void print_word(Word *word) {
  printf("[");
  while (word) {
    printf("%s", word->token);
    word = word->next;
  }
  printf("]\n");
}

void fs_cpin(Cursor *cursor, Word *args) {
  
}

void fs_cpout(Cursor *cursor, Word *args) {
  
}

