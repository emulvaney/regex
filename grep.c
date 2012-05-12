/* My grep Implementation
 * Copyright (c) 2012 Eric Mulvaney
 * TODO: Add GPL3 bit here
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"

int
main(int argc, char *argv[])
{
  struct Program prog;
  char buf[BUFSIZ], *captures[20];
  size_t len;
  int rc;

  if(argc != 2) {
    fprintf(stderr, "usage: %s (regex)\n", argv[0]);
    return EXIT_FAILURE;
  }
  rc = compile(&prog, argv[1]);
  if(rc) { perror("parse"); return EXIT_FAILURE; }
  while(fgets(buf, sizeof buf, stdin) != NULL || errno == EINTR) {
    if(errno == EINTR)
      continue;
    len = strlen(buf);
    if(buf[len-1] == '\n')
      buf[--len] = '\0';
    rc = vm(&prog, buf, captures);
    if(rc < 0) { perror("vm"); return EXIT_FAILURE; }
    if(rc > 0) {
      do { rc = puts(buf); } while(rc == EOF && errno == EINTR);
      if(rc == EOF) { perror("puts"); return EXIT_FAILURE; }
    }
  }
  freeprogram(&prog);
  return EXIT_SUCCESS;
}
