#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <time.h>
//#include <logger/Logger.h>

template<typename T, typename... ARGS>
std::string fmt(const T& value)
{
}
namespace common
{
	class File
	{
		friend class Parser;
#ifdef USE_FSTREAM
		std::fstream _fs;
#else
		mutable FILE *_fs;
#endif
		std::string _fileName;
#if 0
		struct Buffer
		{
			uint32_t max_size = 0x100000;
			uint32_t init_size = 0x10000;
			uint32_t size;
			char *allocated;
			char *working;
			char *keep_at_once;
			struct Cur
			{
				static char noLF;
				char *head;
				char *line_tail;
				char *line_head;
				//char *keep_at_once;
				char *lastCR;
				char *lastLF;
				Cur()
				{
					reset();
				}

				void reset()
				{
					head = nullptr;
					line_tail = nullptr;
					line_head = nullptr;
					lastCR = &noLF;
					lastLF = &noLF;
				}

				Cur(const Cur& other) :
					head(other.head),
					line_tail(other.line_tail),
					line_head(other.line_head),
					lastCR(other.lastCR),
					lastLF(other.lastLF)
				{

				}

				void swap(Cur& other)
				{
					Cur tmp(*this);
					*this = other;
					other = tmp;
				}

				int clearCRLF(char *buffer)
				{
					if (*buffer == '\n')
						lastLF = buffer;
					if (*(buffer - 1) == '\r')
						lastCR = buffer - 1;
					*lastLF = *lastCR = 0;
					return (lastLF == buffer ? 1 : 0) + (lastCR == (buffer - 1) ? 1 : 0);
				}

				int restoreCRLF()
				{
					if (*lastLF == ' ' || *lastCR == ' ')
						_CrtDbgBreak();
					*lastLF = '\n';
					*lastCR = '\r';
					int size = (lastCR != &noLF) + (lastLF != &noLF);
					lastLF = lastCR = &noLF;
					return size;
				}
			};

			Cur cur;
			Cur save;

			Buffer() :
				size(0),
				allocated(nullptr),
				working(nullptr),
				keep_at_once(nullptr)
			{
			}

			void reset();

			void resize(uint32_t size, bool append);

			void saveBuffer()
			{
				save = cur;
				save.clearCRLF(save.head);
			}

			void restoreBuffer()
			{
				save.restoreCRLF();
				cur = save;
				clearSaveBuffer();
			}

			void clearSaveBuffer()
			{
				save.restoreCRLF();
				new(&save) Cur;

				//memset(&save, 0, sizeof(save));
			}
		};
#endif

		struct BufferBase
		{
			static char noLF;
			char *head;
			char *line_tail;
			char *line_head;
			char *lastCR;
			char *lastLF;
			BufferBase()
			{
				reset();
			}

			void reset()
			{
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

			void swap(BufferBase& other)
			{
				BufferBase tmp(*this);
				*this = other;
				other = tmp;
			}

			int clearCRLF(char *buffer)
			{
				if (*buffer == '\n')
					lastLF = buffer;
				if (*(buffer - 1) == '\r')
					lastCR = buffer - 1;
				*lastLF = *lastCR = 0;
				return (lastLF == buffer ? 1 : 0) + (lastCR == (buffer - 1) ? 1 : 0);
			}

			int restoreCRLF()
			{
				if (*lastLF == ' ' || *lastCR == ' ')
					_CrtDbgBreak();
				*lastLF = '\n';
				*lastCR = '\r';
				int size = (lastCR != &noLF) + (lastLF != &noLF);
				lastLF = lastCR = &noLF;
				return size;
			}
		};
		struct Buffer: BufferBase
		{
			uint32_t max_size = 0x100000;
			uint32_t init_size = 0x10000;
			uint32_t size;
			char *allocated;
			char *working;
			char *keep_at_once;
			BufferBase& cur = *this;
			BufferBase save;

			Buffer() :
				size(0),
				allocated(nullptr),
				working(nullptr),
				keep_at_once(nullptr)
			{
			}

			void reset();

			void resize(uint32_t size, bool append);

			void saveBuffer()
			{
				save = cur;
				save.clearCRLF(save.head);
			}

			void restoreBuffer()
			{
				save.restoreCRLF();
				cur = save;
				clearSaveBuffer();
			}

			void clearSaveBuffer()
			{
				save.restoreCRLF();
				new(&save) Cur;

				//memset(&save, 0, sizeof(save));
			}
		};
		Buffer _buf;

		const int eof = 1;
		const int peeked = 2;
		const int prepare_peek = 4;
		char _flags;
		char _flags_save;
		uint64_t _bufferStartPos;	/// holds the file pos where the buffer starts
		uint64_t _totalReaded;

		bool _isClone;
		uint32_t readBuffer(bool backward = false);
		void resetBuffer();
	public:
		File();

		File(const File& other)
		{
			_isClone = true;
			_fs = nullptr;
			*this = other;
		}


		~File();

		File& operator = (const File& other)
		{

			// other._fs;
			_fileName = other._fileName;
			_buf = other._buf;
			_flags = other._flags;
			_flags_save = other._flags_save;
			_bufferStartPos = other._bufferStartPos;
			_totalReaded = other._totalReaded;
			return *this;
		}

		void resizeBuffer(uint32_t size, bool append = true);

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
	};
}