<<<<<<< HEAD
//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:												  	//
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			    //
//////////////////////////////////////////////////////////////////

module TIMER( 
    input clk, 
    input rd, 
    input wr, 
    input [31:0] maximo, 
    
    output reg flag_temp,
    output reg [31:0] cont 
); 

reg [31:0]aux=0; 

// Valores iniciales al arranque
initial 
    begin  
    cont = 0; 
    flag_temp = 0; 
    end 

// Contador sincronizado con el reloj del sistema
always @(posedge clk) 
    begin 
        // Escritura: carga nuevo periodo y reinicia contador/bandera
        if (wr)  
            begin 
                aux <= maximo; 
                cont <= 0; 
                flag_temp <= 0; 
            end 
            
        if(aux > 0) 
            begin 
                // Lectura: limpia la bandera para detectar nuevo evento
                if(rd)  
                    flag_temp <= 0;

                // Compara con el periodo ajustado por el reloj (18)
                if(cont == aux*18) 
                    begin 
                        cont <= 0; 
                        flag_temp <= 1; 
                    end 
                else 
                    cont <= (cont+1); 
            end 
    end 
=======

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
>>>>>>> 4deec76f3dae665a120623fd9830555733c0b73a
endmodule