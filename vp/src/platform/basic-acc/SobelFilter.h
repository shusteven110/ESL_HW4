#ifndef SOBEL_FILTER_H_
#define SOBEL_FILTER_H_
#include <systemc>
#include <cmath>
#include <iomanip>
using namespace sc_core;

#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "filter_def.h"

struct SobelFilter : public sc_module {
  tlm_utils::simple_target_socket<SobelFilter> tsock;

  sc_fifo<unsigned char> i_r;
  sc_fifo<unsigned char> i_g;
  sc_fifo<unsigned char> i_b;
  sc_fifo<unsigned int> o_result;

  SC_HAS_PROCESS(SobelFilter);

  SobelFilter(sc_module_name n): 
    sc_module(n), 
    tsock("t_skt"), 
    base_offset(0) 
  {
    tsock.register_b_transport(this, &SobelFilter::blocking_transport);
    SC_THREAD(do_filter);
  }

  ~SobelFilter() {
	}

  // int val[MASK_N];
  unsigned int base_offset;

  sc_dt::sc_uint<12> reds[9];
  sc_dt::sc_uint<12> greens[9];
  sc_dt::sc_uint<12> blues[9];
  unsigned char median_r;
  unsigned char median_g; 
  unsigned char median_b;
  sc_dt::sc_uint<8> o_r;
  sc_dt::sc_uint<8> o_g;
  sc_dt::sc_uint<8> o_b;
  
  int mask[9]={1,1,1,1,2,1,1,1,1};
  
  void do_filter(){
    { wait(CLOCK_PERIOD, SC_NS); }
    while (true) {
      // for (unsigned int i = 0; i < MASK_N; ++i) {
      //   val[i] = 0;
      //   wait(CLOCK_PERIOD, SC_NS);
      // }
      // for (unsigned int v = 0; v < MASK_Y; ++v) {
      //   for (unsigned int u = 0; u < MASK_X; ++u) {
      //     unsigned char grey = (i_r.read() + i_g.read() + i_b.read()) / 3;
      //     wait(CLOCK_PERIOD, SC_NS);
      //     for (unsigned int i = 0; i != MASK_N; ++i) {
      //       val[i] += grey * mask[i][u][v];
      //       wait(CLOCK_PERIOD, SC_NS);
      //     }
      //   }
      // }
      // double total = 0;
      // for (unsigned int i = 0; i != MASK_N; ++i) {
      //   total += val[i] * val[i];
      //   wait(CLOCK_PERIOD, SC_NS);
      // }
      // int result = static_cast<int>(std::sqrt(total));

      // // cout << (int)result << endl;

      // o_result.write(result);
      for (unsigned int v = 0; v < MASK_Y; ++v) {
        for (unsigned int u = 0; u < MASK_X; ++u) {
          reds[v * 3 + u] = i_r.read();
          greens[v * 3 + u] = i_g.read();
          blues[v * 3 + u] = i_b.read();
        }
      }

      std::sort(&reds[0], &reds[9]);
      std::sort(&greens[0], &greens[9]);
      std::sort(&blues[0], &blues[9]);

      median_r = reds[4];
      median_g = greens[4];
      median_b = blues[4];

      o_r = 0;
      o_g = 0;
      o_b = 0;

      for (int i = 0; i < 9; i++) {
        o_r += reds[i]*mask[i];
        o_g += greens[i]*mask[i];
        o_b += blues[i]*mask[i];
      }

      o_r -= median_r;
      o_g -= median_g;
      o_b -= median_b;
      o_r /= 10;
      o_g /= 10;
      o_b /= 10;

      sc_dt::sc_uint<32> result = (0, o_b, o_g, o_r);

      o_result.write(result);
    }
  }

  void blocking_transport(tlm::tlm_generic_payload &payload, sc_core::sc_time &delay){
    wait(delay);
    // unsigned char *mask_ptr = payload.get_byte_enable_ptr();
    // auto len = payload.get_data_length();
    tlm::tlm_command cmd = payload.get_command();
    sc_dt::uint64 addr = payload.get_address();
    unsigned char *data_ptr = payload.get_data_ptr();

    addr -= base_offset;


    // cout << (int)data_ptr[0] << endl;
    // cout << (int)data_ptr[1] << endl;
    // cout << (int)data_ptr[2] << endl;
    word buffer;

    switch (cmd) {
      case tlm::TLM_READ_COMMAND:
        // cout << "READ" << endl;
        switch (addr) {
          case SOBEL_FILTER_RESULT_ADDR:
            buffer.uint = o_result.read();
            break;
          default:
            std::cerr << "READ Error! SobelFilter::blocking_transport: address 0x"
                      << std::setfill('0') << std::setw(8) << std::hex << addr
                      << std::dec << " is not valid" << std::endl;
          }
        data_ptr[0] = buffer.uc[0];
        data_ptr[1] = buffer.uc[1];
        data_ptr[2] = buffer.uc[2];
        data_ptr[3] = buffer.uc[3];
        break;
      case tlm::TLM_WRITE_COMMAND:
        // cout << "WRITE" << endl;
        switch (addr) {
          case SOBEL_FILTER_R_ADDR:
            i_r.write(data_ptr[3]);
            i_g.write(data_ptr[2]);
            i_b.write(data_ptr[1]);
            break;
          default:
            std::cerr << "WRITE Error! SobelFilter::blocking_transport: address 0x"
                      << std::setfill('0') << std::setw(8) << std::hex << addr
                      << std::dec << " is not valid" << std::endl;
        }
        break;
      case tlm::TLM_IGNORE_COMMAND:
        payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
        return;
      default:
        payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
        return;
      }
      payload.set_response_status(tlm::TLM_OK_RESPONSE); // Always OK
  }
};
#endif
