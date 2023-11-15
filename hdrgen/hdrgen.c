#include <lrtypes.h>
#include <stdio.h>
#include <string.h>

struct romheader_t {
	char name[32];
	u32 filesize;
	u32 flags;
	u32 spritefollow;
	u32 reserved;
};

enum {
	FLAG_PPUHACK_ENABLE = 	1 << 0,
	FLAG_CPUHACK_DISABLE =	1 << 1,
	FLAG_PAL = 		1 << 2,
	FLAG_ADDRESSFOLLOW = 	1 << 5, // if 0, spritefollow
};

int main() {
	struct romheader_t hdr;

	memset(&hdr, 0, sizeof(struct romheader_t));

	strcpy(hdr.name, "game");
	hdr.filesize = 262160;
	hdr.flags = 0;
	hdr.spritefollow = 0;

	FILE *f = fopen("hdr.bin", "w");
	if (!f)
		return 1;
	fwrite(&hdr, sizeof(struct romheader_t), 1, f);
	fclose(f);

	return 0;
}
