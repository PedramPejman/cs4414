#include "shell.h"
// Initialize the word 
void init_word(Word *word, char *tok) {
  memset(word, 0, sizeof(Word));
  word->token = tok;
}

int init_command(Command *command, char *str) {
  memset(command, 0, sizeof(Command));
  int length = strlen(str);
  command->string = (char *)malloc((length + 1) * sizeof(char));
  strcpy(command->string, str);
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
	EntryNode *parent_node;
  Word *word = command->words;

  switch (get_command(word->token)) {
    case LS:
      parent_node->children = fs_ls(cursor, word->next);
	  	display_children(parent_node);
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



