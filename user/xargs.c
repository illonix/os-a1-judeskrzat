#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

static void
usage(void)
{
  fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
  exit(1);
}

static int
parse_count(const char *text, int *count)
{
  int value = 0;

  if (*text == '\0')
    return -1;
  for (; *text; text++) {
    int digit;

    if (*text < '0' || *text > '9')
      return -1;
    digit = *text - '0';
    if (value > (0x7fffffff - digit) / 10)
      return -1;
    value = value * 10 + digit;
  }
  if (value < 1)
    return -1;
  *count = value;
  return 0;
}

static int
append_char(char **buffer, int *capacity, int *length, char ch)
{
  int new_capacity;
  char *new_buffer;

  if (*length >= 0x7ffffffe)
    return -1;
  if (*capacity <= *length + 1) {
    new_capacity = *capacity ? *capacity : 64;
    while (new_capacity <= *length + 1) {
      if (new_capacity > 0x3fffffff)
        return -1;
      new_capacity *= 2;
    }
    new_buffer = malloc(new_capacity);
    if (new_buffer == 0)
      return -1;
    if (*length > 0)
      memmove(new_buffer, *buffer, *length);
    if (*buffer)
      free(*buffer);
    *buffer = new_buffer;
    *capacity = new_capacity;
  }
  (*buffer)[(*length)++] = ch;
  return 0;
}

static int
finish_word(char **buffer, int *capacity, int *length, char **words,
            int *word_count, int word_limit)
{
  if (*length == 0)
    return 0;
  if (*word_count >= word_limit)
    return -1;
  (*buffer)[*length] = '\0';
  words[(*word_count)++] = *buffer;
  *buffer = 0;
  *capacity = 0;
  *length = 0;
  return 0;
}

static void
free_words(char **words, int *word_count)
{
  int i;

  for (i = 0; i < *word_count; i++) {
    free(words[i]);
    words[i] = 0;
  }
  *word_count = 0;
}

static int
run_command(char **argv, int command_index, int fixed_count, char **words,
            int word_count)
{
  char *command_argv[MAXARG + 1];
  int status;
  int pid;
  int i;

  for (i = 0; i < fixed_count; i++)
    command_argv[i] = argv[command_index + i];
  for (i = 0; i < word_count; i++)
    command_argv[fixed_count + i] = words[i];
  command_argv[fixed_count + word_count] = 0;

  pid = fork();
  if (pid < 0)
    return -1;
  if (pid == 0) {
    exec(command_argv[0], command_argv);
    exit(1);
  }
  status = -1;
  if (wait(&status) < 0 || status != 0)
    return -1;
  return 0;
}

static int
flush_words(char **argv, int command_index, int fixed_count, char **words,
            int *word_count)
{
  int result;

  if (*word_count == 0)
    return 0;
  result = run_command(argv, command_index, fixed_count, words, *word_count);
  free_words(words, word_count);
  return result;
}

int
main(int argc, char *argv[])
{
  char *words[MAXARG];
  char *word_buffer = 0;
  char ch;
  int command_index = 1;
  int fixed_count;
  int word_count = 0;
  int word_capacity = 0;
  int word_length = 0;
  int word_limit;
  int batch_size = 0;
  int use_batch_size = 0;
  int read_count;

  if (argc < 2)
    usage();
  if (strcmp(argv[1], "-n") == 0) {
    if (argc < 4 || parse_count(argv[2], &batch_size) < 0)
      usage();
    command_index = 3;
    use_batch_size = 1;
  }
  if (command_index >= argc)
    usage();

  fixed_count = argc - command_index;
  if (fixed_count > MAXARG)
    usage();
  word_limit = MAXARG - fixed_count;
  if (use_batch_size && batch_size < word_limit)
    word_limit = batch_size;

  memset(words, 0, sizeof(words));
  for (;;) {
    read_count = read(0, &ch, 1);
    if (read_count < 0)
      exit(1);
    if (read_count == 0) {
      if (finish_word(&word_buffer, &word_capacity, &word_length, words,
                      &word_count, word_limit) < 0 ||
          flush_words(argv, command_index, fixed_count, words, &word_count) < 0)
        exit(1);
      break;
    }

    if (ch == '\0')
      exit(1);
    if (ch == ' ' || ch == '\t' || ch == '\n') {
      if (finish_word(&word_buffer, &word_capacity, &word_length, words,
                      &word_count, word_limit) < 0)
        exit(1);
      if ((use_batch_size && word_count == word_limit) ||
          (!use_batch_size && ch == '\n' && word_count > 0)) {
        if (flush_words(argv, command_index, fixed_count, words, &word_count) < 0)
          exit(1);
      }
      continue;
    }

    if (word_length == 0 && word_count >= word_limit)
      exit(1);
    if (append_char(&word_buffer, &word_capacity, &word_length, ch) < 0)
      exit(1);
  }

  if (word_buffer)
    free(word_buffer);
  exit(0);
}