#include <common/File.h>
#include <common/StringFormat.h>
#include <gtest/gtest.h>
#include <logger/Logger.h>
#include <random>
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
	std::string readLine(bool forward = true)
	{
		while (true) {
			seek(_pos >= 100 ? _pos - 100 : 0);
			int readed = fread(_buf, 1, 100, _fs);
			if (readed <= 0)
				return "";
			if (!forward && _buf[readed - 1] == '\n')
				readed--;
			while (readed > 0) {
				if (_buf[readed - 1] == '\n') {
					seek(_pos + readed);
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

class FileTest : public ::testing::Test
{
protected:
	common::File f;
	static const uint32_t lines = 100000;
	uint32_t loops = 1000;
	enum class LineBreakMethod
	{
		LF, CRLF, CR, RANDOM
	};
	static void genTestFile(const std::string& path, uint32_t lines
		, uint32_t minMsgLen, uint32_t maxMsgLen, LineBreakMethod lb = LineBreakMethod::LF)
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
			
			outmsg.resize(std::max(std::max(std::to_string(i).length(), minMsgLen) + 1, rand() % maxMsgLen) - (i < lines ? 1 : 0)); // -1 for following \n
			auto getLineBreak = [](LineBreakMethod lb) {
				while (true) {
					switch (lb) {
					case LineBreakMethod::LF: return "\n";
					case LineBreakMethod::CRLF: return "\r\n";
					case LineBreakMethod::CR: return "\r";
					case LineBreakMethod::RANDOM: lb = static_cast<LineBreakMethod>(rand() % 3);
					}
				}
			};
			fprintf(f, "%s", outmsg.c_str());
			if (i < lines)
				fprintf(f, getLineBreak(lb));
		}
		fclose(f);
	}
	static void SetUpTestCase()
	{
		//genTestFile("testfile.txt", lines, 20, 200);
		//genTestFile("testfile_fixed_len_10000_100_100.txt", 10000, 100, 100);
		//genTestFile("testfile_variable_len_10000_1_100.txt", 10000, 1, 100);
		//genTestFile("testfile_random_linebreak.txt", lines, 20, 200, LineBreakMethod::RANDOM);
		logger::Logger::set_level(".*", logger::Logger::Level::Warning);
	}

	FileTest()
	{
		//f.open("/logs/error_huge.loglined");
		f.open("testfile.txt");
	}

	~FileTest()
	{
		if (HasFailure()) {
			std::cout << common::sfmt("loops = %1%\n", loops);
		}
	}

	void breakWhen(bool condition)
	{
		if (condition) _CrtDbgBreak();
	}
};


TEST_F(FileTest, misc)
{
	FILE *f = fopen("c:\\logs\\testing.log", "w+");
	int index = 0;
	for (index = 0; index < 100000; index++) {
		std::string msg =
			(boost::format("2018-02-17 | 09:39:43.043 | I | T0 | testing | D:\data\repos\service_layer\daemons\customer_cache\src\cache.cpp:139 | %1% this is the message %1%\n") % index).str();
		fputs(msg.c_str(), f);
		fflush(f);
	}
	fclose(f);
}

TEST_F(FileTest, misc_linebreaks)
{
	f.open("testfile_random_linebreak.txt");
	uint32_t lineNr = 0;
	while (f.hasNext()) {
		lineNr++;
		//breakWhen(lineNr == 99999);
		auto line = f.readLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));
	}
	ASSERT_EQ(100000, lineNr);
}

TEST_F(FileTest, seek_to_begin)
{
	f.open("testfile_fixed_len_10000_100_100.txt");
	auto seekPos = 0;
	{
		ASSERT_EQ(0, f.posTail());
		ASSERT_EQ(0, f.posHead());
		ASSERT_FALSE(f.hasPrev());
		ASSERT_TRUE(f.hasNext());
		f.seek(seekPos);
		auto line = f.readLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(1, atoi(line));
		f.seek(seekPos);
		line = f.readPrevLine();
		ASSERT_EQ(0, atoi(line));
	}
}

TEST_F(FileTest, seek_to_end)
{
	f.open("testfile_fixed_len_10000_100_100.txt");
	auto seekPos = 0;
	{
		f.seekEnd();
		ASSERT_EQ(f.size(), f.posTail());
		ASSERT_EQ(f.size(), f.posHead());
		ASSERT_TRUE(f.hasPrev());
		ASSERT_FALSE(f.hasNext());
		
		auto line = f.readLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(10000, atoi(line));
	}
	{
		f.seekEnd();
		auto line = f.readPrevLine();
		ASSERT_EQ(10000, atoi(line));
	}
}

