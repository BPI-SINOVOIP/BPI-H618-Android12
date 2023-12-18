/*
 * (C) Copyright David Gibson <dwg@au1.ibm.com>, IBM Corporation.  2005.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *                                                                   USA
 */

#include "dtc.h"
#include "srcpos.h"
#include "updatetree.h"
#include "version_gen.h"

/*
 * Command line options
 */
int quiet;		/* Level of quietness */
int reservenum;		/* Number of memory reservation slots */
int minsize;		/* Minimum blob size */
int padsize;		/* Additional padding to blob */
int phandle_format = PHANDLE_BOTH;	/* Use linux,phandle or phandle properties */


static void fill_fullpaths(struct node *tree, const char *prefix)
{
	struct node *child;
	const char *unit;

	tree->fullpath = join_path(prefix, tree->name);

	unit = strchr(tree->name, '@');
	if (unit)
		tree->basenamelen = unit - tree->name;
	else
		tree->basenamelen = strlen(tree->name);

	for_each_child(tree, child)
		fill_fullpaths(child, tree->fullpath);
}

static void  __attribute__ ((noreturn)) usage(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\tdtc [options] <input file>\n");
	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "\t-h\n");
	fprintf(stderr, "\t\tThis help text\n");
	fprintf(stderr, "\t-q\n");
	fprintf(stderr, "\t\tQuiet: -q suppress warnings, -qq errors, -qqq all\n");
	fprintf(stderr, "\t-I <input format>\n");
	fprintf(stderr, "\t\tInput formats are:\n");
	fprintf(stderr, "\t\t\tdts - device tree source text\n");
	fprintf(stderr, "\t\t\tdtb - device tree blob\n");
	fprintf(stderr, "\t\t\tfs - /proc/device-tree style directory\n");
	fprintf(stderr, "\t-o <output file>\n");
	fprintf(stderr, "\t-O <output format>\n");
	fprintf(stderr, "\t\tOutput formats are:\n");
	fprintf(stderr, "\t\t\tdts - device tree source text\n");
	fprintf(stderr, "\t\t\tdtb - device tree blob\n");
	fprintf(stderr, "\t\t\tasm - assembler source\n");
	fprintf(stderr, "\t-V <output version>\n");
	fprintf(stderr, "\t\tBlob version to produce, defaults to %d (relevant for dtb\n\t\tand asm output only)\n", DEFAULT_FDT_VERSION);
	fprintf(stderr, "\t-d <output dependency file>\n");
	fprintf(stderr, "\t-R <number>\n");
	fprintf(stderr, "\t\tMake space for <number> reserve map entries (relevant for \n\t\tdtb and asm output only)\n");
	fprintf(stderr, "\t-S <bytes>\n");
	fprintf(stderr, "\t\tMake the blob at least <bytes> long (extra space)\n");
	fprintf(stderr, "\t-p <bytes>\n");
	fprintf(stderr, "\t\tAdd padding to the blob of <bytes> long (extra space)\n");
	fprintf(stderr, "\t-b <number>\n");
	fprintf(stderr, "\t\tSet the physical boot cpu\n");
	fprintf(stderr, "\t-f\n");
	fprintf(stderr, "\t\tForce - try to produce output even if the input tree has errors\n");
	fprintf(stderr, "\t-i\n");
	fprintf(stderr, "\t\tAdd a path to search for include files\n");
	fprintf(stderr, "\t-s\n");
	fprintf(stderr, "\t\tSort nodes and properties before outputting (only useful for\n\t\tcomparing trees)\n");
	fprintf(stderr, "\t-v\n");
	fprintf(stderr, "\t\tPrint DTC version and exit\n");
	fprintf(stderr, "\t-H <phandle format>\n");
	fprintf(stderr, "\t\tphandle formats are:\n");
	fprintf(stderr, "\t\t\tlegacy - \"linux,phandle\" properties only\n");
	fprintf(stderr, "\t\t\tepapr - \"phandle\" properties only\n");
	fprintf(stderr, "\t\t\tboth - Both \"linux,phandle\" and \"phandle\" properties\n");
	fprintf(stderr, "\t-W [no-]<checkname>\n");
	fprintf(stderr, "\t-E [no-]<checkname>\n");
	fprintf(stderr, "\t\t\tenable or disable warnings and errors\n");
	exit(3);
}

struct sunxi_dtc_param {
	char *inname;
	char *outname;
	char *fexname;
};


int sunxi_libdtc(struct sunxi_dtc_param *dtc_param)
{
	struct boot_info *bi;
	const char *inform = "dtb";
	const char *outform = "dtb";
	const char *outname = NULL;
	const char *fexname = NULL;
	int force = 0, sort = 0;
	const char *arg;
	int opt;
	FILE *outf = NULL;
	int outversion = DEFAULT_FDT_VERSION;
	long long cmdline_boot_cpuid = 0;

	quiet      = 0;
	reservenum = 0;
	padsize    = 0;

	outname = dtc_param->outname;
	fexname = dtc_param->fexname;
	arg = dtc_param->inname;

	printf("inform=%s,outform=%s,outname=%s,fexname=%s,arg=%s\n",inform, outform, outname, fexname,arg);

	if (streq(inform, "dts"))
		bi = dt_from_source(arg);
	else if (streq(inform, "fs"))
		bi = dt_from_fs(arg);
	else if(streq(inform, "dtb"))
		bi = dt_from_blob(arg);
	else {
		die("Unknown input format \"%s\"\n", inform);
		goto err;
	}

	if (cmdline_boot_cpuid != -1)
		bi->boot_cpuid_phys = cmdline_boot_cpuid;

	fill_fullpaths(bi->dt, "");
	process_checks(force, bi);

	if (sort)
		sort_tree(bi);

	if (streq(outname, "-")) {
		outf = stdout;
	} else {
		outf = fopen(outname, "w");
		if (! outf) {
			die("Couldn't open output file %s: %s\n",
			    outname, strerror(errno));
			goto err;
		}
	}

	/* add by huangshr.
	 * here we parser script file and
	 * insert mainkey info into bi struct.
	 */
	if (fexname) {
		dt_update_source(fexname, outf, bi);

		/* recheck bi struct */
		fill_fullpaths(bi->dt, "");
		dirty_checks();
		process_checks(force, bi);
	}

	if (streq(outform, "dts")) {
		dt_to_source(outf, bi);
	} else if (streq(outform, "dtb")) {
		dt_to_blob(outf, bi, outversion);
	} else if (streq(outform, "asm")) {
		dt_to_asm(outf, bi, outversion);
	} else if (streq(outform, "null")) {
		/* do nothing */
		die("Unknown output format is null\n");
		goto err;
	} else {
		die("Unknown output format \"%s\"\n", outform);
		goto err;
	}

	if (outf) {
		int outf_fd = fileno(outf);
		fsync(outf_fd);
		fclose(outf);
		outf = NULL;
	}
	return 0;

err:
	if (outf) {
		int outf_fd = fileno(outf);
		fsync(outf_fd);
		flose(outf);
		outf = NULL;
	}
	return -1;
}
