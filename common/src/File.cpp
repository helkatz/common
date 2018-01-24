#include <common/File.h>
#include <logger/Logger.h>

#include <boost/format.hpp>

#define BUFFER_GAP 10
#define RSIZE(SIZE) SIZE + BUFFER_GAP * 2
#define WBUF(BUF) (BUF + BUFFER_GAP)
/**
Buffering

0        9          20                   41                         70                           100
|-------------------------------FILE--------------------------------------------------------------|
|        |---KF-----|                    |---KB---------------------|-----------------------------|
|        WB         LT------LINE---------LH
|--------|--------------WORKINGBUFFER------WS---WS---WS---WS---WS---|-----------------------------|

WB = workingbuffer start
WS = size of working buffer
KF = keep buffer at once start for forward reading KF can be between WB and LT
KB = keep buffer at once start for backward reading KB can be betwenn LH and WB+WSIZE
LT = line tail start of current line
LH = line head end of current line
FP = tells us the filpos at WB

iWB = iKF < iLT ? iKF : iLT
BTM = WS - iLH

MF  = KF < LT ? KF : LT       MF  = LT
BTM = LH - MF                 BTM = (KB > LH ? KB : LH) - MF
MO  = MF -WB                  MO  = WS - BTM - MF - WB
BTR = WS - BTM                BTR = WS - BTM
FP  = FP + MF - WB            FP  = FP > BTR THEN WS -

Buffer realoading and resizing
Behavior on forward reading.
When during reading a line the end of buffer is reached without an line break then we have to load
the next part of the file into the buffer and we have to keep all readings.
So all bytes we want to keep aremoved to the begin of the buffer and new bytes are appendeed

On backward reading we move the keeping buffer to the end and fill up the gap between WB and the
kept buffer

In both cases the associated filepos FP 

    forward
  _bufferStartPos -= (_buf.size - (keep_head - _buf.working))


*/
#define printbuf(msg) printf("%s bsize=%d cur.line_tail=%d cur.line_head=%d\n", \
		msg, _buf.size,\
		_buf.cur.line_tail - _buf.working, \
		_buf.cur.line_head - _buf.working)
namespace common {
	char File::Buffer::Cur::noLF;

	File::File() :
		_fs(nullptr),
		_bufferStartPos(0),
		_totalReaded(0),
		_flags(0),
		_isClone(false)
	{
		resizeBuffer(0x10000, true);
	}

	File::~File()
	{
		if (_fs)
			fclose(_fs);
		if (_isClone == false)
			delete[] _buf.allocated;
	}

	bool File::open(const std::string& fileName)
	{
		_fileName = fileName;
		if (_fs)
			fclose(_fs);
		log_debug() << "";
		log_trace(1) << fileName;
		_fs = _fsopen(fileName.c_str(), "rb", _SH_DENYNO);
		if (!_fs) {
			log_error() << "cannot open" << fileName;
			return false;
		}
		reset();
		return true;
	}

	bool File::hasPrev() const
	{
		return posTail() > 0;
	}

	bool File::hasNext() const
	{
		return !(_flags & eof);
	}

	uint64_t File::size() const
	{
		uint64_t cur = _ftelli64(_fs);
		_fseeki64(_fs, 0, SEEK_END);
		uint64_t size = _ftelli64(_fs);
		_fseeki64(_fs, cur, SEEK_SET);
		return size;
	}

	uint64_t File::posTail() const
	{
		return _bufferStartPos + (_buf.cur.line_tail - _buf.working);
	}

	uint64_t File::posHead() const
	{
		return _flags & eof ? -1 : _bufferStartPos
			+ (_buf.cur.line_head - _buf.working)
			+ (_buf.cur.lastCR != &Buffer::Cur::noLF)
			+ (_buf.cur.lastLF != &Buffer::Cur::noLF);
	}

	void File::reset()
	{
		seek(0);
	}

