#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader t_header = seg.header();
    Buffer t_payload = seg.payload();
    size_t seg_len = seg.length_in_sequence_space();
    WrappingInt32 in_seq_no = t_header.seqno;
    string_view sv1 = t_payload.str();
    string data(sv1.begin(), sv1.end());
    uint64_t index;
    // if (t_header.fin) is_fin = true;
    // handle the first syn message
    if (!is_syn && t_header.syn){
        if (t_header.fin) is_fin = true;
        is_syn = true;
        _isn = in_seq_no;
        _last_reassembled = in_seq_no.raw_value();
        // index = unwrap(in_seq_no, _isn, _last_reassembled);
        index = 0;
        _reassembler.push_substring(data, index, is_fin);
        _ackno = _last_reassembled + seg_len;
        _last_reassembled = _reassembler.stream_out().bytes_written();
        return ;
    }

    // handle no syn
    if (!is_syn && !t_header.syn) return ;

    // stream index;
    size_t relative_seq_no = unwrap(in_seq_no, _isn, _last_reassembled);
    index = relative_seq_no - 1;
    if (!is_fin && t_header.fin){
        is_fin = true;
        _reassembler.push_substring(data, index, true);
    }
    else _reassembler.push_substring(data, index, false);
    _ackno = _reassembler.stream_out().bytes_written() + _isn.raw_value() + is_syn + (_reassembler.empty()? is_fin:0);
    _last_reassembled = _reassembler.stream_out().bytes_written();
    
    return; 
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (is_syn == false) return {};
    else {
        return WrappingInt32(_ackno);
    } 
}

size_t TCPReceiver::window_size() const {
    return (_reassembler.get_first_unacceptalbe()) - (_reassembler.get_first_unassembled());
}
