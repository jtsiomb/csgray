#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "csgray.h"

static int save_image(const char *fname, float *pix, int xsz, int ysz);
static int parse_opt(int argc, char **argv);

static int width = 800, height = 600;
static const char *out_fname = "output.ppm";

int main(int argc, char **argv)
{
	csg_object *oa, *ob, *oc;
	float *pixels;

	if(parse_opt(argc, argv) == -1) {
		return 1;
	}

	if(csg_init() == -1) {
		return 1;
	}

	if(!(pixels = malloc(width * height * 3 * sizeof *pixels))) {
		perror("failed to allocate framebuffer");
		return 1;
	}

	oa = csg_sphere(0, 0, 0, 1);
	ob = csg_sphere(0, 1, 0, 0.8);
	oc = csg_intersection(oa, ob);

	csg_add_object(0, oc);

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

	fprintf(fp, "P6\n%d %d\n65535\n", xsz, ysz);

	for(i=0; i<xsz * ysz; i++) {
		unsigned int r = *pix++ * 65535.0f;
		unsigned int g = *pix++ * 65535.0f;
		unsigned int b = *pix++ * 65535.0f;

		if(r > 0xffff) r = 0xffff;
		if(g > 0xffff) g = 0xffff;
		if(b > 0xffff) b = 0xffff;

		fputc(r >> 8, fp);
		fputc(r, fp);
		fputc(g >> 8, fp);
		fputc(g, fp);
		fputc(b >> 8, fp);
		fputc(b, fp);
	}
	fclose(fp);
	return 0;
}

static void print_usage(const char *argv0)
{
	printf("Usage: %s [options] <csg file>\n", argv0);
	printf("Options:\n");
	printf(" -s <WxH>  output image resolution\n");
	printf(" -o <file> output image file\n");
	printf(" -h        print usage information and exit\n");
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
			fprintf(stderr, "unexpected argument: %s\n", argv[i]);
			return -1;
		}
	}
	return 0;
}
