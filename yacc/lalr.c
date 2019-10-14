/*	$OpenBSD: lalr.c,v 1.19 2017/05/25 20:11:03 tedu Exp $	*/
/*	$NetBSD: lalr.c,v 1.4 1996/03/19 03:21:33 jtc Exp $	*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Paul Corbett.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "defs.h"

typedef struct shorts {
	struct shorts *next;
	short value;
} shorts;

int tokensetsize;
short *lookaheads;
short *LAruleno;
unsigned *LA;
short *accessing_symbol;
core **state_table;
shifts **shift_table;
reductions **reduction_table;
short *goto_map;
short *from_state;
short *to_state;

short **transpose(short **, int);
void set_state_table(void);
void set_accessing_symbol(void);
void set_shift_table(void);
void set_reduction_table(void);
void set_maxrhs(void);
void initialize_LA(void);
void set_goto_map(void);
void initialize_F(void);
void build_relations(void);
void compute_FOLLOWS(void);
void compute_lookaheads(void);
int map_goto(int, int);
void digraph(short **);
void add_lookback_edge(int, int, int);
void traverse(int);

static int infinity;
static int maxrhs;
static int ngotos;
static unsigned *F;
static short **includes;
static shorts **lookback;
static short **R;
static short *INDEX;
static short *VERTICES;
static int top;

void
lalr(void)
{
	tokensetsize = WORDSIZE(ntokens);

	set_state_table();
	set_accessing_symbol();
	set_shift_table();
	set_reduction_table();
	set_maxrhs();
	initialize_LA();
	set_goto_map();
	initialize_F();
	build_relations();
	compute_FOLLOWS();
	compute_lookaheads();
	free_derives();
	free_nullable();
}


void
set_state_table(void)
{
	core *sp;

	state_table = NEW2(nstates, core *);
	for (sp = first_state; sp; sp = sp->next)
		state_table[sp->number] = sp;
}


void
set_accessing_symbol(void)
{
	core *sp;

	accessing_symbol = NEW2(nstates, short);
	for (sp = first_state; sp; sp = sp->next)
		accessing_symbol[sp->number] = sp->accessing_symbol;
}


void
set_shift_table(void)
{
	shifts *sp;

	shift_table = NEW2(nstates, shifts *);
	for (sp = first_shift; sp; sp = sp->next)
		shift_table[sp->number] = sp;
}


void
set_reduction_table(void)
{
	reductions *rp;

	reduction_table = NEW2(nstates, reductions *);
	for (rp = first_reduction; rp; rp = rp->next)
		reduction_table[rp->number] = rp;
}


void
set_maxrhs(void)
{
	short *itemp, *item_end;
	int length, max;

	length = 0;
	max = 0;
	item_end = ritem + nitems;
	for (itemp = ritem; itemp < item_end; itemp++) {
		if (*itemp >= 0) {
			length++;
		} else {
			if (length > max) max = length;
			length = 0;
		}
	}

	maxrhs = max;
}


void
initialize_LA(void)
{
	int i, j, k;
	reductions *rp;

	lookaheads = NEW2(nstates + 1, short);

	k = 0;
	for (i = 0; i < nstates; i++) {
		lookaheads[i] = k;
		rp = reduction_table[i];
		if (rp)
			k += rp->nreds;
	}
	lookaheads[nstates] = k;

	LA = NEW2(k * tokensetsize, unsigned);
	LAruleno = NEW2(k, short);
	lookback = NEW2(k, shorts *);

	k = 0;
	for (i = 0; i < nstates; i++) {
		rp = reduction_table[i];
		if (rp) {
			for (j = 0; j < rp->nreds; j++) {
				LAruleno[k] = rp->rules[j];
				k++;
			}
		}
	}
}

void
set_goto_map(void)
{
	shifts *sp;
	int i, k, symbol;
	int state1, state2;
	short *temp_map;

	goto_map = NEW2(nvars + 1, short) - ntokens;
	temp_map = NEW2(nvars + 1, short) - ntokens;

	ngotos = 0;
	for (sp = first_shift; sp; sp = sp->next) {
		for (i = sp->nshifts - 1; i >= 0; i--) {
			symbol = accessing_symbol[sp->shift[i]];

			if (ISTOKEN(symbol)) break;

			if (ngotos == MAXSHORT)
				fatal("too many gotos");

			ngotos++;
			goto_map[symbol]++;
		}
	}

	k = 0;
	for (i = ntokens; i < nsyms; i++) {
		temp_map[i] = k;
		k += goto_map[i];
	}

	for (i = ntokens; i < nsyms; i++)
		goto_map[i] = temp_map[i];

	goto_map[nsyms] = ngotos;
	temp_map[nsyms] = ngotos;

	from_state = NEW2(ngotos, short);
	to_state = NEW2(ngotos, short);

	for (sp = first_shift; sp; sp = sp->next) {
		state1 = sp->number;
		for (i = sp->nshifts - 1; i >= 0; i--) {
			state2 = sp->shift[i];
			symbol = accessing_symbol[state2];

			if (ISTOKEN(symbol)) break;

			k = temp_map[symbol]++;
			from_state[k] = state1;
			to_state[k] = state2;
		}
	}

	free(temp_map + ntokens);
}



/*  Map_goto maps a state/symbol pair into its numeric representation.	*/

