#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - You are given the following helper functions:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists
 *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the homework spec.
 */

/* beargit init
 *
 * - Create .beargit directory
 * - Create empty .beargit/.index file
 * - Create .beargit/.prev file containing 0..0 commit id
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);
  
  write_string_to_file(".beargit/.prev", "0000000000000000000000000000000000000000");

  return 0;
}


/* beargit add <filename>
 * 
 * - Append filename to list in .beargit/.index if it isn't in there yet
 *
 * Possible errors (to stderr):
 * >> ERROR: File <filename> already added
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_add(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      fprintf(stderr, "ERROR: File %s already added\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}


/* beargit rm <filename>
 * 
 * See "Step 2" in the homework 1 spec.
 *
 */

int beargit_rm(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");
  char scan_set[15];
  sprintf(scan_set, " %%%u[^\n]", FILENAME_SIZE);
  char s[FILENAME_SIZE+1];
  int sen = 0;
  while(fscanf(findex, scan_set, s) > 0) {
    if (strcmp(s, filename) == 0) {
      sen = 1;
    } else {
      fprintf(fnewindex, "%s\n", s);
    }
  }
  
  fclose(findex);
  fclose(fnewindex);

  if(sen) {
    fs_mv(".beargit/.newindex", ".beargit/.index");
  } else {
    fs_rm(".beargit/.newindex");
    fprintf(stderr, "ERROR: File %s not tracked", filename);
    
    return 1;
  }

  return 0;
}

/* beargit commit -m <msg>
 *
 * See "Step 3" in the homework 1 spec.
 *
 */

const char* go_bears = "GO BEARS!";

int is_commit_msg_ok(const char* msg) {
  int i, j, sen = 0;
  for(i = 0; msg[i] != '\0'; i++){
    for(j = 0; msg[i+j] != '\0' && j < 9; j++){
      if(go_bears[j] == msg[i+j]) {
        sen += 1;
      } else {
        sen = 0;
        break;
      }
    }
    if(sen == 9) {
      return 1;
    } else {
      sen = 0;
    }
  }
  return 0;
}

void next_commit_id(char* commit_id) {
  char prev_commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", prev_commit_id, COMMIT_ID_SIZE);
  
  commit_id[COMMIT_ID_BYTES] = '\0';
  int i = COMMIT_ID_BYTES - 1, carry = 0;
  switch(prev_commit_id[i]) {
    case '0':
      commit_id[i] = '1';
      break;
    case '1':
      commit_id[i] = '6';
      break;
    case '6':
      commit_id[i] = 'c';
      break;
    case 'c':
      commit_id[i] = '1';
      carry = 1;
  }
  for(i -= 1; i > -1 ; i--) {
    if (carry) {
      switch(prev_commit_id[i]) {
        case '0':
          commit_id[i] = '1';
          carry = 0;
          break;
        case '1':
          commit_id[i] = '6';
          carry = 0;
          break;
        case '6':
          commit_id[i] = 'c';
          carry = 0;
          break;
        case 'c':
          commit_id[i] = '1';
          carry = 1;
      }
    } else if(prev_commit_id[i] == '0') {
      commit_id[i] = '1';
    } else {
      commit_id[i] = prev_commit_id[i];
    }
  }
}

int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR: Message must contain \"%s\"\n", go_bears);
    return 1;
  }

  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id);
  
  char prev_commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", prev_commit_id, COMMIT_ID_SIZE);

  char commit_dir[10+COMMIT_ID_SIZE];
  sprintf(commit_dir, ".beargit/%s/", commit_id);
  
  fs_mkdir(commit_dir);
  
  char dest[10+COMMIT_ID_SIZE+FILENAME_SIZE];
  
  strcpy(dest, commit_dir);
  strcat(dest, ".index");
  fs_cp(".beargit/.index", dest);
  
  strcpy(dest, commit_dir);
  strcat(dest, ".prev");
  fs_cp(".beargit/.prev", dest);
  
  FILE* findex = fopen(".beargit/.index", "r");
  char scan_set[15];
  sprintf(scan_set, " %%%u[^\n]", FILENAME_SIZE);
  char src[FILENAME_SIZE];
  while(fscanf(findex, scan_set, src) > 0) {
    strcpy(dest, commit_dir);
    strcat(dest, src);
    
    fs_cp(src, dest);
  }
  
  fclose(findex);
  write_string_to_file(".beargit/.prev", commit_id);
  
  strcpy(dest, commit_dir);
  strcat(dest, ".msg");
  write_string_to_file(dest, msg);


  return 0;
}

/* beargit status
 *
 * See "Step 1" in the homework 1 spec.
 *
 */

int beargit_status() {
  char scan_set[15];
  sprintf(scan_set, " %%%u[^\n]", FILENAME_SIZE);
  
  FILE *f;
  if((f = fopen(".beargit/.index", "r")) == NULL) {
    fprintf(stderr, "\nErro: .beargit/.index nao encontrado!\n");
    fprintf(stderr, "\n      Voce ja inicializou o\n");
    return 2;
  }
  
  printf("Tracked files:\n\n");
  
  int c = 0;
  char s[FILENAME_SIZE+1];
  while(fscanf(f, scan_set, s) > 0) {
    printf("\t%s\n", s);
    c++;
  }
  printf("\n%d files total\n", c);

  return 0;
}

/* beargit log
 *
 * See "Step 4" in the homework 1 spec.
 *
 */

int beargit_log() {

  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  if(!strcmp("0000000000000000000000000000000000000000", commit_id)) {
    fprintf(stderr, "ERROR: There are no commits!");
    return 1;
  }
  while(strcmp("0000000000000000000000000000000000000000", commit_id)) {
    putchar('\n');
    printf("commit %s\n", commit_id);
    char msg[MSG_SIZE];
    char commit_dir[14+COMMIT_ID_SIZE];
    sprintf(commit_dir, ".beargit/%s/", commit_id);
    
    char src[15+COMMIT_ID_SIZE];
    strcpy(src, commit_dir);
    strcat(src, ".msg");
    read_string_from_file(src, msg, MSG_SIZE);
    
    printf("\t%s\n", msg);
    
    strcpy(src, commit_dir);
    strcat(src, ".prev");
    
    read_string_from_file(src, commit_id, COMMIT_ID_SIZE);
  }
  
  

  return 0;
}
