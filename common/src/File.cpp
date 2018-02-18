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
  _buf.posInFile -= (_buf.size - (keep_head - _buf.working))


*/

#define printbufx(msg) printf("%s bsize=%d line_tail=%d line_head=%d\n", \
		msg, _buf.size,\
		_buf.line_tail - _buf.working, \
		_buf.line_head - _buf.working)
#define printbuf(msg)
namespace common {
	char File::Buffer::noLF;

	File::File()
	{
		_buf.resize(0x10000, true);
	}

	File::~File()
	{
		if (_fs)
			fclose(_fs);
		if (_isClone == false)
			delete[] _buf.allocated;
	}

	File& File::operator = (const File& other)
	{
		_fileName = other._fileName;
		_buf = other._buf;
		_flags = other._flags;
		_flags_save = other._flags_save;
		//_totalReaded = other._totalReaded;
		_isClone = true;
		return *this;
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
		return _buf.posInFile + (_buf.line_tail - _buf.working);
	}

	uint64_t File::posHead() const
	{
		return _buf.posInFile
			+ (_buf.line_head - _buf.working)
			+ (_buf.lastCR != &Buffer::noLF)
			+ (_buf.lastLF != &Buffer::noLF);
	}

	void File::reset()
	{
		seek(0);
	}

	void File::seek(uint64_t pos)
	{
		_flags = _flags & ~peeked & ~eof;
		_buf.restoreCRLF();
		_buf.save.reset();
		if (pos >= _buf.posInFile && pos < _buf.posInFile + _buf.size && pos < _buf.totalReaded) {
			_buf.head = _buf.working + pos - _buf.posInFile;
			_buf.posInBuffer = pos - _buf.posInFile;
			_buf.line_tail = _buf.head;
			_buf.line_head = _buf.head;
			_buf.keep_at_once_tail = nullptr;
			log_trace(0) << sfmt("seek in buffer pos=%1%", pos);
			return;
		}

		int e = _fseeki64(_fs, pos, SEEK_SET);
		if (e < 0) return;
		_flags |= size() <= pos ? eof : none;
		_buf.reset();
		_buf.posInFile = pos;
		_buf.totalReaded = pos;
		log_trace(0) << sfmt("seek in file pos=%1%", pos);
	}

	void File::seekEnd()
	{
		seek(size());
	}

	void File::keepBufferAtOnce(bool enable)
	{
		if (enable) {
			_buf.keep_at_once_tail = _buf.line_head
				+ (_buf.lastCR != &Buffer::noLF)
				+ (_buf.lastLF != &Buffer::noLF);
		}
		else
			_buf.keep_at_once_tail = nullptr;
		assert(_buf.keep_at_once_tail == nullptr || _buf.keep_at_once_tail >= _buf.working);
	}

	void File::keepBufferAtOnce(char *ptr)
	{
		_buf.keep_at_once_tail = ptr;
		assert(_buf.keep_at_once_tail == nullptr || _buf.keep_at_once_tail >= _buf.working);
	}

	char *File::getBufferAtOnce()
	{
		// when last line was just peek then this line may not affect the keep_at_once_tail buffer
		// for all buf pos > working there is a 0 termination setted but this cannot be setted at the begining it would override chars of the line
		if (false && _buf.posInFile == 0 && _flags & peeked && _buf.working == _buf.line_tail)
			return "";
		return _buf.keep_at_once_tail;
	}

