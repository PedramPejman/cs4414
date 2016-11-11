#include "shell.h"
// Initialize the word 
void init_word(Word *word, char *tok) {
  memset(word, 0, sizeof(Word));
  word->token = tok;
}

int init_input(Input *input, char *str) {
  memset(input, 0, sizeof(Input));
  int length = strlen(str);
  input->string = (char *)malloc((length + 1) * sizeof(char));
  strcpy(input->string, str);
}

int destruct_input(Input *input) {
  Word *current = input->words;
  Word *prev;
  while (current) {
    prev = current;
    current = current->next;
    free(prev);
  }

  // Free underlying string
  free(input->string);
}


void print_chars(char *s) {
	while (*s != '\0') {
		printf("%c",*s);
		s++;
	}
}
void print_input(Input *input) {
	Word *word = input->words;
	printf("[");
	while (word) {
		print_chars(word->token);
		word = word->next;
		if (word) printf(" ");
	}
	printf("]\n");
}

// tokenize_input tokenizes the input based on empty spaces
int tokenize_input(Input *input) {
  // Error checking for beg and end tokens
  int end_position = strlen(input->string) - 2;

  // Start retrieving the tokens and init inputs
  char *nextWord = strtok(input->string, "/ \n");
  if (!nextWord) {
    return 0;
  }
  Word *word = (Word *)malloc(sizeof(Word));
  word->token = nextWord;
  word->next = NULL;
  input->length=1;
  input->words = word;

  //Now go through the rest and append them to the linked list
  nextWord = strtok(NULL, "/ \n");
  while (nextWord) {
    word->next = (Word *)malloc(sizeof(Word));
    word = word->next;
    word->token = nextWord;
    word->next = NULL;
    input->length++;
    nextWord = strtok(NULL, "/ \n");
  }

  return 0;
}

// parses a string and returns a input type
int get_input(char *str) {
  if (strcmp(str, "ls") == 0) return LS;
  if (strcmp(str, "exit") == 0) return EXIT;
  if (strcmp(str, "cd") == 0) return CD;
  if (strcmp(str, "cpout") == 0) return CPOUT;
  if (strcmp(str, "cpin") == 0) return CPIN;
  return INVALID;
}

// Executes the input
void execute_input(Cursor *cursor, Input *input) {
	EntryNode *parent_node;
  Word *word = input->words;

  switch (get_input(word->token)) {
    case LS:
      parent_node->children = fs_ls(cursor, word->next);
	  	display_children(parent_node);
      break;
    case CD:
      fs_cd(cursor, word->next);
      break;
    case CPIN:
      fs_cpin(cursor, word->next);
      break;
    case EXIT:
      exit(0);
    default:
      disp_error(CODE_5, NULL, 0);
      break;
  }
}

void print_shell_prompt(Cursor *cursor) {
  printf(":");
  Word *head = cursor->path;
  if (!cursor->path) printf("/");
  while (head) {
    printf("/%s", head->token);
    head = head->next;
  }
  printf(">");
}

