// out_port.sv -- crossbar mux for one output. Latches the arbiter's grant and holds it
// until the granted packet's eop transfers (R-DUT-08). Data comes straight from the
// granted input FIFO's head, which cannot change while it is not popped, so the output
// is stable under backpressure (R-DUT-09).
module out_port
  import router_pkg::*;
(
  input  logic                           clk,
  input  logic                           rst_n,
  // arbiter
  input  logic                           gnt_valid,
  input  logic [PORT_W-1:0]              gnt_idx,
  output logic                           idle,
  // input FIFO heads
  input  logic [N_PORTS-1:0][DATA_W-1:0] rd_data,
  input  logic [N_PORTS-1:0]             rd_eop,
  output logic [N_PORTS-1:0]             rd_en,
  // egress stream
  output logic                           out_valid,
  output logic                           out_sop,
  output logic                           out_eop,
  output logic [DATA_W-1:0]              out_data,
  input  logic                           out_ready,
  // statistics
  output logic                           pkt_sent
);

  logic              busy;
  logic              first;      // next word to send is the header
  logic [PORT_W-1:0] owner;
  logic              xfer;

  assign xfer = out_valid && out_ready;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      busy  <= 1'b0;
      first <= 1'b0;
      owner <= '0;
    end else if (!busy) begin
      if (gnt_valid) begin
        busy  <= 1'b1;
        first <= 1'b1;
        owner <= gnt_idx;
      end
    end else if (xfer) begin
      first <= 1'b0;
      if (out_eop) busy <= 1'b0;
    end
  end

  assign idle      = !busy;
  // Store-and-forward guarantees the whole packet is in the owner's FIFO.
`ifdef BUG_SLOW_GRANT
  // Injected bug: an extra pipeline cycle between grant and header (violates R-DUT-12)
  logic warm;
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) warm <= 1'b0;
    else        warm <= busy;
  end
  assign out_valid = busy && warm;
`else
  assign out_valid = busy;
`endif
  assign out_sop   = busy && first;
  assign out_eop   = busy && rd_eop[owner];
  assign out_data  = rd_data[owner];
  assign rd_en     = xfer ? (N_PORTS'(1) << owner) : '0;
  assign pkt_sent  = xfer && out_eop;

endmodule
