// rr_arbiter.sv -- N-way round-robin arbiter with a rotating priority pointer.
// Search starts at ptr; after granting input k the pointer moves to k+1 (R-DUT-07).
// N must be a power of two so the index arithmetic wraps naturally.
module rr_arbiter #(
  parameter int N = 4,
  localparam int W = $clog2(N)
) (
  input  logic         clk,
  input  logic         rst_n,
  input  logic [N-1:0] req,
  input  logic         en,          // output is idle and may take a new grant
  output logic [N-1:0] gnt,         // one-hot or zero
  output logic [W-1:0] gnt_idx,
  output logic         gnt_valid,
  output logic [W-1:0] ptr
);

  logic         found;
  logic [W-1:0] cand;

  always_comb begin
    found   = 1'b0;
    gnt_idx = '0;
    cand    = '0;
    for (int k = 0; k < N; k++) begin
      cand = ptr + W'(k);
      if (!found && req[cand]) begin
        found   = 1'b1;
        gnt_idx = cand;
      end
    end
    gnt_valid = en && found;
    gnt       = gnt_valid ? (N'(1) << gnt_idx) : '0;
  end

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      ptr <= '0;
`ifdef BUG_ARB_NO_ROTATE
    end else if (gnt_valid && 1'b0) begin   // injected bug: pointer never rotates
      ptr <= gnt_idx + 1'b1;
`else
    end else if (gnt_valid) begin
      ptr <= gnt_idx + 1'b1;
`endif
    end
  end

endmodule
