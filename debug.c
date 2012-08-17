/* Debug Routines
 * Copyright (c) 2012 Eric Mulvaney
 * TODO: Add GPL3 bit here
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
