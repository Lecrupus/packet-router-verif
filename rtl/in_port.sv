// in_port.sv -- one ingress port: parser FSM (IDLE/PASS/DROP), src tagging,
// 64-word rate-adaptation FIFO, stored-packet counter, head_valid/head_dst.
module in_port
  import router_pkg::*;
#(
  parameter logic [PORT_W-1:0] PORT_ID = '0
) (
  input  logic                 clk,
  input  logic                 rst_n,
  // ingress stream
  input  logic                 in_valid,
  input  logic                 in_sop,
  input  logic                 in_eop,
  input  logic [DATA_W-1:0]    in_data,
  output logic                 in_ready,
  // read side (towards the crossbar)
  input  logic                 rd_en,
  output logic [DATA_W-1:0]    rd_data,
  output logic                 rd_eop,
  output logic                 head_valid,   // a complete packet's header is at the FIFO head
  output logic [PORT_W-1:0]    head_dst,
  // statistics pulses
  output logic                 good_pkt,     // good packet's eop written into the FIFO
  output logic                 drop_pkt,     // bad packet's eop consumed (dropped)
  // debug
  output logic [FIFO_CW-1:0]   fifo_count
);

  typedef enum logic [1:0] {IDLE = 2'd0, PASS = 2'd1, DROP = 2'd2} state_e;
  state_e state;

  /* verilator lint_off UNUSEDSIGNAL */  // only some header fields are used
  hdr_t hdr_in, hdr_head;
  /* verilator lint_on UNUSEDSIGNAL */
  assign hdr_in = hdr_t'(in_data);

  logic fifo_full, fifo_empty;
  logic hdr_bad, drop_word, xfer, fifo_wr;
  logic [DATA_W-1:0] wr_word;
  logic [FIFO_W-1:0] fifo_rd;

  // R-DUT-02: a bad header and everything up to its eop is consumed but not stored,
  // so in_ready stays 1 while dropping even when the FIFO is full.
  assign hdr_bad   = (state == IDLE) && !len_ok(hdr_in.len);
  assign drop_word = (state == DROP) || hdr_bad;
  // R-DUT-05: in_ready = 0 only when the FIFO is full (and the word would be stored)
`ifdef BUG_DROP_STALL
  assign in_ready  = !fifo_full;               // injected bug: drop path stalls when full
`else
  assign in_ready  = !fifo_full || drop_word;
`endif
  assign xfer      = in_valid && in_ready;
  assign fifo_wr   = xfer && !drop_word;

`ifdef BUG_SRC_TAG
  // Deliberately injected bug (week-2 exercise): wrong src tag
  assign wr_word = (state == IDLE) ? tag_header(in_data, PORT_ID ^ 2'd1) : in_data;
`else
  // R-DUT-04: tag the header with the ingress port
  assign wr_word = (state == IDLE) ? tag_header(in_data, PORT_ID) : in_data;
`endif

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state <= IDLE;
    end else if (xfer) begin
      unique case (state)
        IDLE:    state <= in_eop ? IDLE : (hdr_bad ? DROP : PASS);
        PASS:    if (in_eop) state <= IDLE;
        DROP:    if (in_eop) state <= IDLE;
        default: state <= IDLE;
      endcase
    end
  end

  assign good_pkt = fifo_wr && in_eop;
  assign drop_pkt = xfer && drop_word && in_eop;

  sync_fifo #(.WIDTH(FIFO_W), .DEPTH(FIFO_DEPTH)) u_fifo (
    .clk     (clk),
    .rst_n   (rst_n),
    .wr_en   (fifo_wr),
    .wr_data ({in_eop, wr_word}),
    .rd_en   (rd_en),
    .rd_data (fifo_rd),
    .full    (fifo_full),
    .empty   (fifo_empty),
    .count   (fifo_count)
  );

  assign rd_data = fifo_rd[DATA_W-1:0];
  assign rd_eop  = fifo_rd[DATA_W];

  // R-DUT-06 store-and-forward: count complete packets held in the FIFO.
  logic [PKT_CNT_W-1:0] pkt_cnt;
  logic rd_active;            // a packet is part-way through being read out
  logic pkt_rd_done, pkt_stored;
  assign pkt_rd_done = rd_en && rd_eop;
`ifdef BUG_CUT_THROUGH
  assign pkt_stored  = fifo_wr && (state == IDLE);  // injected bug: eligible at header, not eop
`else
  assign pkt_stored  = good_pkt;
`endif

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      pkt_cnt   <= '0;
      rd_active <= 1'b0;
    end else begin
      if (pkt_stored && !pkt_rd_done)      pkt_cnt <= pkt_cnt + 1'b1;
      else if (pkt_rd_done && !pkt_stored) pkt_cnt <= pkt_cnt - 1'b1;
      if (rd_en) rd_active <= !rd_eop;
    end
  end

  assign hdr_head   = hdr_t'(rd_data);
  assign head_valid = (pkt_cnt != '0) && !rd_active;
  assign head_dst   = hdr_head.dst;

  // fifo_empty is only used by the bound assertions. in_sop is not needed: the parser
  // treats the first word after an eop (or reset) as the header (R-DUT-03 framing);
  // the testbench's protocol checker flags misplaced sop.
  logic unused_ok;
  assign unused_ok = ^{fifo_empty, in_sop};

endmodule