	void File::seek(uint64_t pos)
	{
		//if (pos == File::posHead())
		//	return;
		_flags &= ~peeked & ~eof;
		_buf.cur.restoreCRLF();
		_buf.save.restoreCRLF();
		_buf.clearSaveBuffer();
		if (pos >= _bufferStartPos && pos < _bufferStartPos + _buf.size && pos < _totalReaded) {
			_buf.cur.head = _buf.working + pos - _bufferStartPos;
			_buf.cur.line_tail = _buf.cur.head;
			_buf.cur.line_head = _buf.cur.head;
			_buf.keep_at_once = nullptr;
			return;
		}

		int e = _fseeki64(_fs, pos, SEEK_SET);
		if (e < 0) return;
		_buf.reset();
		_totalReaded = pos;
		_bufferStartPos = pos;
		return;
		//resizeBuffer(0x10000, true);
		_flags = pos >= size() ? eof : 0;
		_totalReaded = pos;
		_bufferStartPos = pos;
		*_buf.working = 0;
		_buf.cur.line_tail = _buf.working;
		_buf.cur.line_head = _buf.working;
		_buf.cur.head = _buf.working;
		_buf.keep_at_once = nullptr;
	}

	void File::seekEnd()
	{
		seek(size());
	}

	void File::keepBufferAtOnce(bool enable)
	{
		if (enable)
			_buf.keep_at_once = _buf.cur.line_head
			+ (_buf.cur.lastCR != &Buffer::Cur::noLF)
			+ (_buf.cur.lastLF != &Buffer::Cur::noLF);
		else
			_buf.keep_at_once = nullptr;
		assert(_buf.keep_at_once == nullptr || _buf.keep_at_once >= _buf.working);
	}

	void File::keepBufferAtOnce(char *ptr)
	{
		_buf.keep_at_once = ptr;
		assert(_buf.keep_at_once == nullptr || _buf.keep_at_once >= _buf.working);
	}

	char *File::getBufferAtOnce()
	{
		// when last line was just peek then this line may not affect the keep_at_once buffer
		// for all buf pos > working there is a 0 termination setted but this cannot be setted at the begining it would override chars of the line
		if (false && _bufferStartPos == 0 && _flags & peeked && _buf.working == _buf.cur.line_tail)
			return "";
		return _buf.keep_at_once;
	}

	void File::resetBuffer()
	{
		
		if(_buf.allocated)
			delete[] _buf.allocated;
		_buf.allocated = new char[RSIZE(_buf.init_size)];
		_buf.size = _buf.init_size;
		_buf.working = _buf.allocated;
		_buf.cur.line_head = _buf.allocated;
		_buf.cur.line_tail = _buf.allocated;
		_buf.cur.head = _buf.allocated;
		_buf.keep_at_once = nullptr;
	}