	uint32_t File::readBuffer(ReadSize readsize)
	{
		char *moveBegin = 
				_buf.keep_at_once_tail && _buf.keep_at_once_tail < _buf.line_tail
				? _buf.keep_at_once_tail
				: _buf.line_tail;
		if (_buf.save.line_tail && _buf.save.line_tail < moveBegin)
			moveBegin = _buf.save.line_tail;

		char *moveEnd
			= _buf.keep_at_once_head > _buf.line_head
			? _buf.keep_at_once_head + 2 
			: _buf.line_head + 2;
		if (_buf.save.line_head && _buf.save.line_head > moveEnd)
			moveEnd = _buf.save.line_head;

		if (!readsize.is_max()) {
			if (readsize.is_backward()) {
				auto endofBuf = _buf.working + _buf.size;
				if (moveEnd <= endofBuf + readsize._size)
					moveEnd = endofBuf + readsize._size;
				//moveEnd = moveEnd - readsize._size;
			} 
			else {
				if (moveBegin >= _buf.working + readsize._size)
					moveBegin = _buf.working + readsize._size;
			}
		}
		uint32_t bytesToMove = moveEnd - moveBegin;
		//bytesToMove = std::min(_buf.readed(), bytesToMove);
		if (bytesToMove >= _buf.size) {
			if(_buf.resize(_buf.size + _buf.init_size) == false)
				return 0;
			return readBuffer(readsize);
		}

		if (readsize.is_forward()) {
			int32_t moveOffset = _buf.working - moveBegin;
			uint32_t seekOffset = (_buf.line_head - moveBegin);
			uint32_t sizeRead = _buf.size - seekOffset;
			
			if (moveOffset != 0) {
				_buf.shift(moveOffset);
				memmove(_buf.working, moveBegin, bytesToMove);
				*_buf.line_head = 0;
			}
			_buf.posInFile -= moveOffset;
			log_trace(2) << sfmt("%1% bytes moved by offset << %2% seek to %3% read %4% bytes", bytesToMove, moveOffset, _buf.posInFile + seekOffset, sizeRead);

			_flags &= ~eof;
			size_t readed = 0;
			if (_fseeki64(_fs, _buf.posInFile + seekOffset, SEEK_SET) == 0)
				readed = fread(_buf.line_head, 1, sizeRead, _fs);
			if (readed && *_buf.line_head == 0) {
				throw std::exception(
					(boost::format("fatal: in File::readBuffer file %1% has \0 signs") % _fileName).str().c_str());
			}
			if (readed == 0 && sizeRead)
				_flags |= eof;
			_buf.line_head[readed] = 0;

			_buf.totalReaded += readed;
			log_trace(4) << sfmt("%1% bytes readed", readed);
			assert(_buf.keep_at_once_tail == nullptr || _buf.keep_at_once_tail >= _buf.working);
			return readed;

		}
		else if(readsize.is_backward()) {
			// new bytes to read and insert at beginning
			int32_t moveOffset = 0;
			uint32_t sizeRead = _buf.size;

			moveOffset = _buf.size - bytesToMove;
			if (readsize.is_max() == false) {
				moveOffset = abs(readsize._size);
				sizeRead = moveOffset;
			}
			else
				sizeRead -= bytesToMove;
			if (_buf.posInFile > sizeRead)
				_buf.posInFile -= sizeRead;
			else {
				sizeRead = static_cast<uint32_t>(_buf.posInFile);
				moveOffset = sizeRead;
				_buf.posInFile = 0;
			}

			_buf.shift(moveOffset);
			memmove(_buf.working + moveOffset, moveBegin, bytesToMove);
			log_trace(2) << sfmt("%1% bytes moved by offset >> %2% seek to %3% read %4% bytes", bytesToMove, moveOffset, _buf.posInFile, sizeRead);
			
			size_t readed = 0;
			//_buf.posInFile += readsize.is_max() ? 0 : readsize._size;
			if (_fseeki64(_fs, _buf.posInFile, SEEK_SET) == 0) {
				//_buf.totalReaded = _buf.posInFile;
				readed = fread(_buf.working, 1, sizeRead, _fs);
			}
			_buf.working[readed + bytesToMove] = 0;
			_buf.totalReaded -= readed;
			log_trace(2) << sfmt("%1% bytes readed", readed);
			// when no more is readed then seek explicit to start to reset tail/head buffers
			//if (readed == 0)
//				seek(0);
			assert(_buf.keep_at_once_tail == nullptr || _buf.keep_at_once_tail >= _buf.working);
			return readed;
		}
		else
			log_warning() << sfmt("readsize not forward nor backward");
		return 0;
	}

	char *File::readPrevLine()
	{
		assert(_buf.keep_at_once_tail == nullptr || _buf.keep_at_once_tail >= _buf.working);
		if (_flags & peeked) {
			_flags = _flags_save;
			_buf.save.reset();
		}
		if (hasPrev() == false) {
			_buf.line_head = _buf.line_tail;
			return "";
		}
		_buf.restoreCRLF();

		_buf.line_head = _buf.line_tail;

		auto neededToTail = readBackwardToLineTail();
		// when p is on linestart then we read the previous line
		if (neededToTail == 0) {
			_buf.line_tail -= _buf.getPrevLineBreakSize();
			_buf.line_head = _buf.line_tail;
			readBackwardToLineTail();
		}
		auto neededToHead = readForwardToLineHead(ReadSize(100));
		return _buf.line_tail;
	}

	char * File::peekLine()
	{
		if (_flags & peeked)
			return _buf.save.line_tail;

		// save current status for later restore
		//_flags_save = _flags;
		_buf.save = _buf;

		// set to prepare for the follwong readLine
		_flags |= prepare_peek;
		char * line = readLine();

		// set termination to the end of the previous line
		//_buf.save.clearCRLF(_buf.line_tail - 1);

		// in curbuf are the correct values as after an normal readLine
		// swap this with save witch holds the state before the above read
		// when the next readLine will be made this bufferes are swaped again 
		_buf.swap(_buf.save);
		_flags_save = _flags & eof ? eof : none;
		_flags = peeked;
		
		
		//std::swap(_flags_save, _flags);
		
		return line;
	}

