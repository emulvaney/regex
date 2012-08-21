/* A Virtual Machine for Pattern Matching
 * Copyright (c) 2012 Eric Mulvaney
 * TODO: Add GPL3 bit here
 */

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"

struct Thread {
  struct Inst *pc;  /* this thread's program counter */
  char *saved[20];  /* for Save instructions */
  int listid;       /* used by ThreadList */
};

static struct Thread
thread(struct Inst *pc, char **saved)
{
  struct Thread t;
  t.pc = pc;
  t.listid = 0;  /* not in any list */
  memcpy(t.saved, saved, sizeof t.saved);
  return t;
}

/* Threads are stored and processed sequentially from t[0] to t[n-1].
 * To ensure that no duplicates are added to a list, each instruction
 * pc, when added, is marked by id at t[pc-pc0].listid--we also mark
 * instructions that lead to those that are added (see addthread()) to
 * avoid unnecessary re-computation.
 */
struct ThreadList {
  struct Thread *t;  /* storage for the thread list */
  struct Inst *pc0;  /* the first instruction of the program */
  int n;    /* the number of threads in the list */
  int max;  /* the capacity of the list */
  int id;   /* to mark instructions while adding threads to list */
};

static int
initlist(struct ThreadList *list, struct Program *prog)
{
  assert(prog->size > 0);
  list->n   = 0;
  list->max = prog->size;  /* we need room for all instructions */
  list->pc0 = prog->code;
  list->id  = 1;
  list->t = calloc(list->max, sizeof *list->t);
  return list->t ? 0 : (errno=ENOMEM, -1);
}

static void
freelist(struct ThreadList* list)
{
  free(list->t);
  list->t = NULL;
}

static void
swap(struct ThreadList* alist, struct ThreadList *blist)
{
  struct ThreadList tmp;
  tmp = *alist; *alist = *blist; *blist = tmp;
}

static void
clear(struct ThreadList *list) {
  list->n = 0;
  if(!++list->id) {  /* not likely but possible */
    memset(list->t, 0, list->max * sizeof *list->t);
    list->id = 1;
  }
}

/* Add a thread to a thread list, unless it's already in the list.
 * The thread will be executed until a new input character is
 * required; sp marks the current position in the input.
 */
static void
addthread(struct ThreadList *list, char *sp, struct Thread t)
{
  struct Thread *p;
  ptrdiff_t i;
  for(;;) {
    i = t.pc - list->pc0;
    assert(i >= 0 && i < list->max);
    if(list->t[i].listid == list->id)
      break;  /* instruction already explored */
    list->t[i].listid = list->id;
    switch(t.pc->opcode) {
    case Jump:
      t.pc = t.pc->args.next.x;
      break;
    case Split:
      addthread(list, sp, thread(t.pc->args.next.x, t.saved));
      t.pc = t.pc->args.next.y;
      break;
    case Save:
      t.saved[t.pc->args.i] = sp;
      t.pc++;
      break;
    default: /* an instruction handled by vm() */
      p = &list->t[list->n++];
      t.listid = p->listid;
      *p = t;
      return;
    }
  }
}

int
vm(struct Program *prog, char *input, char **saved)
{
  struct ThreadList clist={0}, nlist={0};
  struct Thread *t;
  struct Inst *pc;
  char *sp = input;
  int i, j, rc=0;

  if(!prog || !input || !saved || !prog->code || prog->size < 1)
    return (errno=EINVAL, -1);

  rc = initlist(&clist, prog);  if(rc) goto done;
  rc = initlist(&nlist, prog);  if(rc) goto done;

  addthread(&clist, sp, thread(prog->code, saved));
  do {
    for(i = 0; i < clist.n; i++) {
      t = &clist.t[i];
      pc = t->pc;
      switch(pc->opcode) {
      case CharSet:
	if(!(pc->args.set.charset[(unsigned char)*sp] & pc->args.set.mask)) {
	  break;
	case Char:
	  if(*sp != pc->args.c)
	    break;
	}
	/* no break */
      case AnyChar:
	addthread(&nlist, sp+1, thread(t->pc+1, t->saved));
	break;
      case MatchEnd:
	if(*sp) break;
	/* no break */
      case Match:
	memcpy(saved, t->saved, sizeof t->saved);
	rc = 1;  /* first or longer match found */
	assert(t->saved[0] != NULL);
	j = clist.n - 1;
	while(clist.t[j].saved[0] == NULL ||
	      clist.t[j].saved[0] > t->saved[0])
	  j--;
	clist.n = j + 1;  /* drop threads matching later */
	break;
      default: /* should have been handled by addthread() */
	abort();
      }
    }
    swap(&clist, &nlist);
    clear(&nlist);
  } while(*sp++ && clist.n > 0);
 done:
  i = errno;
  freelist(&clist);
  freelist(&nlist);
  errno = i;
  return rc;
}