	void File::resizeBuffer(uint32_t size, bool append)
	{
		_buf.resize(size, append);
		return;
		if (size > _buf.max_size) {
			printf("");
			getchar();
			return;
		}
		char *newBuffer = _buf.allocated;
		if (size == _buf.size)
			return;
		newBuffer = new char[RSIZE(size)];
		memset(newBuffer, 0, RSIZE(size));
		uint32_t offset = 0;
		if (_buf.size) {
			if (size < _buf.size) {
				memmove(WBUF(newBuffer), _buf.working, size);
			}
			else if (append) {
				memmove(WBUF(newBuffer), _buf.working, _buf.size);

			}
			else {
				offset = size - _buf.size;
				memmove(WBUF(newBuffer) + offset, _buf.working, _buf.size);
			}
		}
		_buf.cur.head = WBUF(newBuffer) + offset + (_buf.cur.head - _buf.working);
		_buf.cur.line_tail = WBUF(newBuffer) + offset + (_buf.cur.line_tail - _buf.working);
		_buf.cur.line_head = WBUF(newBuffer) + offset + (_buf.cur.line_head - _buf.working);
		if (_buf.cur.lastLF != &Buffer::Cur::noLF) {
			_buf.cur.lastLF = WBUF(newBuffer) + offset + (_buf.cur.lastLF - _buf.working);
		}
		if (_buf.cur.lastCR != &Buffer::Cur::noLF) {
			_buf.cur.lastCR = WBUF(newBuffer) + offset + (_buf.cur.lastCR - _buf.working);
		}
		if (_buf.keep_at_once)
			_buf.keep_at_once = WBUF(newBuffer) + offset + (_buf.keep_at_once - _buf.working);
		if (_flags & (peeked | prepare_peek)) {
			_buf.save.head = WBUF(newBuffer) + offset + (_buf.save.head - _buf.working);
			_buf.save.line_tail = WBUF(newBuffer) + offset + (_buf.save.line_tail - _buf.working);
			_buf.save.line_head = WBUF(newBuffer) + offset + (_buf.save.line_head - _buf.working);
			if (_buf.save.lastLF != &Buffer::Cur::noLF) {
				_buf.save.lastLF = WBUF(newBuffer) + offset + (_buf.save.lastLF - _buf.working);
			}
			if (_buf.save.lastCR != &Buffer::Cur::noLF) {
				_buf.save.lastCR = WBUF(newBuffer) + offset + (_buf.save.lastCR - _buf.working);
			}
		}
		_buf.working = WBUF(newBuffer);
		_buf.size = size;
		if (_buf.allocated)
			delete[] _buf.allocated;
		_buf.allocated = newBuffer;
	}

