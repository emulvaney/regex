/* A Regular Expression Library - Example Grep Implementation
 * Copyright (c) 2012 Eric Mulvaney
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "core.h"
#include "debug.h"

static int
output(char *str, size_t len)
{
  int rc;

  do { rc = fwrite(str, 1, len, stdout); }
  while(rc == EOF && errno == EINTR);
  return rc;
}

static int
print(char *line, char **captures, char *file, char *fmt)
{
  size_t len;
  int rc, i;
  
  if(file) {
    do { rc = printf("%s:", file); }
    while(rc < 0 && errno == EINTR);
    if(rc < 0) return rc;
  }
  if(!fmt) {
    do { rc = puts(line); }
    while(rc == EOF && errno == EINTR);
    return rc;
  }
  for(;;) {
    if((len = strcspn(fmt, "$")) != 0) {
      if(output(fmt, len) == EOF)
	return EOF;
      fmt += len;
    }
    if(!fmt[0])
      break;
    if(isdigit(fmt[1])) {
      i = (fmt[1] - '0') * 2;
      fmt += 2;
      if(captures[i] && captures[i+1]) {
	len = captures[i+1] - captures[i];
	if(output(captures[i], len) == EOF)
	  return EOF;
      }
    } else {
      switch(fmt[1]) {
      case '$':  fmt++;  /* no break */
      case '\0': len = 1; break;
      default:   len = 2;
      }
      if(output(fmt, len) == EOF)
	return EOF;
      fmt += len;
    }
  }
  return output("\n", 1);
}

static int
readline(char *buf, size_t len, FILE* fin)
{
  while(fgets(buf, len, fin) == NULL) {
    if(errno != EINTR)
      return feof(fin) ? 0 : -1;
  }
  len = strlen(buf);
  if(buf[len-1] == '\n')
    buf[--len] = '\0';
  return 1;
}

static int
grep(struct Program *prog, char *infile, char *outfmt)
{
  char buf[BUFSIZ], *captures[20];
  int rc, matched = 0;
  FILE *fin = stdin;

  if(infile && !strcmp(infile, "-"))
    infile = "(standard input)";
  else if(infile) {
    while((fin = fopen(infile, "r")) == NULL)
      if(errno != EINTR) { perror(infile); return -1; }
  }
  while((rc = readline(buf, sizeof buf, fin)) > 0) {
    if((rc = vm(prog, buf, captures)) > 0) {
      matched = 1;
      rc = print(buf, captures, infile, outfmt);
    }
    if(rc < 0) break;
  }
  if(rc < 0) {
    perror(infile ? infile : "(standard input)");
    matched = -1;
  }
  if(fin && fin != stdin) {
    while(fclose(fin) == EOF)
      if(errno != EINTR) { perror(infile); return -1; }
  }
  return matched;
}

int
main(int argc, char *argv[])
{
  struct Program prog;
  char *outfmt = NULL;
  int debug = 0, matched = 0, errors = 0;
  int i, opt, rc;

  while((opt = getopt(argc, argv, "do:")) != -1) {
    switch(opt) {
    case 'd': debug  = 1;      break;
    case 'o': outfmt = optarg; break;
    default: goto badargs;
    }
  }
  if((i=optind) >= argc) {
  badargs:
    fprintf(stderr, "usage: %s [-d] [-o fmt] (regex) [files...]\n",
	    argv[0]);
    return 2;
  }
  rc = compile(&prog, argv[i++]);
  if(rc) { perror("compile"); return 2; }
  if(debug) printprogram(stderr, &prog);
  do {
    rc = grep(&prog, argv[i], outfmt);
    if     (rc > 0) matched = 1;
    else if(rc < 0) errors  = 1;
  } while(++i < argc);
  freeprogram(&prog);
  return errors ? 2 : !!matched;
}
