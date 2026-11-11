// sync_fifo.sv -- parameterized show-ahead synchronous FIFO with full/empty/count.
// rd_data always shows the head word; rd_en pops it. DEPTH must be a power of two.
module sync_fifo #(
  parameter int WIDTH = 33,
  parameter int DEPTH = 64,
  localparam int AW = $clog2(DEPTH),
  localparam int CW = $clog2(DEPTH + 1)
) (
  input  logic             clk,
  input  logic             rst_n,
  input  logic             wr_en,
  input  logic [WIDTH-1:0] wr_data,
  input  logic             rd_en,
  output logic [WIDTH-1:0] rd_data,
  output logic             full,
  output logic             empty,
  output logic [CW-1:0]    count
);

  logic [WIDTH-1:0] mem [DEPTH];
  logic [AW-1:0]    wr_ptr, rd_ptr;

  // Writes when full / reads when empty are ignored here; router_sva A-01 flags them.
  logic do_wr, do_rd;
  assign do_wr = wr_en && !full;
  assign do_rd = rd_en && !empty;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      wr_ptr <= '0;
      rd_ptr <= '0;
      count  <= '0;
    end else begin
      if (do_wr) wr_ptr <= wr_ptr + 1'b1;
      if (do_rd) rd_ptr <= rd_ptr + 1'b1;
      if (do_wr && !do_rd)      count <= count + 1'b1;
      else if (do_rd && !do_wr) count <= count - 1'b1;
    end
  end

  // Storage has no reset (it is a RAM); --x-initial unique randomizes it per seed.
  always_ff @(posedge clk) begin
    if (do_wr) mem[wr_ptr] <= wr_data;
  end

  assign rd_data = mem[rd_ptr];
  assign full    = (count == CW'(DEPTH));
  assign empty   = (count == '0);

endmodule