	uint32_t File::readBuffer(bool backward)
	{
		log_trace(4) << "readBuffer" << backward;
		if (backward == false) {
			char *moveFrom;
			bool bResize = false;
			while (true) {
				if (_flags & (peeked | prepare_peek))
					moveFrom = _buf.keep_at_once && _buf.keep_at_once < _buf.save.line_tail
					? _buf.keep_at_once
					: _buf.save.line_tail;					
				else
					moveFrom = _buf.keep_at_once ? _buf.keep_at_once : _buf.cur.line_tail;
				printf("movefrom length=%d flags=%d buf.size=%d\n", strlen(moveFrom), _flags, _buf.size);
				if (_buf.size == _buf.cur.line_head - moveFrom) {
					resizeBuffer(_buf.size * 2, true);
					bResize = true;
					continue;
				}
				break;
			};
			uint32_t bytesToMove = moveFrom != _buf.working ? _buf.cur.line_head - moveFrom : 0;
			uint32_t moveOffset = moveFrom - _buf.working;
			uint32_t seekOffset = (_buf.cur.line_head - moveFrom);
			uint32_t sizeRead = _buf.size - seekOffset;

			if (bResize == false) {

				_bufferStartPos += moveOffset;
				if (_buf.save.line_tail) {
					_buf.save.line_tail -= moveOffset;
					_buf.save.line_head -= moveOffset;
				}
				if (_buf.keep_at_once)
					_buf.keep_at_once -= moveOffset;
				_buf.cur.line_tail -= moveOffset;
				if (_buf.cur.lastCR != &Buffer::Cur::noLF)
					_buf.cur.lastCR -= moveOffset;
				if (_buf.cur.lastLF != &Buffer::Cur::noLF)
					_buf.cur.lastLF -= moveOffset;
				if (_buf.save.lastCR != &Buffer::Cur::noLF)
					_buf.save.lastCR -= moveOffset;
				if (_buf.save.lastLF != &Buffer::Cur::noLF)
					_buf.save.lastLF -= moveOffset;

				_buf.cur.line_head = _buf.working + seekOffset;
				memmove(_buf.working, moveFrom, bytesToMove);
				*_buf.cur.line_head = 0;
			}

			_flags &= ~eof;
			uint64_t readed = 0;
			if (_fseeki64(_fs, _bufferStartPos + seekOffset, SEEK_SET) == 0)
				readed = fread(_buf.cur.line_head, 1, sizeRead, _fs);
			if (readed && *_buf.cur.line_head == 0) {
				throw std::exception(
					(boost::format("fatal: in File::readBuffer file %1% has \0 signs") % _fileName).str().c_str());
			}
			if (readed == 0 && sizeRead)
				_flags |= eof;
			_buf.cur.line_head[readed] = 0;

			_totalReaded += readed;
			log_trace(4) << "readBuffer done" << readed;
			assert(_buf.keep_at_once == nullptr || _buf.keep_at_once >= _buf.working);
			return readed;

		}
		else {
			// keeping buffer will be shifted to the end and new data inserted before

			char *moveFrom = nullptr;
			uint32_t bytesToMove = 0;
			bool bResize = false;
			// keep_head holds the buffer start that should be keeped.
			// bytes before that head can be removed
			char *keep_head
				= (_buf.keep_at_once > _buf.cur.line_head 
				? _buf.keep_at_once 
				: _buf.cur.line_head) + 2;
			while (true) {
				if (_flags & (peeked | prepare_peek))
					moveFrom = _buf.keep_at_once && _buf.keep_at_once < _buf.save.line_tail
					? _buf.keep_at_once
					: _buf.save.line_tail;
				else
					moveFrom = _buf.keep_at_once && _buf.keep_at_once < _buf.cur.line_tail
					? _buf.keep_at_once
					: _buf.cur.line_tail;

				bytesToMove
					= _buf.keep_at_once > _buf.cur.line_head 
					? _buf.keep_at_once + 2 - moveFrom
					: _buf.cur.line_head + 2 - moveFrom;

				if (_buf.size == bytesToMove) {
					resizeBuffer(_buf.size * 2, true);
					bResize = true;
					continue;
				}
				break;
			};

			//uint32_t bytesToMove = keep_head - moveFrom;

			// new bytes to read and insert at beginning
			uint32_t sizeRead = _buf.size - bytesToMove;
			// startFIlePos will change 
			// as long as startFIlePos > sizeRead we will decremnt startFIlePos
			if (_bufferStartPos > sizeRead)
				_bufferStartPos -= (_buf.size - (keep_head - _buf.working)); //sizeRead;
			else {
				// when startFilePos smaller than calc read size then read just the gap from begin to 
				// startFIlePos and set startFIlePos to 0
				sizeRead = _bufferStartPos;
				_bufferStartPos = 0;
			}

			// move the keeping buffer to readSize position to make space for reading
			uint32_t bufferShift = sizeRead; 
			_buf.cur.line_head += bufferShift;
			_buf.cur.line_tail += bufferShift;

			memmove(_buf.working + bufferShift, moveFrom, bytesToMove);
			uint64_t readed = 0;
			if (_fseeki64(_fs, _bufferStartPos, SEEK_SET) == 0) {
				_totalReaded = _bufferStartPos;
				readed = fread(_buf.working, 1, sizeRead, _fs);
			}
			_buf.working[readed + bytesToMove] = 0;
			_totalReaded += readed;
			log_trace(2) << "readBuffer done" << readed;
			// when no more is readed then seek explicit to start to reset tail/head buffers
			if (readed == 0)
				seek(0);
			assert(_buf.keep_at_once == nullptr || _buf.keep_at_once >= _buf.working);
			return readed;
		}
		return 0;
	}

