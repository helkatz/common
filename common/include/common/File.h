#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <time.h>
#include <assert.h>
//#include <logger/Logger.h>

template<typename T, typename... ARGS>
std::string fmt(const T& value)
{
}
namespace common
{
	class File
	{
		struct BufferBase
		{
			static char noLF;
			char *head{ nullptr };
			char *line_tail{ nullptr };
			char *line_head{ nullptr };
			char *lastCR{ &noLF };
			char *lastLF{ &noLF };
			BufferBase()
			{
				reset();
			}

			void reset()
			{
				restoreCRLF();
				head = nullptr;
				line_tail = nullptr;
				line_head = nullptr;
				lastCR = &noLF;
				lastLF = &noLF;
			}

			BufferBase(const BufferBase& other) :
				head(other.head),
				line_tail(other.line_tail),
				line_head(other.line_head),
				lastCR(other.lastCR),
				lastLF(other.lastLF)
			{
			}

			void shift(int32_t offset)
			{
				head += offset;
				line_tail += offset;
				line_head += offset;
				if (lastCR != &noLF)
					lastCR += offset;
				if (lastLF != &noLF)
					lastLF += offset;
			}

			void swap(BufferBase& other)
			{
				BufferBase tmp(*this);
				*this = other;
				other = tmp;
			}

			int clearCRLF(char *buffer)
			{
				lastLF = lastCR = &noLF;
				if (*buffer == '\r') {
					lastCR = buffer;
					if(*(buffer + 1) == '\n')
						lastLF = buffer + 1;
				}
				else if (*buffer == '\n') {
					lastLF = buffer;
					if (*(buffer - 1) == '\r')
						lastCR = buffer - 1;
				}
				*lastLF = *lastCR = 0;
				return (lastLF != &noLF ? 1 : 0) + (lastCR != &noLF ? 1 : 0);
				return (lastLF == buffer ? 1 : 0) + (lastCR == (buffer - 1) ? 1 : 0);
			}

			int restoreCRLF()
			{
				assert(*lastLF != ' ' && *lastCR != ' ');

				*lastLF = '\n';
				*lastCR = '\r';				
				int size = (lastCR != &noLF) + (lastLF != &noLF);
				lastLF = lastCR = &noLF;
				return size;
			}
		};
		struct Buffer: BufferBase
		{
			uint32_t max_size{ 0x100000 };	// max buffer size 
			uint32_t init_size{ 0x10000 };	// init or grow size
			uint32_t size{ 0 };				// actual buffer size
			uint32_t posInBuffer{ 0 };		// based on buffer start
			uint64_t posInFile{ 0 };		// based on file start
			uint64_t totalReaded{ 0 };
			char *allocated{ nullptr };
			char *working{ nullptr };
			char *keep_at_once_head{ nullptr };
			char *keep_at_once_tail{ nullptr };
			
			BufferBase save;


			Buffer();

			void reset();

			bool resize(uint32_t size, bool append = true);

			void shift(int32_t offset)
			{
				BufferBase::shift(offset);
				if (save.head)
					save.shift(offset);				
				if (keep_at_once_head)
					keep_at_once_head += offset;
				if (keep_at_once_tail)
					keep_at_once_tail += offset;
			}

			uint32_t readed() { return static_cast<uint32_t>(totalReaded - posInFile); }

			char *readForwardToLineHead()
			{
				while (*line_head > '\r' || 
					*line_head != '\n' && *line_head != '\r' && *line_head) 
					line_head++;

				if (*line_head == '\n' || *line_head == '\r') {
					int cleared = clearCRLF(line_head);
					//line_head -= (cleared - 1);
					return line_tail;
				}
				return nullptr;
			}
			char *readBackwardToLineTail()
			{
				// when line_tail is in \n then the previous line is required
				if (line_tail == working && (*line_tail == '\n' || *line_tail == '\r'))
					return nullptr;
				while (line_tail > working && *--line_tail != '\n' && *line_tail != '\r');

				if (*(line_tail) == '\n') {
					return ++line_tail;
				}
				if (*(line_tail) == '\r') {
					return line_tail += (*(line_tail+1) == '\n' ? 2 : 1);
				}
				return nullptr;
			}

			uint8_t getPrevLineBreakSize() 
			{
				return (*(line_tail - 1) == '\n')
					? 1 + (*(line_tail - 2) == '\r')
					: *(line_tail - 1) == '\r';
			};

			uint8_t getNextLineBreakSize()
			{
				return (*(line_head + 1) == '\n')
					? 1
					: (*(line_head + 1) == '\r') + (*(line_head + 2) == '\n');
			};
		};

		static const char none = 0;
		static const char eof = 1;
		static const char peeked = 2;
		static const char prepare_peek = 4;

		mutable FILE *_fs{ nullptr };
		std::string _fileName;
		Buffer _buf;

		struct ReadSize {
			static const int _max_forward{ std::numeric_limits<int>::max() };
			static const int _max_backward{ std::numeric_limits<int>::min() };
			int _size = 0;
			explicit ReadSize(const int size): _size(size) {

			}

			static ReadSize max_forward() { return ReadSize{ _max_forward }; };
			static ReadSize max_backward() { return ReadSize{ _max_backward }; };
			bool is_max() { return _size == _max_forward || _size == _max_backward;  }
			bool is_forward(){ return _size > 0; }
			bool is_backward() { return _size < 0; }
		};

		char _flags{ none };
		char _flags_save{ none };
		

		bool _isClone{ false };
		uint32_t readBuffer(const ReadSize = ReadSize::max_forward());
		uint32_t readForwardToLineHead(const ReadSize& readSize = ReadSize::max_forward())
		{
			uint32_t pos = posHead();
			while (_buf.readForwardToLineHead() == nullptr && readBuffer(readSize) > 0);
			return posHead() - pos;
		}
		uint32_t readBackwardToLineTail(const ReadSize& readSize = ReadSize::max_backward())
		{
			uint32_t pos = posTail();
			while (_buf.readBackwardToLineTail() == nullptr && readBuffer(readSize) > 0);
			return pos - posTail();
		}
	public:
		File();

		File(const File& other)
		{
			_isClone = true;
			_fs = nullptr;
			*this = other;
		}


		~File();

		File& operator = (const File& other);

		bool open(const std::string& fileName);

		bool hasPrev() const;

		bool hasNext() const;

		uint64_t size() const;

		uint64_t posTail() const;

		uint64_t posHead() const;

		//uint64_t pos() const;

		//uint64_t posLineStart() const;	

		void reset();

		void seek(uint64_t pos);

		void seekEnd();

		char *readPrevLine();

		char *readLine();

		char *peekLine();

		char *readPrevLine(const char *condition);

		char *readLine(const char *condition);

		char *getCurrentLine();

		void keepBufferAtOnce(bool enable);

		void keepBufferAtOnce(char *ptr);

		char *getBufferAtOnce();

		const std::string& getFileName() const { return _fileName; }

		void setBufSize(size_t);
	};
}