TEST_F(FileTest, seek_and_read)
{
	//return;
	//genTestFile("testfile_fixed_len.txt", 100000, 100, 100);
	f.open("testfile_fixed_len_10000_100_100.txt");
	auto seekPos = 0;
	std::random_device rd;     // only used once to initialise (seed) engine
	std::default_random_engine rng;    // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni(0, f.size());
	while(loops--) {
		//breakWhen(loops == 980);
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



TEST_F(FileTest, seek_and_readprev_and_readnext)
{
	//genTestFile("testfile_variable_len.txt", 100000, 20, 100);
	f.open("testfile_variable_len.txt");
	
	auto seekPos = 0;
	std::random_device rd;     // only used once to initialise (seed) engine
	std::default_random_engine rng;    // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni(0, f.size());
	//FILE *fcmp = fopen("testfile_variable_len.txt", "rb");
	CmpFile cmpFile;
	cmpFile.open("testfile_variable_len.txt");
	std::string cmpLine;
	
	while (loops--) {
		//breakWhen(loops == 999);
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

TEST_F(FileTest, seek_and_readprev_and_compare_line_nativ)
{
	//genTestFile("testfile_variable_len.txt", 100000, 20, 100);
	f.open("testfile_variable_len.txt");
	//logger::Logger::set_level(".*", logger::Logger::Level::Trace2);
	auto seekPos = 0;
	std::random_device rd;     // only used once to initialise (seed) engine
	std::default_random_engine rng;    // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni(0, f.size());
	//FILE *fcmp = fopen("testfile_variable_len.txt", "rb");
	CmpFile cmpFile;
	cmpFile.open("testfile_variable_len.txt");
	std::string cmpLine;

	while (loops--) {
		//if(loops == 910) _CrtDbgBreak();
		seekPos = uni(rng);

		f.seek(seekPos);
		if (f.hasPrev() == false)
			break;

		auto line = f.readPrevLine();
		ASSERT_TRUE(line != nullptr);

		cmpFile.seek(seekPos);
		cmpLine = cmpFile.readLine(false);
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
	std::default_random_engine rng;    // random-number engine used (Mersenne-Twister in this case)
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
	f.open("testfile_variable_len_10000_1_100.txt");
	uint32_t lineNr = 0;
	while (f.hasNext()) {
		lineNr++;
		auto line = f.readLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));
	}
	ASSERT_EQ(10000, lineNr);
}

TEST_F(FileTest, readLine_with_keepTail)
{
	using namespace logger;
	//log_set_scoped_level(".*", Trace2);
	f.open("testfile_variable_len_10000_1_100.txt");
	uint32_t lineNr = 0;
	f.keepBufferAtOnce(true);
	while (f.hasNext()) {
		lineNr++;
		auto line = f.readLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));		
	}
	ASSERT_EQ(10000, lineNr);
	auto lines = f.getBufferAtOnce();
	ASSERT_EQ(1, atoi(lines));
	auto lastLine = strrchr(lines, '\n');
	ASSERT_EQ(10000, atoi(lastLine));
	
}


TEST_F(FileTest, peekLine)
{
	f.open("testfile_fixed_len_10000_100_100.txt");
	uint32_t lineNr = 0;
	while (f.hasNext()) {
		loops--;
		lineNr++;
		//breakWhen(loops == 4294958296);
		auto line = f.peekLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));

		// must be the same
		line = f.peekLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));

		// must be the same but a next peek or read will response the next line
		line = f.readLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));
		// linelength is always - CR except on last
		if (f.hasNext())
			ASSERT_EQ(99, strlen(line));
		else
			ASSERT_EQ(100, strlen(line));
	}
	ASSERT_EQ(10000, lineNr);
}

TEST_F(FileTest, frontiers)
{
	genTestFile("testfile_frontiers.txt", 100, 100, 100);
	f.setBufSize(101);
	f.open("testfile_frontiers.txt");
	//log_set_scoped_level(".*", Trace2);
	uint32_t lineNr = 100 + 1;
	f.seekEnd();
	while (f.hasPrev()) {
		lineNr--;
		//if (lineNr == 979621) _CrtDbgBreak();
		auto line = f.readPrevLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));

	}
	ASSERT_EQ(1, lineNr);
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
		//if (lineNr == 999905) _CrtDbgBreak();
		auto line = f.readPrevLine();
		ASSERT_TRUE(line != nullptr);
		ASSERT_EQ(lineNr, atoi(line));
		
	}
	ASSERT_EQ(1, lineNr);
}

TEST_F(FileTest, readPrevLine_with_keepTail)
{
	uint32_t lineNr = 10000 + 1;
	f.open("testfile_variable_len_10000_1_100.txt");
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