	char *File::readPrevLine()
	{
		assert(_buf.keep_at_once == nullptr || _buf.keep_at_once >= _buf.working);
		if (_flags & peeked) {
			_flags = _flags_save;
			_buf.clearSaveBuffer();
		}
		if (hasPrev() == false) {
			_buf.cur.line_head = _buf.cur.line_tail;
			return "";
		}
		_buf.cur.restoreCRLF();
		bool first = true;
		/*
		v=_buf.cur.line_tail
		b=_buf.working
		b            v
		\r\nLine0\r\nLine1 then it have to return Line 0 no buffer eload needed
		v
		Line1 it have to read Line0 buffer reload needed
		v
		Line1
		b             v>
		\r\nLine0\r\nLine1 then it have to return Line 1
		*/
		enum ReturnType { Undefined = -1, CurrentLine = 0, PrevLine = 1 };
		ReturnType returnCurrentLine = Undefined;
		printbuf("readPrevLine");

		if (_buf.cur.line_tail > _buf.working)
			returnCurrentLine = *(_buf.cur.line_tail - 1) == '\n' ? PrevLine : CurrentLine;

	__L001:
		int lineBreakSize = (*(_buf.cur.line_tail - 1) == '\n') + (*(_buf.cur.line_tail - 2) == '\r');
		// the new line has a lineHead when there is a break before  the currentLine 
		// or current filepos points to the end
		bool hasLineHead = lineBreakSize > 0 || posTail() == size();
		_buf.cur.line_tail -= (_flags & eof) ? 0 : lineBreakSize;
		_buf.cur.line_head = _buf.cur.line_tail;
		assert(_buf.keep_at_once == nullptr || _buf.keep_at_once >= _buf.working);
		while (true) {
			printbuf("readPrevLine while(true)");
			if (_buf.cur.line_tail > _buf.working) {
				while (_buf.cur.line_tail > _buf.working && *--_buf.cur.line_tail != '\n');
				if (*(_buf.cur.line_tail) == '\n') {
					_buf.cur.line_tail++; // seek to linestart
					if (hasLineHead) {
						_buf.cur.clearCRLF(_buf.cur.line_head + lineBreakSize - 1);
						_buf.cur.line_tail = _buf.cur.line_tail;
						_flags &= ~eof;
						return _buf.cur.line_tail;
					}
					break;
				}
			}
			// start of file reached
			if (posTail() == 0) {
				break;
			}
			printbuf("readPrevLine readBuffer");
			uint32_t readed = readBuffer(true);
			if (readed <= 0)
				break;
			if (returnCurrentLine == Undefined)
				goto __L001;
		}
		assert(_buf.keep_at_once == nullptr || _buf.keep_at_once >= _buf.working);
		// save the offset not the pointer because buffers can change due readBuffer called in readLine
		uint32_t bufAtOnceOffset = _buf.keep_at_once == nullptr ? -1 : _buf.keep_at_once - _buf.working;
		auto save_buf_keep_at_once = _buf.keep_at_once;
		auto save_buf_working = _buf.working;
		_buf.keep_at_once = _buf.working + ((_buf.cur.line_tail - _buf.working) / 2);
		_buf.cur.line_head = _buf.cur.line_tail;
		char *line = readLine();		
		_buf.keep_at_once = save_buf_keep_at_once;
		//assert(save_buf_working == _buf.working);
		_buf.keep_at_once = bufAtOnceOffset == -1 ? nullptr : _buf.working + bufAtOnceOffset;
		
		_flags &= ~eof;
		
		return line;
	}

	char * File::peekLine()
	{
		if (_flags & peeked)
			return _buf.save.line_tail;

		// save current status for later restore
		_flags_save = _flags;
		_buf.save = _buf.cur;

		// set to prepare for the follwong readLine
		_flags |= prepare_peek;
		char * line = readLine();

		// set termination to the end of the previous line
		_buf.save.clearCRLF(_buf.cur.line_tail - 1);

		// in curbuf are the correct values as after an normal readLine
		// swap this with save witch holds the state before the above read
		// when the next readLine will be made this bufferes are swaped again 
		_buf.cur.swap(_buf.save);
		_flags = _flags & ~prepare_peek | peeked;
		return line;
	}

