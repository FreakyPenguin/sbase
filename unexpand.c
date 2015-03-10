/* See LICENSE file for copyright and license details. */
#include <stdint.h>
#include <stdlib.h>

#include "utf.h"
#include "util.h"

static int     aflag      = 0;
static size_t *tablist    = NULL;
static int     tablistlen = 8;

static size_t
parselist(const char *s)
{
	size_t i;
	char  *p, *tmp;

	tmp = estrdup(s);
	for (i = 0; (p = strsep(&tmp, " ,")); i++) {
		if (*p == '\0')
			eprintf("empty field in tablist\n");
		tablist = ereallocarray(tablist, i + 1, sizeof(*tablist));
		tablist[i] = estrtonum(p, 1, MIN(LLONG_MAX, SIZE_MAX));
		if (i > 0 && tablist[i - 1] >= tablist[i])
			eprintf("tablist must be ascending\n");
	}
	tablist = ereallocarray(tablist, i + 1, sizeof(*tablist));
	return i;
}

static void
unexpandspan(size_t last, size_t col)
{
	size_t off, i, j;
	Rune r;

	if (tablistlen == 1) {
		i = 0;
		off = last % tablist[i];

		if ((col - last) + off >= tablist[i] && last < col)
			last -= off;

		r = '\t';
		for (; last + tablist[i] <= col; last += tablist[i])
			efputrune(&r, stdout, "<stdout>");
		r = ' ';
		for (; last < col; last++)
			efputrune(&r, stdout, "<stdout>");
	} else {
		for (i = 0; i < tablistlen; i++)
			if (col < tablist[i])
				break;
		for (j = 0; j < tablistlen; j++)
			if (last < tablist[j])
				break;
		r = '\t';
		for (; j < i; j++) {
			efputrune(&r, stdout, "<stdout>");
			last = tablist[j];
		}
		r = ' ';
		for (; last < col; last++)
			efputrune(&r, stdout, "<stdout>");
	}
}

static void
unexpand(const char *file, FILE *fp)
{
	Rune r;
	size_t last = 0, col = 0, i;
	int bol = 1;

	while (efgetrune(&r, fp, file)) {
		switch (r) {
		case ' ':
			if (!bol && !aflag)
				last++;
			col++;
			break;
		case '\t':
			if (tablistlen == 1) {
				if (!bol && !aflag)
					last += tablist[0] - col % tablist[0];
				col += tablist[0] - col % tablist[0];
			} else {
				for (i = 0; i < tablistlen; i++)
					if (col < tablist[i])
						break;
				if (!bol && !aflag)
					last = tablist[i];
				col = tablist[i];
			}
			break;
		case '\b':
			if (bol || aflag)
				unexpandspan(last, col);
			col -= (col > 0);
			last = col;
			bol = 0;
			break;
		case '\n':
			if (bol || aflag)
				unexpandspan(last, col);
			last = col = 0;
			bol = 1;
			break;
		default:
			if (bol || aflag)
				unexpandspan(last, col);
			last = ++col;
			bol = 0;
			break;
		}
		if ((r != ' ' && r != '\t') || (!aflag && !bol))
			efputrune(&r, stdout, "<stdout>");
	}
	if (last < col && (bol || aflag))
		unexpandspan(last, col);
}

static void
usage(void)
{
	eprintf("usage: %s [-a] [-t tablist] [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	int ret = 0;
	char *tl = "8";

	ARGBEGIN {
	case 't':
		tl = EARGF(usage());
		if (!*tl)
			eprintf("tablist cannot be empty\n");
		/* Fallthrough: -t implies -a */
	case 'a':
		aflag = 1;
		break;
	default:
		usage();
	} ARGEND;

	tablistlen = parselist(tl);

	if (argc == 0) {
		unexpand("<stdin>", stdin);
	} else {
		for (; argc > 0; argc--, argv++) {
			if (!(fp = fopen(argv[0], "r"))) {
				weprintf("fopen %s:", argv[0]);
				ret = 1;
				continue;
			}
			unexpand(argv[0], fp);
			fclose(fp);
		}
	}
	return ret;
}
