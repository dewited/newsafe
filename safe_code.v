//Donald Wight
//lab 4 verilog code
//****************************************************************************************************
//This code is written for a cpld which is used to direct commands from the master to the slave 
//device.
//****************************************************************************************************
// code for the top module
module lab4 (input [7:0] bus_in, switch, input enable, input [3:0]address, output [7:0] led_on, inout [7:0] bus, output lcd_en);
	wire led_en;
	wire switches_en;
	// instance one for switches
	switches switch_1(.switch(switch), .bus_in(bus_in), .bus(bus), .enable(enable), .switches_en(switches_en));
	// instance one for the led
	led led_1 (.bus_in(bus_in), .enable(enable), .led_on(led_on), .clk(), .bus(bus), .led_en(led_en));
	decoder decoder_1 (.address(address), .switches_en(switches_en), .led_en(led_en), .lcd_en(lcd_en));
endmodule
//  module to take in values from the switches
module switches (input [7:0] switch, input switches_en, input enable, output [7:0] bus, input [7:0] bus_in);
	//assign bus_in to bus
	assign bus_in = bus;
	// assign bus to the inverse of switch if the enable is set and the address is set
	assign bus = (enable == 1'b1 && switches_en == 1'b1) ? ~switch :8'hz;
endmodule
// module to turn on the led
module led ( input [7:0] bus_in, input led_en, input enable, clk, output reg[7:0] led_on, input [7:0] bus);
	// uses a clock to run the always loop
	OSCC oscc0 (clk);
	// assigns bus_in to bus for good measure
	assign bus_in = bus;
	// alwas at the positive clock edge it will run
	always @ (posedge clk) begin
		// if the enable is set to 0 and the address is pointing to the led
		if (enable==1'b0 && led_en == 1'b1)
			// we set led_on to the inverse of bus_in whichis the xor valyue
			led_on =~bus_in;
		else
			// else we turn all cpld led's off
			led_on = 8'b11111111;
	end
endmodule
module decoder (/*input [3:0] address, output reg led_en, output reg switches_en, output reg lcd_en*/ input [3:0] address, output reg led_en, switches_en, lcd_en);
	always @ (address) begin
		case (address)
			4'b1000: {led_en, switches_en, lcd_en} <= 3'b001;
			4'b1001: {led_en, switches_en, lcd_en} <= 3'b001;
			4'b1100: {led_en, switches_en, lcd_en} <= 3'b100;
			4'b1101: {led_en, switches_en, lcd_en} <= 3'b010;
			default: {led_en, switches_en, lcd_en} <= 3'b000;
		endcase 
	end
	
endmodule
			
