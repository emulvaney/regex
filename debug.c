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

int
printprogram(FILE *stream, struct Program *prog)
{
  struct Inst *pc0, *pc;
  int i;

  if(stream == NULL || prog == NULL || prog->size < 1)
    return (errno=EINVAL, -1);
  pc0 = prog->code;
  for(i = 0; i < prog->size; i++) {
    pc = &prog->code[i];
    fprintf(stream, "%03d ", (int)(pc - pc0));
    switch(pc->opcode) {
    case Char:
      fprintf(stream, "Char %c\n", pc->args.c);
      break;
    case AnyChar:
      fprintf(stream, "AnyChar\n");
      break;
    case CharSet:
      fprintf(stream, "CharSet [...]\n");
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
