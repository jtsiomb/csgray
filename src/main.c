/*
csgray - simple CSG raytracer
Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "csgray.h"

#define DFL_WIDTH	800
#define DFL_HEIGHT	600
#define DFL_GAMMA	2.2f
#define DFL_OUTFILE	"output.ppm"

static int save_image(const char *fname, float *pix, int xsz, int ysz);
static int parse_opt(int argc, char **argv);

static int width = DFL_WIDTH, height = DFL_HEIGHT;
static float inv_gamma = 1.0f / DFL_GAMMA;
static const char *out_fname = DFL_OUTFILE;
static const char *in_fname;

int main(int argc, char **argv)
{
	csg_object *oa, *ob, *oc, *obj;
	float *pixels;

	if(parse_opt(argc, argv) == -1) {
		return 1;
	}

	if(csg_init() == -1) {
		return 1;
	}

	if(csg_load(in_fname) == -1) {
		return 1;
	}

	if(!(pixels = malloc(width * height * 3 * sizeof *pixels))) {
		perror("failed to allocate framebuffer");
		return 1;
	}

	csg_render_image(pixels, width, height);
	save_image(out_fname, pixels, width, height);

	csg_destroy();
	return 0;
}

static int save_image(const char *fname, float *pix, int xsz, int ysz)
{
	int i;
	FILE *fp;

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "failed to open %s for writing: %s\n", fname, strerror(errno));
		return -1;
	}

	fprintf(fp, "P6\n%d %d\n255\n", xsz, ysz);

	for(i=0; i<xsz * ysz; i++) {
		unsigned int r = pow(*pix++, inv_gamma) * 255.0f;
		unsigned int g = pow(*pix++, inv_gamma) * 255.0f;
		unsigned int b = pow(*pix++, inv_gamma) * 255.0f;

		if(r > 255) r = 255;
		if(g > 255) g = 255;
		if(b > 255) b = 255;

		fputc(r, fp);
		fputc(g, fp);
		fputc(b, fp);
	}
	fclose(fp);
	return 0;
}

static void print_usage(const char *argv0)
{
	printf("Usage: %s [options] <csg file>\n", argv0);
	printf("Options:\n");
	printf(" -s <WxH>   output image resolution (default: %dx%d)\n", DFL_WIDTH, DFL_HEIGHT);
	printf(" -g <gamma> set output gamma (default: %g)\n", DFL_GAMMA);
	printf(" -o <file>  output image file (default: %s)\n", DFL_OUTFILE);
	printf(" -h         print usage information and exit\n");
}

static int parse_opt(int argc, char **argv)
{
	int i;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(argv[i][2] == 0) {
				switch(argv[i][1]) {
				case 's':
					if(sscanf(argv[++i], "%dx%d", &width, &height) != 2) {
						fprintf(stderr, "-s must be followed by WIDTHxHEIGHT\n");
						return -1;
					}
					break;

				case 'g':
					if((inv_gamma = atof(argv[++i])) == 0.0f) {
						fprintf(stderr, "-g must be followed by a non-zero gamma value\n");
						return -1;
					}
					inv_gamma = 1.0f / inv_gamma;
					break;

				case 'o':
					out_fname = argv[++i];
					break;

				case 'h':
					print_usage(argv[0]);
					exit(0);

				default:
					fprintf(stderr, "invalid option: %s\n", argv[i]);
					return -1;
				}
			} else {
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return -1;
			}
		} else {
			if(in_fname) {
				fprintf(stderr, "unexpected argument: %s\n", argv[i]);
				return -1;
			}
			in_fname = argv[i];
		}
	}

	if(!in_fname) {
		fprintf(stderr, "you need to pass a scene file to read\n");
		return -1;
	}

	return 0;
}
