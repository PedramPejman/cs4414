/**
Author: Pedram Pejman (pp5nv)
Source for Fat16BootSector and Fat16Entry structure: http://wiki.osdev.org/FAT#FAT_16
Source for reading bytes of file: http://stackoverflow.com/questions/22059189/read-a-file-as-byte-array
*/

#include "fat.h"

#define COMMAND_LENGTH 128
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

enum commands_t {
  LS,
  CD,
  CPIN,
  CPOUT,
  EXIT,
  INVALID
};

int init_command(Command *command, char *str) {
  memset(command, 0, sizeof(Command));
  int length = strlen(str);
  command->string = (char *)malloc((length + 1) * sizeof(char));
  strcpy(command->string, str);
}

// Initialize the word 
void init_word(Word *word, char *tok) {
  memset(word, 0, sizeof(Word));
  word->token = tok;
}

int destruct_command(Command *command) {
  Word *current = command->words;
  Word *prev;
  while (current) {
    prev = current;
    current = current->next;
    free(prev);
  }

  // Free underlying string
  free(command->string);
}


void print_chars(char *s) {
	while (*s != '\0') {
		printf("%c",*s);
		s++;
	}
}
void print_command(Command *command) {
	Word *word = command->words;
	printf("[");
	while (word) {
		print_chars(word->token);
		word = word->next;
		if (word) printf(" ");
	}
	printf("]\n");
}

// tokenize_command tokenizes the input based on empty spaces
int tokenize_command(Command *command) {
  // Error checking for beg and end tokens
  int end_position = strlen(command->string) - 2;

  // Start retrieving the tokens and init commands
  char *nextWord = strtok(command->string, "/ \n");
  if (!nextWord) {
    return 0;
  }
  Word *word = (Word *)malloc(sizeof(Word));
  word->token = nextWord;
  word->next = NULL;
  command->length=1;
  command->words = word;

  //Now go through the rest and append them to the linked list
  nextWord = strtok(NULL, "/ \n");
  while (nextWord) {
    word->next = (Word *)malloc(sizeof(Word));
    word = word->next;
    word->token = nextWord;
    word->next = NULL;
    command->length++;
    nextWord = strtok(NULL, "/ \n");
  }

  return 0;
}

// parses a string and returns a command type
int get_command(char *str) {
  if (strcmp(str, "ls") == 0) return LS;
  if (strcmp(str, "exit") == 0) return EXIT;
  if (strcmp(str, "cd") == 0) return CD;
  if (strcmp(str, "cpout") == 0) return CPOUT;
  if (strcmp(str, "coin") == 0) return CPIN;
  return INVALID;
}

// Executes the command
void execute_command(Cursor *cursor, Command *command) {
	Word *word = command->words;
  switch (get_command(word->token)) {
    case LS:
      cursor->current->children = fs_ls(cursor, word->next);
	  	display_children(cursor->current);
      break;
    case CD:
      fs_cd(cursor, word->next);
      break;
    case EXIT:
      exit(0);
    default:
      disp_error(CODE_5, NULL, 0);
      break;
  }
}

void run_shell(Cursor *cursor) {	
  FILE *fat_file = cursor->fat_file;
  BPB *boot_sector = cursor->bpb;
	// Read the root directory
	EntryNode *current = malloc(sizeof(EntryNode));
	current->isRoot = 1;
  cursor->current = current;
	current->children = fs_ls(cursor, NULL);
  
  //display_children(current);
	char buffer[COMMAND_LENGTH];
  	Command command;
  	while (!feof(stdin)) {
	    // Prompt the user
	    printf(":/>");
	    fflush(stdout);

	    // Read from stdin
	    memset(buffer, 0, COMMAND_LENGTH);
	    fgets(buffer, COMMAND_LENGTH, stdin);

	    // Initialize the command
	    init_command(&command, buffer);
	    
	    // Parse our input and check for errors
	    tokenize_command(&command);

	    //DO IT
	    execute_command(cursor, &command);

	    // Deallocate any internal memory
	    destruct_command(&command);
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
	return 0;
}











