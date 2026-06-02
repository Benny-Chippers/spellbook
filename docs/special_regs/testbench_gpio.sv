/*
 * Testbench GPIO hookup for tests/test_special_regs.c
 *
 * Paste into wizardCore/src/testBench.sv under `ifdef SIMULATION.
 *
 * 1. Replace top_instance .gpio(gpio) with .gpio(gpio_bus).
 * 2. Add this block after top_instance is instantiated.
 *
 * test_special_regs reads pin 8 as an input; initial value drives it high.
 */

reg [31:0] gpio;

initial begin
    gpio = 32'h0000_0100;   /* bit 8 high for test_gpio_input_pin8() */
end

wire [31:0] gpio_bus;
wire [31:0] gpio_dir = top_instance.SPECIAL.GPIO.r_direction;
wire [31:0] gpio_out = top_instance.SPECIAL.GPIO.r_out_reg;

genvar gi;
generate
    for (gi = 0; gi < 32; gi = gi + 1) begin : gpio_pin_hookup
        assign gpio_bus[gi] = gpio_dir[gi] ? gpio_out[gi] : gpio[gi];
    end
endgenerate
