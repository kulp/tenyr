`define CLOCKPERIOD 10
`define RAMDELAY (1 * `CLOCKPERIOD)
// TODO use proper reset vectors
`define RESETVECTOR 'h0000
//`define SETUPTIME (`CLOCKPERIOD / 2 - 1)
//`define SETUPTIME 0
//`define SETUP #(`SETUPTIME)
//`define DECODETIME `SETUP
//`define EXECTIME `SETUP
//`define RAMDELAY #2
`define HALT_TENYR 0 // index of Tenyr module halt line
`define HALT_EXEC 1 // index of exec module halt line
`define HALT_SIM 2
`define HALTBUSWIDTH `HALT_LAST + 1 // the number of devices supplying halt signals

`ifdef SIM
    `define HALT_LAST `HALT_SIM
`else
    `define HALT_LAST `HALT_EXEC
`endif

`define HALTTYPE inout[`HALTBUSWIDTH-1:0]

/* vi: set ts=4 sw=4 et syntax=verilog: */

