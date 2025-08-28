
// tb_controller.v : Testbench
`timescale 1ns/1ps
module tb_controller;
  reg pir, isDark, tempHigh, authorized;
  wire lightOn, fanOn, alarmOn;

  controller dut(.pir(pir), .isDark(isDark), .tempHigh(tempHigh), .authorized(authorized),
                 .lightOn(lightOn), .fanOn(fanOn), .alarmOn(alarmOn));

  initial begin
    $display("pir isDark tempHigh auth | light fan alarm");
    foreach_loop();
    $finish;
  end

  task foreach_loop;
    integer i;
    begin
      for (i=0;i<16;i=i+1) begin
        {pir, isDark, tempHigh, authorized} = i[3:0];
        #1;
        $display("%0b    %0b      %0b        %0b   |   %0b     %0b    %0b",
          pir, isDark, tempHigh, authorized, lightOn, fanOn, alarmOn);
      end
    end
  endtask
endmodule
