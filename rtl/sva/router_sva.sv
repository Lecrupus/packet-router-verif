// router_sva.sv -- RTL assertions A-01..A-06 (spec 7.5), attached with bind
// (see bind_router.sv). Kept to the SVA subset Verilator supports: |->, |=>, $past, $stable.
// Failures call into the C++ testbench through DPI so they become logged ERRORs with
// the assertion ID in the message, which the regression buckets by.
/* verilator coverage_off */
/* verilator lint_off DECLFILENAME */

package router_sva_pkg;
  import "DPI-C" function void sva_fail(input string msg);
endpackage

// A-01: FIFO never written when full, never read when empty
module fifo_sva (
  input logic clk,
  input logic rst_n,
  input logic wr_en,
  input logic rd_en,
  input logic full,
  input logic empty
);
  import router_sva_pkg::*;
  a01_no_write_full: assert property (@(posedge clk) disable iff (!rst_n) !(wr_en && full))
    else sva_fail($sformatf("A-01 %m: FIFO written while full"));
  a01_no_read_empty: assert property (@(posedge clk) disable iff (!rst_n) !(rd_en && empty))
    else sva_fail($sformatf("A-01 %m: FIFO read while empty"));
endmodule

// A-02: grant vector one-hot or zero.  A-05: after a grant to k, ptr = k+1
module arb_sva #(parameter int N = 4, parameter int W = 2) (
  input logic         clk,
  input logic         rst_n,
  input logic [N-1:0] gnt,
  input logic         gnt_valid,
  input logic [W-1:0] gnt_idx,
  input logic [W-1:0] ptr
);
  import router_sva_pkg::*;
  a02_onehot0: assert property (@(posedge clk) disable iff (!rst_n) $onehot0(gnt))
    else sva_fail($sformatf("A-02 %m: grant vector not one-hot"));
  a05_ptr_rotates: assert property (@(posedge clk) disable iff (!rst_n)
                                    gnt_valid |=> (ptr == W'($past(gnt_idx) + 1'b1)))
    else sva_fail($sformatf("A-05 %m: priority pointer did not move past the granted input"));
endmodule

// A-03: grant held from sop to eop transfer.  A-04: output stable while stalled
module out_port_sva #(parameter int DW = 32, parameter int PW = 2) (
  input logic          clk,
  input logic          rst_n,
  input logic          busy,
  input logic [PW-1:0] owner,
  input logic          out_valid,
  input logic          out_ready,
  input logic          out_sop,
  input logic          out_eop,
  input logic [DW-1:0] out_data
);
  import router_sva_pkg::*;
  a03_grant_held: assert property (@(posedge clk) disable iff (!rst_n)
                                   busy && !(out_valid && out_ready && out_eop)
                                   |=> busy && $stable(owner))
    else sva_fail($sformatf("A-03 %m: grant released or changed before eop"));
  a04_stable: assert property (@(posedge clk) disable iff (!rst_n)
                               out_valid && !out_ready
                               |=> out_valid && $stable(out_data) && $stable(out_sop) && $stable(out_eop))
    else sva_fail($sformatf("A-04 %m: output changed while out_valid && !out_ready"));
endmodule

// A-06: parser FSM is IDLE whenever no packet is in progress (i.e. right after an eop)
module in_port_sva (
  input logic       clk,
  input logic       rst_n,
  input logic       in_valid,
  input logic       in_ready,
  input logic       in_eop,
  input logic [1:0] state
);
  import router_sva_pkg::*;
  a06_idle_after_eop: assert property (@(posedge clk) disable iff (!rst_n)
                                       in_valid && in_ready && in_eop |=> state == 2'd0)
    else sva_fail($sformatf("A-06 %m: parser not IDLE after eop"));
endmodule

/* verilator coverage_on */