int
map_goto(int state, int symbol)
{
	int high, low, middle, s;

	low = goto_map[symbol];
	high = goto_map[symbol + 1];

	for (;;) {
		assert(low <= high);
		middle = (low + high) >> 1;
		s = from_state[middle];
		if (s == state)
			return (middle);
		else if (s < state)
			low = middle + 1;
		else
			high = middle - 1;
	}
}


void
initialize_F(void)
{
	int i, j, k;
	shifts *sp;
	short *edge, *rp, **reads;
	unsigned int *rowp;
	int nedges, stateno, symbol, nwords;

	nwords = ngotos * tokensetsize;
	F = NEW2(nwords, unsigned);

	reads = NEW2(ngotos, short *);
	edge = NEW2(ngotos + 1, short);
	nedges = 0;

	rowp = F;
	for (i = 0; i < ngotos; i++) {
		stateno = to_state[i];
		sp = shift_table[stateno];

		if (sp) {
			k = sp->nshifts;

			for (j = 0; j < k; j++) {
				symbol = accessing_symbol[sp->shift[j]];
				if (ISVAR(symbol))
					break;
				SETBIT(rowp, symbol);
			}

			for (; j < k; j++) {
				symbol = accessing_symbol[sp->shift[j]];
				if (nullable[symbol])
					edge[nedges++] = map_goto(stateno, symbol);
			}
			
			if (nedges) {
				reads[i] = rp = NEW2(nedges + 1, short);

				for (j = 0; j < nedges; j++)
					rp[j] = edge[j];

				rp[nedges] = -1;
				nedges = 0;
			}
		}

		rowp += tokensetsize;
	}

	SETBIT(F, 0);
	digraph(reads);

	for (i = 0; i < ngotos; i++)
		free(reads[i]);

	free(reads);
	free(edge);
}


void
build_relations(void)
{
	int i, j, k;
	short *rulep, *rp;
	shifts *sp;
	int length, nedges, done;
	int state1, stateno, symbol1, symbol2;
	short *shortp, *edge, *states;
	short **new_includes;

	includes = NEW2(ngotos, short *);
	edge = NEW2(ngotos + 1, short);
	states = NEW2(maxrhs + 1, short);

	for (i = 0; i < ngotos; i++) {
		nedges = 0;
		state1 = from_state[i];
		symbol1 = accessing_symbol[to_state[i]];

		for (rulep = derives[symbol1]; *rulep >= 0; rulep++) {
			length = 1;
			states[0] = state1;
			stateno = state1;

			for (rp = ritem + rrhs[*rulep]; *rp >= 0; rp++) {
				symbol2 = *rp;
				sp = shift_table[stateno];
				k = sp->nshifts;

				for (j = 0; j < k; j++) {
					stateno = sp->shift[j];
					if (accessing_symbol[stateno] == symbol2)
						break;
				}

				states[length++] = stateno;
			}

			add_lookback_edge(stateno, *rulep, i);

			length--;
			done = 0;
			while (!done) {
				done = 1;
				rp--;
				if (ISVAR(*rp)) {
					stateno = states[--length];
					edge[nedges++] = map_goto(stateno, *rp);
					if (nullable[*rp] && length > 0)
						done = 0;
				}
			}
		}

		if (nedges) {
			includes[i] = shortp = NEW2(nedges + 1, short);
			for (j = 0; j < nedges; j++)
				shortp[j] = edge[j];
			shortp[nedges] = -1;
		}
	}

	new_includes = transpose(includes, ngotos);

	for (i = 0; i < ngotos; i++)
		free(includes[i]);

	free(includes);

	includes = new_includes;

	free(edge);
	free(states);
}