	char *File::readLine()
	{
		assert(_buf.keep_at_once_tail == nullptr || _buf.keep_at_once_tail >= _buf.working);
		if (_flags & peeked) {
			_buf.swap(_buf.save);
			_buf.save.reset();
			_flags = _flags_save;
			return _buf.line_tail;
		}
		int restored = _buf.restoreCRLF();
		_buf.line_head += restored;
		_buf.line_tail = _buf.line_head;
		auto neededToHead = readForwardToLineHead();
		// when neededToHead = 0 then we are on lineend so we read the next line
		if (neededToHead == 0) {
			_buf.line_head += _buf.getNextLineBreakSize();
			_buf.line_tail = _buf.line_head;
			readForwardToLineHead();
		}
		if (restored)
			return _buf.line_tail;
		readBackwardToLineTail(ReadSize(-100));
		return _buf.line_tail;
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
		assert(_buf.keep_at_once_tail == nullptr || _buf.keep_at_once_tail >= _buf.working);
		if (_flags & peeked) {
			_flags = _flags_save;
			_buf.save.reset();
		}
		size_t conditionLength = strlen(condition);
		if (_buf.size < conditionLength) {
			_buf.resize(conditionLength * 4);
		}

		_buf.keep_at_once_tail = nullptr;
		while (true) {
			_buf.restoreCRLF();
			const char *found = strrstr(_buf.working, _buf.line_tail, condition);
			if (found) {
				//_buf.restoreCRLF();
				_buf.line_tail = _buf.line_head = (char *)found;
				//uint64_t foundPos = posTail();
				char *line;
				// when stays at linestart then just readLine otherwise readPrevLine
				if (*(_buf.line_tail - 1) == '\n')
					line = readLine();
				else
					line = readPrevLine();
				//uint32_t posInLine = foundPos - posTail();
				return line;
			}
			_buf.line_tail = _buf.working + conditionLength;
			_buf.line_head = _buf.line_tail;
			if (_buf.line_head != _buf.line_tail)
				_buf.line_head = _buf.line_tail + conditionLength;
			uint32_t readed = readBuffer(ReadSize::max_forward());
			if (readed <= 0)
				return "";
			continue;
		}
	}

	char *File::readLine(const char *condition)
	{
		if (_flags & peeked) {
			_flags = _flags_save;
			_buf.save.reset();
		}
		_buf.restoreCRLF();
		size_t conditionLength = strlen(condition);
		if (_buf.size < conditionLength) {
			_buf.resize(conditionLength * 4);
		}
		_buf.keep_at_once_tail = nullptr;
		while (true) {
			char *found = strstr(_buf.line_head, condition);
			if (found) {
				_buf.line_tail = _buf.line_head = found;
				return readLine();
			}
			if (_buf.totalReaded && *_buf.line_head) {
				_buf.line_head = _buf.working + _buf.size;
				while (_buf.line_head > _buf.working && *(_buf.line_head - 1) != '\n')
					_buf.line_head--;
				_buf.line_tail = _buf.line_head;
				//_buf.keep_at_once_tail = _buf.line_head;

			}
			uint32_t readed = readBuffer();
			_buf.line_head = _buf.working;
			if (readed <= 0)
				return "";
			continue;
		}
	}

	char *File::getCurrentLine()
	{
		return _buf.line_tail;
	}

	File::Buffer::Buffer()
	{

	}

	void File::Buffer::reset()
	{
		if (allocated)
			delete[] allocated;
		allocated = new char[RSIZE(init_size)];
		size = init_size;
		BufferBase::reset();
		save.reset();
		keep_at_once_head = nullptr;
		keep_at_once_tail = nullptr;
		head = line_head = line_tail = working = allocated;
		posInBuffer = 0;
		*working = 0;
		totalReaded = posInFile;
	}

	bool File::Buffer::resize(uint32_t newSize, bool append)
	{
		log_debug() << sfmt("resize from %1% to %2%", size, newSize);
		if (newSize > max_size) {
			log_error() << sfmt("newsize exeeds max_size");
			return false;
		}
		
		if (newSize == size)
			return false;
		char *newBuffer = new char[RSIZE(newSize)];
		memset(newBuffer, 0, RSIZE(newSize));
		int32_t offset = 0;
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
		
		offset += (WBUF(newBuffer) - working);
		shift(offset);
		working += offset;

		size = newSize;
		if (allocated)
			delete[] allocated;
		allocated = newBuffer;
		return true;
	}

	void File::setBufSize(size_t size)
	{
		_buf.init_size = size;
		_buf.resize(size, true);
	}
}