	char *File::readLine()
	{
		assert(_buf.keep_at_once == nullptr || _buf.keep_at_once >= _buf.working);
		if (_flags & peeked) {
			_buf.cur.swap(_buf.save);
			_buf.clearSaveBuffer();
			_flags = _flags_save;
			return _buf.cur.line_tail;
		}
		int restored = _buf.cur.restoreCRLF();
		//bool hasLineHead = *(_buf.cur.line_head + restored - 1) == '\n';
		// When the current line has a correct head then the new head will start after the current line
		if (*(_buf.cur.line_head + restored - 1) == '\n')
			_buf.cur.line_head = _buf.cur.line_head + restored;
		else while(hasPrev()) {
			while (_buf.cur.line_tail > _buf.working && *--_buf.cur.line_tail != '\n');
			if (*(_buf.cur.line_tail) == '\n') {
				_buf.cur.line_tail++;
				break;
			}
			uint32_t readed = readBuffer(true);
			return readPrevLine();
		}
		_buf.cur.line_tail = _buf.cur.line_head;
		printbuf("readLine");
		while (true) {
			// check for > \n because most chars are true in this case
			// and that needs just one condition
			while (*_buf.cur.line_head > '\n' || *_buf.cur.line_head != '\n' && *_buf.cur.line_head) 
				_buf.cur.line_head++;

			if (*_buf.cur.line_head == '\n') {
				int cleared = _buf.cur.clearCRLF(_buf.cur.line_head);
				_buf.cur.line_head -= (cleared - 1);
				return _buf.cur.line_tail;
			}
			else {
				printbuf("readLine readBuffer");
				uint32_t readed = readBuffer();
				if (readed <= 0)
					break;
			}
		}
		_buf.cur.line_head = _buf.cur.line_head;
		return _buf.cur.line_tail;
	}

	char *strrstr(char* first, char *last, const char *needle)
	{
		char *found = nullptr;
		char chSave;
		// sets 0 termination temporary
		char *term = (char *)last; chSave = *term; *term = 0;
		uint32_t count = last - first;
		uint32_t step;
		char *p;
		while ((last - first) / 2 > 0) {
			auto from = first;
			//count = last - first;
			step = (last - first) / 2;
			std::advance(from, step);
			if (p = strstr(from, needle)) {                 // or: if (comp(*it,val)), for version (2)
				found = p;
				//step += found - from;
				first = found;
				//count -= step + 1;
			}
			else {
				last = from;
				//count = step;
			}
		}
		*term = chSave;
		return found;
	}

	char *File::readPrevLine(const char *condition)
	{
		assert(_buf.keep_at_once == nullptr || _buf.keep_at_once >= _buf.working);
		if (_flags & peeked) {
			_flags = _flags_save;
			_buf.clearSaveBuffer();
		}
		uint16_t conditionLength = strlen(condition);
		if (_buf.size < conditionLength) {
			resizeBuffer(conditionLength * 4);
		}

		_buf.keep_at_once = nullptr;
		while (true) {
			_buf.cur.restoreCRLF();
			const char *found = strrstr(_buf.working, _buf.cur.line_tail, condition);
			if (found) {
				//_buf.cur.restoreCRLF();
				_buf.cur.line_tail = _buf.cur.line_head = (char *)found;
				//uint64_t foundPos = posTail();
				char *line;
				// when stays at linestart then just readLine otherwise readPrevLine
				if (*(_buf.cur.line_tail - 1) == '\n')
					line = readLine();
				else
					line = readPrevLine();
				//uint32_t posInLine = foundPos - posTail();
				return line;
			}
			_buf.cur.line_tail = _buf.working + conditionLength;
			_buf.cur.line_head = _buf.cur.line_tail;
			if (_buf.cur.line_head != _buf.cur.line_tail)
				_buf.cur.line_head = _buf.cur.line_tail + conditionLength;
			uint32_t readed = readBuffer(true);
			if (readed <= 0)
				return "";
			continue;
		}
	}

