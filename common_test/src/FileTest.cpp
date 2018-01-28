#include <common/File.h>
#include <common/StringFormat.h>
#include <gtest/gtest.h>
#include <logger/Logger.h>
#include <random>
class FileTest : public ::testing::Test
{
protected:
	common::File f;
	static const uint32_t lines = 10000;
	uint32_t loops = 1000;
	static void genTestFile(const std::string& path, uint32_t lines, uint32_t minMsgLen, uint32_t maxMsgLen)
	{
		FILE *f = fopen(path.c_str(), "wb+");
		//std::string msg = "this is a testmessage for file test and this line will be randomly truncated and written to the testfile";
		std::string msg;
		msg.reserve(maxMsgLen);
		for (int i = 0; i < maxMsgLen; i++)
			msg += common::sfmt("%d", i % 10);
		for (int i = 1; i <= lines; i++) {
			std::string outmsg;
			//if (i > 1) outmsg += "\n";
			outmsg += common::sfmt("%d %s", i, msg);
			outmsg.resize(std::max(minMsgLen, rand() % maxMsgLen) - (i < lines ? 1 : 0)); // -1 for following \n
			if(i < lines)
				fprintf(f, "%s\n", outmsg.c_str());
			else
				fprintf(f, "%s", outmsg.c_str());
		}
		fclose(f);
	}
	static void SetUpTestCase()
	{
		//genTestFile("testfile.txt", lines, 20, 200);
		//genTestFile("testfile_fixed_len.txt", 10000, 100, 100);
		logger::Logger::set_level(".*", logger::Logger::Level::Warning);
	}

	FileTest()
	{
		//f.open("/logs/error_huge.loglined");
		f.open("testfile.txt");
	}
};

template <typename>
struct is_map : std::false_type
{ };

template <class _Kty, class _Ty, class _Pr, class _Alloc>
struct is_map<std::map<_Kty, _Ty, _Pr, _Alloc>> : std::true_type
{ };

template<typename T>
typename std::enable_if<is_map<T>::value, bool>::type
func(const T&)
{
	return true;
}

template<typename T>
typename std::enable_if<!is_map<T>::value, bool>::type
func(const T&)
{
	return true;
}
TEST_F(FileTest, misc)
{
	std::map<int, int> m;
	std::vector<int> v;
	func(m);
	func(v);

}

TEST_F(FileTest, seek_to_begin)
{
	f.open("testfile_fixed_len.txt");
	auto seekPos = 0;
	{
		f.seek(seekPos);
		auto line = f.readLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(1, atoi(line));
	}
	{
		f.seek(seekPos);
		auto line = f.readPrevLine();
	}

}

TEST_F(FileTest, seek_and_read)
{
	return;
	//genTestFile("testfile_fixed_len.txt", 100000, 100, 100);
	f.open("testfile_fixed_len.txt");
	auto seekPos = 0;
	std::random_device rd;     // only used once to initialise (seed) engine
	std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni(0, f.size());
	while(true) {
		
		seekPos = uni(rng);
		f.seek(seekPos);
		if (f.hasNext() == false)
			break;
		auto line = f.readLine();
		
		ASSERT_TRUE(line != nullptr);
		uint32_t lineNr = (seekPos / 100) + 1;
		ASSERT_EQ(lineNr, atoi(line));
		log_trace(0) << common::sfmt("line=%1%", lineNr);
	}
}

struct CmpFile
{
	char _buf[0xffff];
	uint64_t _pos;
	FILE *_fs;
	void open(const std::string& path)
	{
		_fs = fopen(path.c_str(), "r");
	}
	void seek(uint64_t pos)
	{
		_pos = pos;
		_fseeki64(_fs, _pos, SEEK_SET);
	}
	std::string readLine()
	{
		while (true) {
			seek(_pos - 100);
			int readed = fread(_buf, 1, 100, _fs);
			while (readed > 0) {
				if (_buf[readed] == '\n') {
					seek(_pos + readed + 1);
					fgets(_buf, sizeof(_buf), _fs);
					size_t ln = strlen(_buf) - 1;
					if (_buf[ln] == '\n')
						_buf[ln] = '\0';
					return _buf;
				}
				readed--;
			}
		}
	}
};

