
// controller.v : Digital logic for decisions
// Inputs: pir, isDark, tempHigh, authorized
// Outputs: lightOn, fanOn, alarmOn
module controller(
  input wire pir,
  input wire isDark,
  input wire tempHigh,
  input wire authorized,
  output wire lightOn,
  output wire fanOn,
  output wire alarmOn
);
  // Light = PIR AND Dark
  assign lightOn = pir & isDark;

  // Fan = TempHigh
  assign fanOn = tempHigh;

  // Alarm = PIR AND NOT(Authorized)
  assign alarmOn = pir & ~authorized;
endmodule