	char *File::readLine(const char *condition)
	{
		if (_flags & peeked) {
			_flags = _flags_save;
			_buf.clearSaveBuffer();
		}
		_buf.cur.restoreCRLF();
		uint16_t conditionLength = strlen(condition);
		if (_buf.size < conditionLength) {
			resizeBuffer(conditionLength * 4);
		}
		_buf.keep_at_once = nullptr;
		while (true) {
			char *found = strstr(_buf.cur.line_head, condition);
			if (found) {
				_buf.cur.line_tail = _buf.cur.line_head = found;
				//uint64_t foundPos = posTail();
				char *line;
				if (*(_buf.cur.line_tail - 1) == '\n')
					line = readLine();
				else
					line = readPrevLine();
				return line;
			}
			if (_totalReaded && *_buf.cur.line_head) {
				_buf.cur.line_head = _buf.working + _buf.size;
				while (_buf.cur.line_head > _buf.working && *(_buf.cur.line_head - 1) != '\n')
					_buf.cur.line_head--;
				_buf.cur.line_tail = _buf.cur.line_head;
				//_buf.keep_at_once = _buf.cur.line_head;

			}
			uint32_t readed = readBuffer();
			_buf.cur.line_head = _buf.working;
			if (readed <= 0)
				return "";
			continue;
		}
	}

	char *File::getCurrentLine()
	{
		return _buf.cur.line_tail;
	}


	void File::Buffer::reset()
	{
		if (allocated)
			delete[] allocated;
		allocated = new char[RSIZE(init_size)];
		size = init_size;
		cur.reset();
		save.reset();
		keep_at_once = nullptr;
		cur.head = cur.line_head = cur.line_tail = working = allocated;
		*working = 0;
	}

	void File::Buffer::resize(uint32_t newSize, bool append)
	{
		if (newSize > max_size) {
			printf("");
			getchar();
			return;
		}
		char *newBuffer = allocated;
		if (newSize == size)
			return;
		newBuffer = new char[RSIZE(newSize)];
		memset(newBuffer, 0, RSIZE(newSize));
		uint32_t offset = 0;
		if (size) {
			if (newSize < size) {
				memmove(WBUF(newBuffer), working, newSize);
			}
			else if (append) {
				memmove(WBUF(newBuffer), working, size);

			}
			else {
				offset = newSize - size;
				memmove(WBUF(newBuffer) + offset, working, size);
			}
		}
		cur.head = WBUF(newBuffer) + offset + (cur.head - working);
		cur.line_tail = WBUF(newBuffer) + offset + (cur.line_tail - working);
		cur.line_head = WBUF(newBuffer) + offset + (cur.line_head - working);
		if (cur.lastLF != &Buffer::Cur::noLF) {
			cur.lastLF = WBUF(newBuffer) + offset + (cur.lastLF - working);
		}
		if (cur.lastCR != &Buffer::Cur::noLF) {
			cur.lastCR = WBUF(newBuffer) + offset + (cur.lastCR - working);
		}
		if (keep_at_once)
			keep_at_once = WBUF(newBuffer) + offset + (keep_at_once - working);

		if (save.head) {
			save.head = WBUF(newBuffer) + offset + (save.head - working);
			save.line_tail = WBUF(newBuffer) + offset + (save.line_tail - working);
			save.line_head = WBUF(newBuffer) + offset + (save.line_head - working);
			if (save.lastLF != &Buffer::Cur::noLF) {
				save.lastLF = WBUF(newBuffer) + offset + (save.lastLF - working);
			}
			if (save.lastCR != &Buffer::Cur::noLF) {
				save.lastCR = WBUF(newBuffer) + offset + (save.lastCR - working);
			}
		}
		working = WBUF(newBuffer);
		size = newSize;
		if (allocated)
			delete[] allocated;
		allocated = newBuffer;
	}
}