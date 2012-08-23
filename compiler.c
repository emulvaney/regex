/* A Regular Expression Library - Bytecode Compiler
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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"

/* Six instructions are required to match the smallest regex:
 *     000 Split 003 001
 *     001 AnyChar
 *     002 Jump 000
 *     003 Save 0
 *     004 Save 1
 *     005 Match
 * Each node in an AST corresponding to a character in a regex can add
 * at most two instructions (see compiletree()); nodes that do not
 * correspond to characters in the original regex add none.
 * Therefore, an upper-bound on the program size is 2*N+6 for a regex
 * of size N (see compile()).
 */
enum { MIN_CODESIZE=6 };

enum { MAX_SAVE=9 };  /* capture only $0 through $9 */

struct Flags {
  int matchend;  /* match to end of string */
  int nextsave;  /* 0..MAX_SAVE for Save instructions */
};

static struct Inst*
compiletree(struct Inst *pc, struct Flags *flags, struct AST *t)
{
  struct Inst *next;
  int savepoint;

  for(;;) {
    switch(t->op) {
    case Epsilon:
      goto done;
    case Onechar:
      pc->opcode = Char;
      pc->args.c = t->args.c;
      pc++;
      goto done;
    case Anychar:
      pc->opcode = AnyChar;
      pc++;
      goto done;
    case Charset:
      pc->opcode = CharSet;
      pc->args.set.mask    = t->args.set.mask;
      pc->args.set.charset = t->args.set.charset;
      pc++;
      goto done;
    case Dollar:
      flags->matchend = 1;
      goto done;
    case Concat:
      pc = compiletree(pc, flags, t->args.next.x);
      t = t->args.next.y;
      break;
    case Either:
      pc->opcode = Split;
      pc->args.next.x = pc+1;
      next = compiletree(pc+1, flags, t->args.next.x);
      pc->args.next.y = next+1;
      next->opcode = Jump;
      pc = next->args.next.x = compiletree(next+1, flags, t->args.next.y);
      goto done;
    case Optional:
      pc->opcode = Split;
      pc->args.next.x = pc+1;
      pc = pc->args.next.y = compiletree(pc+1, flags, t->args.next.x);
      goto done;
    case WeakOpt:
      pc->opcode = Split;
      pc->args.next.y = pc+1;
      pc = pc->args.next.x = compiletree(pc+1, flags, t->args.next.x);
      goto done;
    case Star:
      next = compiletree(pc+1, flags, t->args.next.x);
      pc->opcode = Split;
      pc->args.next.x = pc+1;
      pc->args.next.y = next+1;
      next->opcode = Jump;
      next->args.next.x = pc;
      pc = next + 1;
      goto done;
    case WeakStar:
      next = compiletree(pc+1, flags, t->args.next.x);
      pc->opcode = Split;
      pc->args.next.x = next+1;
      pc->args.next.y = pc+1;
      next->opcode = Jump;
      next->args.next.x = pc;
      pc = next + 1;
      goto done;
    case Plus:
      next = compiletree(pc, flags, t->args.next.x);
      next->opcode = Split;
      next->args.next.x = pc;
      next->args.next.y = next+1;
      pc = next + 1;
      goto done;
    case WeakPlus:
      next = compiletree(pc, flags, t->args.next.x);
      next->opcode = Split;
      next->args.next.x = next+1;
      next->args.next.y = pc;
      pc = next + 1;
      goto done;
    case Capture:
      if(flags->nextsave > MAX_SAVE) {
	t = t->args.next.x;
	break;
      }
      savepoint = 2*flags->nextsave++;
      pc->opcode = Save;
      pc->args.i = savepoint;
      pc = compiletree(pc+1, flags, t->args.next.x);
      pc->opcode = Save;
      pc->args.i = savepoint + 1;
      pc++;
      goto done;
    default:
      abort();
    }
  }
 done:
  return pc;
}

int
compile(struct Program *prog, char *regex)
{
  struct AST *t;
  struct Inst *pc;
  struct Flags flags = {0};
  size_t max;
  int rc;

  if(!prog || !regex)
    return (errno=EINVAL, -1);
  rc = parse(&t, prog, regex);
  if(rc) return rc;
  max = 2*strlen(regex) + MIN_CODESIZE;
  prog->code = calloc(max, sizeof *prog->code);
  if(prog->code == NULL) {
    free(t);
    return (errno=ENOMEM, -1);
  }
  pc = compiletree(prog->code, &flags, t);
  pc->opcode = flags.matchend ? MatchEnd : Match;
  pc++;
  prog->size = pc - prog->code;
  assert(prog->size <= max);
  pc = realloc(prog->code, prog->size * sizeof *prog->code);
  if(pc) prog->code = pc;
  free(t);
  return 0;
}

void
freeprogram(struct Program *prog)
{
  free(prog->code);
  prog->code = NULL;
}
