// bind_router.sv -- attach the assertion modules to every instance of each RTL module.
// The design files contain no verification code.

bind sync_fifo fifo_sva u_fifo_sva (
  .clk   (clk),
  .rst_n (rst_n),
  .wr_en (wr_en),
  .rd_en (rd_en),
  .full  (full),
  .empty (empty)
);

bind rr_arbiter arb_sva #(.N(N), .W(W)) u_arb_sva (
  .clk       (clk),
  .rst_n     (rst_n),
  .gnt       (gnt),
  .gnt_valid (gnt_valid),
  .gnt_idx   (gnt_idx),
  .ptr       (ptr)
);

bind out_port out_port_sva u_out_sva (
  .clk       (clk),
  .rst_n     (rst_n),
  .busy      (busy),
  .owner     (owner),
  .out_valid (out_valid),
  .out_ready (out_ready),
  .out_sop   (out_sop),
  .out_eop   (out_eop),
  .out_data  (out_data)
);

bind in_port in_port_sva u_in_sva (
  .clk      (clk),
  .rst_n    (rst_n),
  .in_valid (in_valid),
  .in_ready (in_ready),
  .in_eop   (in_eop),
  .state    (state)
);
