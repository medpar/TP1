
// TIMER MODULE
module TIMER( 
input clk, 
input [31:0] maxcount, 
input rd, 
input wr, 
output reg flagtimer, //Flag 
output reg [31:0] contador 
); 
reg [31:0]aux=0; 
initial 
begin  
contador = 0; 
flagtimer = 0; 
end 
always @(posedge clk) 
begin 
if (wr)  
begin 
aux <= maxcount; 
contador <= 0; 
flagtimer <= 0; 
end 
if(aux > 0) 
begin 
if(rd)  
flagtimer <= 0; 
if(contador == aux*18) 
begin 
contador <= 0; 
flagtimer <= 1; 
end 
else 
contador <= (contador+1); 
end 
end 
endmodule