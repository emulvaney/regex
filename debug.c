/* A Regular Expression Library - Debug Routines
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

#include <errno.h>
#include <stdlib.h>
#include "core.h"
#include "debug.h"

static char*
range(char *s, int first, int last)
{
  *s++ = first;
  if(last > first) {
    if(last > first+1)
      *s++ = '-';
    *s++ = last;
  }
  return s;
}

static char*
charset(char *buf, struct Inst *pc)
{
  unsigned *set = pc->args.set.charset;
  unsigned mask = pc->args.set.mask;
  unsigned marked = mask;
  int c, first=0, last=0;
  char *s = buf;

  if(set[1] & mask) {  /* likely [^...] */
    marked = 0;
    *s++ = '^';
  }
  if((set[']'] & mask) == marked)
    *s++ = ']';
  for(c=1; c < UCHAR_MAX; c++) {
    if((set[c] & mask) != marked || c == ']')
      continue;
    else if(last && last < c-1) {
      s = range(s, first, last);
      first = last = c;
    } else {
      last = c;
      if(!first) first = c;
    }
  }
  if(last) s = range(s, first, last);
  *s = '\0';
  return buf;
}

int
printprogram(FILE *stream, struct Program *prog)
{
  struct Inst *pc0, *pc;
  char buf[UCHAR_MAX];
  int i;

  if(stream == NULL || prog == NULL || prog->size < 1)
    return (errno=EINVAL, -1);
  pc0 = prog->code;
  for(i = 0; i < prog->size; i++) {
    pc = &prog->code[i];
    fprintf(stream, "%03d ", (int)(pc - pc0));
    switch(pc->opcode) {
    case CharAlt:
      fprintf(stream, "CharAlt %c %c\n", pc->args.chr.c, pc->args.chr.alt);
      break;
    case Char:
      fprintf(stream, "Char %c\n", pc->args.chr.c);
      break;
    case AnyChar:
      fprintf(stream, "AnyChar\n");
      break;
    case CharSet:
      fprintf(stream, "CharSet [%s]\n", charset(buf, pc));
      break;
    case Match:
      fprintf(stream, "Match\n");
      break;
    case MatchEnd:
      fprintf(stream, "MatchEnd\n");
      break;
    case Jump:
      fprintf(stream, "Jump %03d\n", (int)(pc->args.next.x - pc0));
      break;
    case Split:
      fprintf(stream, "Split %03d %03d\n",
	      (int)(pc->args.next.x - pc0),
	      (int)(pc->args.next.y - pc0));
      break;
    case Save:
      fprintf(stream, "Save %d\n", pc->args.i);
      break;
    default:
      abort();
    }
  }
  return 0;
}