TEST_F(FileTest, seek_and_readprev_and_readnext)
{
	//genTestFile("testfile_variable_len.txt", 100000, 20, 100);
	f.open("testfile_variable_len.txt");
	
	auto seekPos = 0;
	std::random_device rd;     // only used once to initialise (seed) engine
	std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni(0, f.size());
	//FILE *fcmp = fopen("testfile_variable_len.txt", "rb");
	CmpFile cmpFile;
	cmpFile.open("testfile_variable_len.txt");
	std::string cmpLine;
	
	while (loops--) {

		seekPos = uni(rng);
		
		f.seek(seekPos);
		if (f.hasNext() == false)
			break;
		
		auto line = f.readLine();
		ASSERT_TRUE(line != nullptr);

		cmpFile.seek(seekPos);
		cmpLine = cmpFile.readLine();
		ASSERT_EQ(line, cmpLine);
	}
}

TEST_F(FileTest, seek_and_read_and_compare_line_nativ)
{
	//genTestFile("testfile_variable_len.txt", 100000, 20, 100);
	f.open("testfile_variable_len.txt");
	//logger::Logger::set_level(".*", logger::Logger::Level::Trace2);
	auto seekPos = 0;
	std::random_device rd;     // only used once to initialise (seed) engine
	std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni(0, f.size());
	//FILE *fcmp = fopen("testfile_variable_len.txt", "rb");
	CmpFile cmpFile;
	cmpFile.open("testfile_variable_len.txt");
	std::string cmpLine;

	while(loops--) {

		seekPos = uni(rng);

		f.seek(seekPos);
		if (f.hasNext() == false)
			break;

		auto line = f.readLine();
		ASSERT_TRUE(line != nullptr);

		cmpFile.seek(seekPos);
		cmpLine = cmpFile.readLine();
		ASSERT_EQ(line, cmpLine);
	}
}

TEST_F(FileTest, readLine)
{
	uint32_t lineNr = 0;
	while (f.hasNext()) {
		lineNr++;
		auto line = f.readLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));
	}
	ASSERT_EQ(lines, lineNr);
}

TEST_F(FileTest, readLine_with_keepTail)
{
	using namespace logger;
	//log_set_scoped_level(".*", Trace2);
	uint32_t lineNr = 0;
	f.keepBufferAtOnce(true);
	while (f.hasNext()) {
		lineNr++;
		if (lineNr > 2610)
			log_debug();
		auto line = f.readLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));		
	}
	ASSERT_EQ(lines, lineNr);
}

TEST_F(FileTest, readPrevLine)
{
//	genTestFile("testfile_variable_len_huge.txt", 1000000, 20, 200);
	f.open("testfile_variable_len_huge.txt");
	//log_set_scoped_level(".*", Trace2);
	uint32_t lineNr = 1000000 + 1;
	f.seekEnd();
	while (f.hasPrev()) {
		lineNr--;
		auto line = f.readPrevLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));
		
	}
	ASSERT_EQ(1, lineNr);
}
TEST_F(FileTest, readPrevLine_with_keepTail)
{
	uint32_t lineNr = lines + 1;
	f.seekEnd();
	f.keepBufferAtOnce(true);
	while (f.hasPrev()) {
		lineNr--;
		auto line = f.readPrevLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));		
	}
	ASSERT_EQ(1, lineNr);
}
TEST_F(FileTest, findbug)
{
	return;
	int pos_arr[] = {
		70757,
		1192552
	};
	common::File f;
	f.open("/logs/error_huge.loglined");
	FILE *fout = fopen("arr", "wb+");
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
