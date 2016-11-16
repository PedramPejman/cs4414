/**
Author: Pedram Pejman (pp5nv)
Source for Fat16BootSector and Fat16Entry structure: http://wiki.osdev.org/FAT#FAT_16
Source for reading bytes of file: http://stackoverflow.com/questions/22059189/read-a-file-as-byte-array
*/

#include "fat.h"
#include "shell.h"

void run_shell(Cursor *cursor) {	
  FILE *fat_file = cursor->fat_file;
  BPB *boot_sector = cursor->bpb;

	// Read the root directory
	EntryNode *current = malloc(sizeof(EntryNode));
	current->isRoot = 1;
  cursor->current = current;
  cursor->path = malloc(sizeof(Word));
  cursor->path->token = "/";
  cursor->path->next = NULL;
	current->children = fs_ls(cursor, NULL);
  
	char buffer[COMMAND_LENGTH];
  	Input input;
  	while (!feof(stdin)) {

	    // Prompt the user
	    printf(SHELL_PROMPT);
	    fflush(stdout);

	    // Read from stdin
	    memset(buffer, 0, COMMAND_LENGTH);
	    fgets(buffer, COMMAND_LENGTH, stdin);

	    // Initialize the input
	    init_input(&input, buffer);
	    
	    // Parse our input and check for errors
	    tokenize_input(&input);

	    //DO IT
	    execute_input(cursor, &input);

	    // Deallocate any internal memory
	    destruct_input(&input);
  	}

}

int main(int argc, char **argv) {
	// Make sure we are supplied a filename
	if (argc < 2) disp_error(CODE_0, NULL, 1);
	char *filename = argv[1];

	FILE *fat_file = fopen(filename, "r");
	if (fat_file == NULL) {
		disp_error(CODE_1, filename, 1);
	}

	// Initialize and setup BootSector
	BPB boot_sector;
	init_boot_sector(&boot_sector, fat_file);
	
  // Initialize and setup cursor
  Cursor *cursor = malloc(sizeof(Cursor));
  cursor->fat_file = fat_file;
  cursor->bpb = &boot_sector;

	// Run the actual shell
	run_shell(cursor);
	
	// Clean up
	fclose(fat_file);

  //free_cursor(cursor);
	return 0;
}











