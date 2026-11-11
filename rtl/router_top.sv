// router_top.sv -- 4x4 store-and-forward packet router.
// 4 in_port (parser + FIFO), 4 rr_arbiter, 4 out_port (crossbar mux), statistics counters.
module router_top
  import router_pkg::*;
(
  input  logic                           clk,
  input  logic                           rst_n,
  // ingress (one stream per input port)
  input  logic [N_PORTS-1:0]             in_valid,
  input  logic [N_PORTS-1:0]             in_sop,
  input  logic [N_PORTS-1:0]             in_eop,
  input  logic [N_PORTS-1:0][DATA_W-1:0] in_data,
  output logic [N_PORTS-1:0]             in_ready,
  // egress (one stream per output port)
  output logic [N_PORTS-1:0]             out_valid,
  output logic [N_PORTS-1:0]             out_sop,
  output logic [N_PORTS-1:0]             out_eop,
  output logic [N_PORTS-1:0][DATA_W-1:0] out_data,
  input  logic [N_PORTS-1:0]             out_ready,
  // statistics
  output logic [N_PORTS-1:0][CNT_W-1:0]  pkt_in_cnt,
  output logic [N_PORTS-1:0][CNT_W-1:0]  pkt_out_cnt,
  output logic [N_PORTS-1:0][CNT_W-1:0]  drop_cnt
);

  // ---------------------------------------------------------------------------
  // R-DUT-14: reset asserted asynchronously, released synchronously
  // ---------------------------------------------------------------------------
  logic [1:0] rst_sync;
  logic       rst_ni;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) rst_sync <= 2'b00;
    else        rst_sync <= {rst_sync[0], 1'b1};
  end
  assign rst_ni = rst_sync[1];

  // ---------------------------------------------------------------------------
  // Input ports
  // ---------------------------------------------------------------------------
  logic [N_PORTS-1:0][DATA_W-1:0] rd_data;
  logic [N_PORTS-1:0]             rd_eop;
  logic [N_PORTS-1:0]             rd_en_in;     // [input]
  logic [N_PORTS-1:0]             head_valid;
  logic [N_PORTS-1:0][PORT_W-1:0] head_dst;
  logic [N_PORTS-1:0]             good_pkt, drop_pkt;
  logic [N_PORTS-1:0][FIFO_CW-1:0] fifo_count;

  for (genvar i = 0; i < N_PORTS; i++) begin : g_in
    in_port #(.PORT_ID(PORT_W'(i))) u_in (
      .clk        (clk),
      .rst_n      (rst_ni),
      .in_valid   (in_valid[i]),
      .in_sop     (in_sop[i]),
      .in_eop     (in_eop[i]),
      .in_data    (in_data[i]),
      .in_ready   (in_ready[i]),
      .rd_en      (rd_en_in[i]),
      .rd_data    (rd_data[i]),
      .rd_eop     (rd_eop[i]),
      .head_valid (head_valid[i]),
      .head_dst   (head_dst[i]),
      .good_pkt   (good_pkt[i]),
      .drop_pkt   (drop_pkt[i]),
      .fifo_count (fifo_count[i])
    );
  end

  // ---------------------------------------------------------------------------
  // Arbiters + output ports. req[o][i]: input i's head packet targets output o.
  // ---------------------------------------------------------------------------
  logic [N_PORTS-1:0][N_PORTS-1:0] req;         // [output][input]
  logic [N_PORTS-1:0][N_PORTS-1:0] gnt;         // [output][input]
  logic [N_PORTS-1:0][N_PORTS-1:0] rd_en_out;   // [output][input]
  logic [N_PORTS-1:0][PORT_W-1:0]  gnt_idx;
  logic [N_PORTS-1:0]              gnt_valid;
  logic [N_PORTS-1:0]              out_idle;
  logic [N_PORTS-1:0]              pkt_sent;
  logic [N_PORTS-1:0][PORT_W-1:0]  arb_ptr;     // only observed by assertions

  for (genvar o = 0; o < N_PORTS; o++) begin : g_out
    for (genvar i = 0; i < N_PORTS; i++) begin : g_req
      assign req[o][i] = head_valid[i] && (head_dst[i] == PORT_W'(o));
    end

    rr_arbiter #(.N(N_PORTS)) u_arb (
      .clk       (clk),
      .rst_n     (rst_ni),
      .req       (req[o]),
      .en        (out_idle[o]),
      .gnt       (gnt[o]),
      .gnt_idx   (gnt_idx[o]),
      .gnt_valid (gnt_valid[o]),
      .ptr       (arb_ptr[o])
    );

    out_port u_out (
      .clk       (clk),
      .rst_n     (rst_ni),
      .gnt_valid (gnt_valid[o]),
      .gnt_idx   (gnt_idx[o]),
      .idle      (out_idle[o]),
      .rd_data   (rd_data),
      .rd_eop    (rd_eop),
      .rd_en     (rd_en_out[o]),
      .out_valid (out_valid[o]),
      .out_sop   (out_sop[o]),
      .out_eop   (out_eop[o]),
      .out_data  (out_data[o]),
      .out_ready (out_ready[o]),
      .pkt_sent  (pkt_sent[o])
    );
  end

  // An input is read by at most one output (the one holding its grant).
  for (genvar i = 0; i < N_PORTS; i++) begin : g_rd
    logic [N_PORTS-1:0] by_out;
    for (genvar o = 0; o < N_PORTS; o++) begin : g_col
      assign by_out[o] = rd_en_out[o][i];
    end
    assign rd_en_in[i] = |by_out;
  end

  // ---------------------------------------------------------------------------
  // R-DUT-13: statistics counters (wrap at 2^CNT_W)
  // ---------------------------------------------------------------------------
  always_ff @(posedge clk or negedge rst_ni) begin
    if (!rst_ni) begin
      pkt_in_cnt  <= '0;
      pkt_out_cnt <= '0;
      drop_cnt    <= '0;
    end else begin
      for (int p = 0; p < N_PORTS; p++) begin
        if (good_pkt[p]) pkt_in_cnt[p]  <= pkt_in_cnt[p]  + 1'b1;
`ifndef BUG_NO_DROP_CNT
        if (drop_pkt[p]) drop_cnt[p]    <= drop_cnt[p]    + 1'b1;
`endif
        if (pkt_sent[p]) pkt_out_cnt[p] <= pkt_out_cnt[p] + 1'b1;
      end
    end
  end

  // ---------------------------------------------------------------------------
  // Debug signals read by the C++ protocol checker and coverage (P-05, P-06, cg_arb,
  // cg_fifo). Read-only from the testbench; no effect on the design.
  // ---------------------------------------------------------------------------
  logic [N_PORTS-1:0][FIFO_CW-1:0] dbg_fifo_count /*verilator public_flat_rd*/;
  logic [N_PORTS-1:0][N_PORTS-1:0] dbg_req        /*verilator public_flat_rd*/;
  logic [N_PORTS-1:0]              dbg_gnt_valid  /*verilator public_flat_rd*/;
  logic [N_PORTS-1:0][PORT_W-1:0]  dbg_gnt_idx    /*verilator public_flat_rd*/;

  assign dbg_fifo_count = fifo_count;
  assign dbg_req        = req;
  assign dbg_gnt_valid  = gnt_valid;
  assign dbg_gnt_idx    = gnt_idx;

  // gnt is consumed by the bound assertions (A-02)
  logic unused_ok;
  assign unused_ok = ^{gnt, arb_ptr};

endmodule
