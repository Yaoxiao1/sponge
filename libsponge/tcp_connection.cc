#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {  
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const { 
    return _temp_time - _last_segment_recv_time;
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
    // if RST set , kill the connection
    if (seg.header().rst){
        // kill the connection permanently
    }
    // the receiver get the segment
    _receiver.segment_received(seg);
    // if ACK set, tells the TCPSender about the fields it cares about on incoming
    if (seg.header().ack){
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    // at least one segment is sent in reply, to reflect the updated seqno and winsize
    TCPSegment tseg;
    tseg.header().seqno = _sender.next_seqno();
    tseg.header().win = _receiver.window_size();
    _sender.send_segment(tseg);
}

bool TCPConnection::active() const { return {}; }

size_t TCPConnection::write(const string &data) {
    return _sender.stream_in().write(data);
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _temp_time += ms_since_last_tick;
    // tells the sender the passage of time
    _sender.tick(ms_since_last_tick);
    // if too many consecutive retransmissions, abort the connection and send a reset message to the peer
    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS){
        // send a reset message to peer
        TCPSegment tesg;
        tesg.header().seqno = _sender.next_seqno();
        tesg.header().rst = true;
        _sender.send_segment(tesg);
    }
    // end the connection cleanly
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
}

void TCPConnection::connect() {}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
