#include <common/File.h>

#include <gtest/gtest.h>

TEST(FileTest, readLine)
{
	int pos_arr[] = {
		70757,
		1192552
	};
	common::File f;
	f.open("/logs/error_huge.loglined");
	FILE *fout = fopen("arr", "w+");
	while (true) {
		auto pos = rand() + 1;
		pos = f.size() / pos;
		for (auto pos : pos_arr) {
			f.seek(pos);
			f.keepBufferAtOnce(true);
			int linenr = 0;
			while (f.hasNext() && ++linenr <= 2) {
				f.readPrevLine();
				f.readLine();
				f.readLine();				
				const char *line = f.readLine();
				printf("pos %d line %d linelen=%d\n", pos, linenr, strlen(line));
				fprintf(fout, "%d,", pos);
			}
		}
	}
}