void
add_lookback_edge(int stateno, int ruleno, int gotono)
{
	int i, k, found;
	shorts *sp;

	i = lookaheads[stateno];
	k = lookaheads[stateno + 1];
	found = 0;
	while (!found && i < k) {
		if (LAruleno[i] == ruleno)
			found = 1;
		else
			++i;
	}
	assert(found);

	sp = NEW(shorts);
	sp->next = lookback[i];
	sp->value = gotono;
	lookback[i] = sp;
}



short **
transpose(short **old_R, int n)
{
	short **new_R, **temp_R, *nedges, *sp;
	int i, k;

	nedges = NEW2(n, short);

	for (i = 0; i < n; i++) {
		sp = old_R[i];
		if (sp) {
			while (*sp >= 0)
				nedges[*sp++]++;
		}
	}

	new_R = NEW2(n, short *);
	temp_R = NEW2(n, short *);

	for (i = 0; i < n; i++) {
		k = nedges[i];
		if (k > 0) {
			sp = NEW2(k + 1, short);
			new_R[i] = sp;
			temp_R[i] = sp;
			sp[k] = -1;
		}
	}

	free(nedges);

	for (i = 0; i < n; i++) {
		sp = old_R[i];
		if (sp) {
			while (*sp >= 0)
				*temp_R[*sp++]++ = i;
		}
	}

	free(temp_R);

	return (new_R);
}


void
compute_FOLLOWS(void)
{
	digraph(includes);
}

void
compute_lookaheads(void)
{
	int i, n;
	unsigned int *fp1, *fp2, *fp3;
	shorts *sp, *next;
	unsigned int *rowp;

	rowp = LA;
	n = lookaheads[nstates];
	for (i = 0; i < n; i++) {
		fp3 = rowp + tokensetsize;
		for (sp = lookback[i]; sp; sp = sp->next) {
			fp1 = rowp;
			fp2 = F + tokensetsize * sp->value;
			while (fp1 < fp3)
				*fp1++ |= *fp2++;
		}
		rowp = fp3;
	}

	for (i = 0; i < n; i++)
		for (sp = lookback[i]; sp; sp = next) {
			next = sp->next;
			free(sp);
		}

	free(lookback);
	free(F);
}

void
digraph(short **relation)
{
	int i;

	infinity = ngotos + 2;
	INDEX = NEW2(ngotos + 1, short);
	VERTICES = NEW2(ngotos + 1, short);
	top = 0;
	R = relation;

	memset(INDEX, 0, ngotos * sizeof(short));
	for (i = 0; i < ngotos; i++)
		if (R[i])
			traverse(i);

	free(INDEX);
	free(VERTICES);
}


void
traverse(int i)
{
	unsigned int *base, *fp1, *fp2, *fp3;
	int j, height;
	short *rp;

	VERTICES[++top] = i;
	INDEX[i] = height = top;

	base = F + i * tokensetsize;
	fp3 = base + tokensetsize;

	rp = R[i];
	if (rp) {
		while ((j = *rp++) >= 0) {
			if (INDEX[j] == 0)
				traverse(j);

			if (INDEX[i] > INDEX[j])
				INDEX[i] = INDEX[j];

			fp1 = base;
			fp2 = F + j * tokensetsize;

			while (fp1 < fp3)
				*fp1++ |= *fp2++;
		}
	}

	if (INDEX[i] == height) {
		for (;;) {
			j = VERTICES[top--];
			INDEX[j] = infinity;

			if (i == j)
				break;

			fp1 = base;
			fp2 = F + j * tokensetsize;

			while (fp1 < fp3)
				*fp2++ = *fp1++;
		}
	}
}
