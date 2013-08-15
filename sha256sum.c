/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "util.h"
#include "crypt.h"
#include "sha256.h"

struct sha256 s;
struct crypt_ops sha256_ops = {
	sha256_init,
	sha256_update,
	sha256_sum,
	&s,
};

static void
usage(void)
{
	eprintf("usage: %s [-c] [file...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	uint8_t md[SHA256_DIGEST_LENGTH];

	ARGBEGIN {
	case 'c':
		eprintf("not implemented\n");
	default:
		usage();
	} ARGEND;

	if (argc == 0) {
		cryptsum(&sha256_ops, stdin, "<stdin>", md);
		mdprint(md, "<stdin>", sizeof(md));
	} else {
		for (; argc > 0; argc--) {
			if ((fp = fopen(*argv, "r")) == NULL)
				eprintf("fopen %s:", *argv);
			cryptsum(&sha256_ops, fp, *argv, md);
			mdprint(md, *argv, sizeof(md));
			fclose(fp);
			argv++;
		}
	}

	return 0;
}
