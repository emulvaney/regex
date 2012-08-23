/* A Regular Expression Library - Parser
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

/* The smallest AST (for an empty regex) contains five parts:
 *       (Concat (WeakStar (AnyChar))
 *               (Capture (Epsilon)))
 * Each character in a regex can add at most one part to this tree
 * (see parselevel()), while adjacent parts are concatenated (adding
 * another).  Therefore, an upper-bound on the tree size is 2*N+5 for
 * a regex of size N (see parse()).
 */
enum { MIN_TREESIZE=5 };

struct Tree {
  struct AST *root, *stack, *heap;
};

static int
inittree(struct Tree* t, size_t size)
{
  t->root = calloc(size, sizeof *t->root);
  if(t->root == NULL) return (errno=ENOMEM, -1);
  t->stack = t->root;
  t->heap  = t->root + size;
  return 0;
}

static void
freetree(struct Tree *t)
{
  free(t->root);
  memset(t, 0, sizeof *t);
}

#define top(t)   ((t)->stack - 1)
#define push(t)  ((t)->stack++)
#define pop(t)   (*--(t)->heap = *--(t)->stack, (t)->heap)
#define drop(t)  ((void)(t)->stack--)

/* Concatenate all the elements on the top of the stack down to bot,
 * leaving one element at bot.  If there are no elements to
 * concatenate (so bot > top(t)), push Epsilon instead.
 */
static void
concat(struct AST *bot, struct Tree *t)
{
  struct AST *x, *y, *z;
  if(top(t) < bot)
    push(t)->op = Epsilon;
  while(top(t) > bot) {
    y = pop(t);
    x = pop(t);
    z = push(t);
    z->op = Concat;
    z->args.next.x = x;
    z->args.next.y = y;
  }
  assert(top(t) == bot);
}

static int
parseclass(struct Tree *t, struct Program *prog, char **ref)
{
  struct AST *x, *bot = t->stack;
  unsigned char *sp = (unsigned char*)*ref;
  unsigned mask = prog->charset[0];
  int c, negate = 0;

  assert(mask != 0);  /* only 32 character classes allowed (TODO) */
  assert(((mask - 1) & mask) == 0);  /* only one bit should be set */
  prog->charset[0] <<= 1;
  if(*sp == '^') {
    sp++;
    negate = 1;
  }
  if(*sp == ']') {
    c = *sp++;
    goto notspecial;
  }
  for(;;) {
    c = *sp++;
    switch(c) {
    case '\0':
      sp--;
      /* no break */
    case ']':
      goto finished;
    default:
    notspecial:
      if(sp[0] != '-' || sp[1] == ']' || sp[1] == '\0') {
	prog->charset[c] |= mask;
      } else { /* it's a range like "A-Z" */
	for(; c <= sp[1]; c++)
	  prog->charset[c] |= mask;
	sp += 2;
      }
    }
  }
 finished:
  if(negate) {
    for(c=1; c < sizeof prog->charset; c++)
      prog->charset[c] ^= mask;
  }
  x = push(t);
  x->op = Charset;
  x->args.set.mask    = mask;
  x->args.set.charset = prog->charset;
  *ref = (char*)sp;
  assert(top(t) == bot);
  return 0;
}

static int
parselevel(struct Tree *t, struct Program *prog, char **ref, int level)
{
  struct AST *x, *y, *z, *bot = t->stack;
  char c, *sp = *ref;
  int rc;

  for(;;) {
    c = *sp++;
    switch(c) {
    case '\\':
      if(*sp)
	c = *sp++;
      /* no break */
    default: onechar:
      x = push(t);
      x->op = Onechar;
      x->args.c = c;
      break;
    case '.':
      push(t)->op = Anychar;
      break;
    case '[':
      rc = parseclass(t, prog, &sp);
      if(rc) return rc;
      break;
    case '?':
      if(t->stack == bot) goto onechar;
      x = pop(t);
      y = push(t);
      y->op = (*sp == '?' ? (sp++, WeakOpt) : Optional);
      y->args.next.x = x;
      break;
    case '*':
      if(t->stack == bot) goto onechar;
      x = pop(t);
      y = push(t);
      y->op = (*sp == '?' ? (sp++, WeakStar) : Star);
      y->args.next.x = x;
      break;
    case '+':
      if(t->stack == bot) goto onechar;
      x = pop(t);
      y = push(t);
      y->op = (*sp == '?' ? (sp++, WeakPlus) : Plus);
      y->args.next.x = x;
      break;
    case '|':
      concat(bot, t);
      x = top(t);
      rc = parselevel(t, prog, &sp, level);
      if(rc) return rc;
      y = top(t);
      if(x->op == Epsilon && y->op == Epsilon) {
	drop(t);  /* one will do */
      } else if(x->op == Epsilon || y->op == Epsilon) {
	if(x->op == Epsilon) { x = pop(t); drop(t); }  /* |y */
	else                 { drop(t); x = pop(t); }  /* x| */
	y = push(t);
	y->op = Optional;
	y->args.next.x = x;
      } else {
	y = pop(t);
	x = pop(t);
	z = push(t);
	z->op = Either;
	z->args.next.x = x;
	z->args.next.y = y;
      }
      break;
    case '(':
      rc = parselevel(t, prog, &sp, level+1);
      if(rc) return rc;
      x = pop(t);
      y = push(t);
      y->op = Capture;
      y->args.next.x = x;
      break;
    case ')':
      if(level == 0) goto onechar;
      goto finish;
    case '$':
      if(*sp) goto onechar;
      /* no break */
    case '\0':
      sp--;
      goto finish;
    }
  }
 finish:
  *ref = sp;
  concat(bot, t);
  assert(top(t) == bot);
  return 0;
}

static int
parseregex(struct Tree *t, struct Program *prog, char *sp)
{
  struct AST *x, *y, *bot = t->stack;
  int rc;

  if(*sp == '^')
    sp++;
  else {
    push(t)->op = Anychar;
    x = pop(t);
    y = push(t);
    y->op = WeakStar;
    y->args.next.x = x;
  }
  rc = parselevel(t, prog, &sp, 0);
  if(rc) return rc;
  x = pop(t);
  y = push(t);
  y->op = Capture;
  y->args.next.x = x;
  if(*sp == '$') {
    sp++;
    push(t)->op = Dollar;
  }
  concat(bot, t);
  assert(top(t) == bot);
  return 0;
}

int
parse(struct AST **ast, struct Program *prog, char *regex)
{
  struct Tree t;
  size_t max = 2*strlen(regex) + MIN_TREESIZE;
  int rc;

  if(!ast || !prog || !regex)
    return (errno=EINVAL, -1);
  rc = inittree(&t, max);
  if(rc) return rc;
  memset(prog->charset, 0, sizeof prog->charset);
  prog->charset[0] = 1;  /* the next bit to use */
  rc = parseregex(&t, prog, regex);
  prog->charset[0] = 0;  /* we never match NULs */
  assert(t.stack <= t.heap);  /* overflow check */
  if(rc) {
    freetree(&t);
    return rc;
  }
  *ast = t.root;
  return 0;
}
