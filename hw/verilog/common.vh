`timescale 1ns/10ps

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
`define HALTBUSWIDTH 2 // the number of devices supplying halt signals
`define HALTTYPE inout[`HALTBUSWIDTH-1:0]

/* vi: set ts=4 sw=4 et syntax=verilog: */

