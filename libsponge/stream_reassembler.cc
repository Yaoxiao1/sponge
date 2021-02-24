#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {
    _first_unacceptable = _first_unread + _capacity;
    _buffer.assign(_capacity, 0);
    _hasData.assign(_capacity, 0);
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //step1 check bytestream with total_write
    if (_output.bytes_read() > _first_unread){
        size_t last_read = _first_unassembled - _first_unread;
        _bytes_in_buffer -= _first_unassembled - _first_unread;
        _first_unread = _first_unassembled;
        _first_unacceptable = _first_unread + _capacity;
        for (size_t i=0; i<last_read; ++i){
            _buffer.pop_front();
            _buffer.push_back(0);
            _hasData.pop_front();
            _hasData.push_back(0);
        }
    }
    //step2 merge data
    size_t data_sz = data.length();
    size_t data_len = min(index+data_sz, _first_unacceptable) - max(index, _first_unread);
    if (index+data_sz < _first_unread) data_len = 0;
    size_t buffer_pos = max(index, _first_unread) - _first_unread;
    size_t data_pos = max(index, _first_unread) - index;
    size_t i = 0;
    for (; i<data_len; ++i){
        if (_hasData[buffer_pos+i]) continue;
        _buffer[buffer_pos+i] = data[data_pos+i];
        _hasData[buffer_pos+i] = 1;
        _bytes_in_buffer += 1;
    }

    //step3 check first_unassembled and update to bytestream
    if (_hasData[_first_unassembled - _first_unread]){
        // size_t temp = _first_unassembled;
        string tempstr = "";
        while (_first_unassembled<_first_unacceptable && _hasData[_first_unassembled - _first_unread] != 0){
            tempstr.push_back(_buffer[_first_unassembled - _first_unread]);
            // _buffer[_first_unassembled - _first_unread] = 0;
            _first_unassembled += 1;
        }
        _output.write(tempstr);
    }
    //step4 check eof
    if (eof){
        _eof = true;
        _end_idx = index + data_sz;
    }
    if (_eof && _end_idx==_first_unassembled){
        _output.end_input();
    }
    return;
}

size_t StreamReassembler::unassembled_bytes() const { 
    return _bytes_in_buffer - (_first_unassembled - _first_unread);
}

bool StreamReassembler::empty() const { 
    return unassembled_bytes() == 0;
}
