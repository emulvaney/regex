/* A Regular Expression Library - Core Routines
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

#include <limits.h>

/* Opcodes (Inst.opcode) */
enum Opcode {
  CharAlt,   /* die unless next char is chr.c or chr.alt */
  Char,      /* die unless next char is chr.c */
  AnyChar,   /* accept the current character */
  CharSet,   /* die unless charset[next_char] & mask */
  Match,     /* regex match successful */
  MatchEnd,  /* regex match if at end of string */
  Jump,      /* jump to x */
  Split,     /* fork, jumping to x and y */
  Save       /* save position in saved[i] */
};

/* Full Instructions */
struct Inst {
  enum Opcode opcode;  /* as described above */
  union {
    int i;
    struct {
      int c, alt;
    } chr;
    struct {
      unsigned mask, *charset;
    } set;
    struct {
      struct Inst *x, *y;
    } next;
  } args;
};

/* Regex Operations (AST.op) */
enum Op {
  Onechar,  /* c   - match the character c */
  Anychar,  /* .   - match any character */
  Charset,  /* []  - match if charset[next_char] & mask */
  Dollar,   /* $   - match the end of a string */
  Epsilon,  /*     - match nothing (the empty string) */
  Concat,   /* xy  - match x then y */
  Either,   /* x|y - match either x or y */
  Optional, /* x?  - match x or epsilon (prefer to match) */
  WeakOpt,  /* x?? - match epsilon or x (prefer not to match) */
  Star,     /* x*  - match zero or more x (prefer longest match) */
  WeakStar, /* x*? - match more or zero x (prefer shortest match) */
  Plus,     /* x+  - match one or more x (prefer longest match) */
  WeakPlus, /* x+? - match more or one x (prefer shortest match) */
  Capture   /* (x) - match x, and make note of its start/end */
};

/* Abstract Syntax Tree */
struct AST
{
  enum Op op;
  union {
    int c;
    struct {
      unsigned mask, *charset;
    } set;
    struct {
      struct AST *x, *y;
    } next;
  } args;
};

struct Program {
  struct Inst *code;
  int options, size;
  unsigned charset[UCHAR_MAX];
};

enum Options {  /* bits */
  IgnoreCase = 1
};

/* parse(*ast, prog, regex)
 *
 * Create the abstract syntax tree (AST) for a new program and the
 * given regex.  When the regex is successfully parsed, *ast will be
 * assigned the AST.
 */
int parse(struct AST **ast, struct Program *prog, char *regex);

/* compile(prog, regex)
 *
 * Compile a regular expression (regex) into a program (prog).
 */
int compile(struct Program *prog, char *regex, int options);
void freeprogram(struct Program *prog);

/* vm(prog, input, saved)
 *
 * Execute compiled regex (prog) on input string (input).  If
 * successful, 1 is returned and the array (saved[20]) will contain
 * every position recorded by a Save instruction (unused entries will
 * be set NULL).  If no match is found, 0 is returned.  On error, -1
 * is returned and errno is set appropriately.
 */
int vm(struct Program *prog, char *input, char **saved);
