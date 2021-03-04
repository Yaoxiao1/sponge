#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <algorithm>
#include <iostream>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , _retransmit_timer(retx_timeout){
    }

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    // handle the first segment 
    // if (_window_size == 0 && _income_win_zero) _window_size = 1;
    if (_next_seqno==0){
        
        TCPSegment first_seg{};
        first_seg.header().syn = true;
        first_seg.header().seqno = _isn;
        _segments_out.push(first_seg);
        _outstanding_segs.push(first_seg);
        _bytes_in_flight += first_seg.length_in_sequence_space();
        _next_seqno += first_seg.length_in_sequence_space();
        _window_size -= 1;
    }
    else{
        if (_stream.eof() && !_fin_sent && _next_seqno < _right_limit){
            TCPSegment tseg;
            // if (_window_size==0 && _income_win_zero) _window_size = 1;
            tseg.header().seqno = wrap(_next_seqno, _isn);
            tseg.header().fin = true;
            send_segment(tseg);
            _fin_sent = true;
            return ;
        }
        while (!_stream.buffer_empty() && _next_seqno < _right_limit){
            // handle the window size
            // if (_window_size==0 && _income_win_zero) _window_size = 1;
            size_t bytes_to_read = min(_stream.buffer_size(), min(_window_size, TCPConfig::MAX_PAYLOAD_SIZE));
            string data = std::move(_stream.read(bytes_to_read));
            _window_size -= bytes_to_read;

            // handle the segment_to_sent
            TCPSegment segment_to_sent;
    
            segment_to_sent.header().seqno = wrap(_next_seqno, _isn);
            if (_stream.eof() && _window_size > 0) {
                segment_to_sent.header().fin = true;
                _fin_sent = true;
                _window_size -= 1;
            }
            
            
            // Buffer buf1(std::move(data));

            segment_to_sent.payload() = Buffer(std::move(data));
            
            _segments_out.push(segment_to_sent);
            _outstanding_segs.push(segment_to_sent);
            _bytes_in_flight += segment_to_sent.length_in_sequence_space();
            _next_seqno += segment_to_sent.length_in_sequence_space();
        }
        
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
        
        uint64_t checkpoint = _stream.bytes_read();
        uint64_t ack_seqno = unwrap(ackno, _isn, checkpoint);
        if (ack_seqno < _next_seqno && ack_seqno + window_size <= _right_limit) return ;
        if (ack_seqno > _right_limit) return;
        while (!_outstanding_segs.empty()){
            auto& ft_seg = _outstanding_segs.front();
            if (unwrap(ft_seg.header().seqno, _isn, checkpoint) + ft_seg.length_in_sequence_space() <= ack_seqno){
               
                size_t test = ft_seg.length_in_sequence_space();
                _bytes_in_flight -= test;
                _outstanding_segs.pop();
            }
            else break;
        }
        _window_size = (window_size==0)? 1: window_size;
        _right_limit = ack_seqno + _window_size;
        _income_win_zero = (window_size == 0);
        _cons_retrans = 0;
        if(!_income_win_zero) _retransmit_timer.reset_RTO();
        _retransmit_timer.reset();

        
        if (_stream.eof() && _fin_sent==false && _outstanding_segs.empty()){
            if (_window_size == 0 && _income_win_zero) _window_size = 1;
            TCPSegment tseg;
            tseg.header().seqno = wrap(_next_seqno, _isn);
            tseg.header().fin = true;
            send_segment(tseg);
            _fin_sent = true;
        }
        return ;
        
    }

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _retransmit_timer._ms_passed(ms_since_last_tick);
    if (_retransmit_timer.is_expired()){
        if (_outstanding_segs.empty()) return ;
        auto& rt_seg = _outstanding_segs.front();
        // _window_size += rt_seg.length_in_sequence_space();
        _segments_out.push(rt_seg);
        if (!_income_win_zero){
            _cons_retrans += 1;
            _retransmit_timer.double_RTO();
        }
        
        // _window_size -= rt_seg.length_in_sequence_space();
        _retransmit_timer.reset();

    }
    else fill_window();
}

unsigned int TCPSender::consecutive_retransmissions() const { return _cons_retrans; }

void TCPSender::send_empty_segment() {
    TCPSegment tesg;
    tesg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(tesg);

}

void TCPSender::send_segment(TCPSegment& tseg) {
    _segments_out.push(tseg);
    _outstanding_segs.push(tseg);
    _bytes_in_flight += tseg.length_in_sequence_space();
    _next_seqno += tseg.length_in_sequence_space();
    _window_size -= tseg.length_in_sequence_space();
    return; 
}
