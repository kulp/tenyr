`define CLOCKPERIOD 100
`define RAMDELAY (1 * `CLOCKPERIOD)
// TODO use proper reset vectors
`define RESETVECTOR 'h0000

`define HALT_EXTERNAL 0
`define HALT_TENYR 1 // index of Tenyr module halt line
`define HALT_EXEC 2 // index of exec module halt line

`define HALT_LAST `HALT_EXEC
`define HALTBUSWIDTH `HALT_LAST + 1 // the number of devices supplying halt signals

`define HALTTYPE [`HALTBUSWIDTH-1:0]

`define VIDEO_ADDR 'h10000
`define EDGE negedge

/* vi: set ts=4 sw=4 et syntax=verilog: */

