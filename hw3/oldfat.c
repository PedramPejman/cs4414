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
    return bpb->rsvdSecCnt;
}

uint32_t fat_address(BPB *bpb) {
	return first_fat_sector(bpb)*bpb->bytsPerSec;
}

uint32_t root_directory_sector()

uint32_t root_address(BPB *bpb) {
    return fat_address(bpb) + bpb->numFATs * bpb->FATSz16 * bpb->bytsPerSec;
}

uint32_t data_address(BPB *bpb) {
    return root_address(bpb) + bpb->rootEntCnt * 32;
}

uint32_t data_sector_count(BPB *bpb) {
    return bpb->totSec32 - data_address(bpb) / bpb->bytsPerSec;
}

uint16_t get_next_cluster(FILE *f, BPB *bpb, uint16_t current_cluster) {
	uint16_t retval;
	read_bytes(f, fat_address(bpb) + current_cluster*2, sizeof(retval), &retval);
	return retval;
}

void init_boot_sector(BPB *boot_sector, FILE *file) {
	read_bytes(file, BOOT_SECTOR_OFFSET, sizeof(*boot_sector), boot_sector);
	if(boot_sector->bytsPerSec != BYTES_PER_SECTOR || boot_sector->numFATs != NUM_FATS)
        disp_error(CODE_4, NULL, 1);
}

void print_cluster(FILE *f, BPB *bpb, Fat16Entry *entry) {
    const uint32_t cluster_size = bpb->bytsPerSec * bpb->secPerClus;
    uint16_t c;
    printf("size:%d\n",entry->starting_cluster);fflush(stdout);

    for(c = entry->starting_cluster; c < 0xFFF8; c = get_next_cluster(f, bpb, c)) {
        printf("next\n");fflush(stdout);
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

//TODO: FIX
void format_entry(Fat16Entry *entry) {
	// Put extension
	int size=0;
	while (entry->name[size] != SPACE && size<8) {
    size++;
	}
  if (size < 8) entry->name[size] = '\0';
  //print_charss(entry->name);
}

int compare_directory_name (char *s1, char *s2) {
  int i=0;
  while (i < 8) {
    if (s1[i] != s2[i]) {printf("[%x][%x]",s1[i],s2[i]);return i;}
    if (s1[i] == '\0') return 0;
    i++;
  }
  return 0;
}

EntryNode *get_entry_for(EntryNode *current, char* name) {
	EntryNode *child = current->children;
	while (child) {
	  if (compare_directory_name(child->entry->name, name) == 0) {
      return child; 
    }
    child = child->next;
	}
}

EntryNode *fs_ls(FILE *fat_file, BPB *boot, EntryNode *current, Word *args) {
  uint32_t offset;

  if (current->isRoot) {
    offset = root_address(boot);
  } else {
    uint32_t cluster = current->entry->starting_cluster;
    uint32_t root_dir_sectors = ((boot->rootEntCnt * 32) + (boot->bytsPerSec - 1)) / boot->bytsPerSec;
    uint32_t first_data_sector = boot->rsvdSecCnt + (boot->numFATs * fat_size) + root_dir_sectors;
    offset = ((cluster - 2) * fat_boot->sectors_per_cluster) + first_data_sector;
    offset *= boot->bytsPerSec;
  }

	int i=0;
	EntryNode *head;
	EntryNode *previous=NULL;
	while (true) {
		Fat16Entry *fatEntry = malloc(sizeof(Fat16Entry));
		read_bytes(fat_file, offset + (i++)*32, sizeof(Fat16Entry), fatEntry);
		if (fatEntry->name[0] == 0) {
			free(fatEntry);
			break;
		}
		if (fatEntry->name[0] == UNUSED_FLAG || 
			fatEntry->attributes & DIR_ATTR_LFN) {
			free(fatEntry);
			continue;
		}	

		EntryNode *node = malloc(sizeof(EntryNode));
		node->entry=fatEntry;
		if (fatEntry->attributes & DIR_ATTR_DIRECTORY) 
			node->isDirectory = 1;
	  format_entry(fatEntry);
		if (previous != NULL) 
			previous->next = node;
		else
			head = node;
		previous = node;		
	}

  if (args) {
		current->children = head;
    EntryNode *next_dir = get_entry_for(current, args->token);
    //      printf("CC[");fflush(stdout);
  //    print_cluster(fat_file,boot,next_dir->entry);
//      printf("]CC");fflush(stdout);

    return fs_ls(fat_file, boot, next_dir, args->next);
	}
  
  return head;
 }




