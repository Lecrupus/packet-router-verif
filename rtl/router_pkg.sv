// router_pkg.sv -- parameters, header struct and helper functions (spec 2.1, 2.2)
package router_pkg;

  localparam int N_PORTS    = 4;
  localparam int PORT_W     = 2;                        // $clog2(N_PORTS)
  localparam int DATA_W     = 32;
  localparam int MAX_LEN    = 31;                       // max payload words
  localparam int FIFO_DEPTH = 64;                       // words per input FIFO
  localparam int CNT_W      = 16;                       // statistics counters (wrap)
  localparam int FIFO_CW    = $clog2(FIFO_DEPTH + 1);   // FIFO count width (7)
  localparam int FIFO_W     = DATA_W + 1;               // FIFO word = {eop, data}
  localparam int PKT_CNT_W  = $clog2(FIFO_DEPTH / 2 + 1); // stored packets per FIFO (<= 32)

  // Header word layout (spec 2.2)
  typedef struct packed {
    logic [15:0] flow_id;   // [31:16] opaque to the router
    logic [3:0]  reserved;  // [15:12] passed through unchanged
    logic [1:0]  src_tag;   // [11:10] overwritten with the ingress port
    logic [1:0]  dst;       // [9:8]   output port
    logic [7:0]  len;       // [7:0]   payload words, legal 1..MAX_LEN
  } hdr_t;

  // R-DUT-02: a header is legal when 1 <= len <= MAX_LEN
  function automatic logic len_ok(input logic [7:0] len);
    return (len != 8'd0) && (len <= 8'(MAX_LEN));
  endfunction

  // R-DUT-04: write the ingress port into bits [11:10], change nothing else
  function automatic logic [DATA_W-1:0] tag_header(input logic [DATA_W-1:0] word,
                                                   input logic [PORT_W-1:0] port);
    hdr_t h;
    h = hdr_t'(word);
    h.src_tag = port;
    return DATA_W'(h);
  endfunction

endpackage
