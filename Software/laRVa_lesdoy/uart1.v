//Peripheral output bus MUX
// Línea 215


//---------------------------------------------
//		Core de UART
//	- Doble función de transmisor y receptor con una FIFO
// Etiqueta: changed_VictorAmalia_FIFO
// Modificado por Víctor Ramos y Amalia Gil (11/2023)
//---------------------------------------------

module UART_FIFO(
  // SALIDAS
  output [7:0]q, // Datos RX				--> Bus de salida
  output txd,    // Salida TX
  output tend,	 // Flag TX completa
  output dv,     // Flag dato RX válido
  output full,   // Flag FIFO llena			// changed_VictorAmalia_FIFO
  output empty,  // Flag FIFO vacía 		// changed_VictorAmalia_FIFO
  output fe,     // Flag Framing Error
  output ove,    // Flag Overrun
  
  //output thre,   // Flag Buffer TX vacío	--> Este es el que se ha sustituido por la FIFO
  
  // ENTRADAS
  input [15:0]d, // Datos TX,BRG			--> Bus de entrada
  input wrtx,    // Escritura en TX
  input wrbaud,	 // Escritura en BRG
  input rxd,     // Entrada RX
  input rd,      // Lectura RX (borra DV)
  input reset,   // Señal de reset global	// changed_VictorAmalia_FIFO
  input clk	 // Señal de reloj
);

// Generación de la tasa de baudios para la comunicación serie
parameter BAUDBITS = 3;	// Cantidad de bits necesarios para representar la tasa de baudios

//////// BAUD Rate Generation /////////
reg [BAUDBITS-1:0]divider = 0;		// Se inicializa el registro divider 
always @(posedge clk) 
	if (wrbaud)
		divider <= d[BAUDBITS-1:0];	// Si la señal wrbaud está activa en divider se meten los bits de d		


///////////////////////////////////////
// Transmisor FIFO
///////////////////////////////////////

// changed_VictorAmalia_FIFO

// Cables intermedios que se van a emplear
wire [7:0]outFIFO;		// Salida de la FIFO
wire datoLoad;			// Activación de la carga del dato en el shifter


// Instancia del módulo FIFO
FIFO #(.ADDRWIDTH(3), .DATAWIDTH(8)) 
		fifo(	
			.DO(outFIFO), 	// Salida de datos desde la FIFO (datos que se están leyendo)
			.EMPTY(empty), 	// Señal que indica si la FIFO está vacía
			.FULL(full),	// Señal que indica si la FIFO está llena
		 	.DI(d[7:0]), 	// Datos de entrada a la FIFO (8 bits)
			.WR(wrtx), 		// Señal de escritura en la FIFO
			.RD(rdf),		// Señal de lectura desde la FIFO
	 		.WCLK(clk), 	// Reloj de escritura para la FIFO
			.RCLK(clk), 	// Reloj de lectura para la FIFO
			.nINIT(reset)	// Señal de reinicio para la FIFO
			);

// Instancia del Shifter que saca los datos en serie
SHIFTER shifter (	.DataIn(outFIFO), 		// Datos de entrada al Shifter
					.enable(enable), 		// Señal de habilitación para el Shifter
					.wr(wr), 				// Señal de escritura en el Shifter
					.outsh(txd), 			// Salida del Shifter (salida de datos serie)
					.clk(clk), 				// Reloj del Shifter
					.reset(reset),			// Señal de reinicio para el Shifter
					.dato_cargado(datoLoad)	// Señal que activa la carga del dato en el desplazador
				);

/*********** Generador del divisor de baudios (TX) ************/

reg [BAUDBITS-1:0] divtx = 0;	// Se inicializa el divisor
wire clko;						// Pulsos de 1 ciclo de salida
assign clko = (divtx==0);		// Si el divisor vale cero, clko se activa
always @ (posedge clk) 
    divtx <= (wrtx&rdy) ? 0 : (clko ? divider: divtx-1);	// Cada vez que haya un flanco positivo del reloj el divisor se actualiza


/********* Transmitter controller ***********/

reg rdy = 1;			// Estado del registro de desplazamiento (1 == idle)
reg [3:0]cntbit;		// Contador de bits que va a emplear el shifter
reg rdf;				// Bit que hace que salga un dato de la FIFO por q
reg wr;					// Bit que hace que se cargue un dato en el shifter
reg enable;				// Bit que habilita el registro de desplazamiento

always @(posedge clk)
begin
	if(clko)
	begin
    if(rdy&(~empty)) 			// Si se está en IDLE y hay dato en la FIFO...
		begin      				// Carga del registro de desplazamiento
			rdy <= 1'b0; 		// Sale del estado ready 
			wr = 1;    			// Se carga el dato en el shifter
			cntbit <= 4'b0000;	// Se inicializa/reinicia el contador de bits a 0
		end
	end	
	if(datoLoad)				// Si se ha cargado un dato...
		begin
			wr = 0;				// Se desactiva la escritura en el shifter
		end
					
	if (clko) 
		begin 
			if(~rdy) begin             	// Desplazamiento de bits
				assign enable = 1;		// Se habilita el desplazamiento
				#50 
				assign enable = 0;		// Se deshabilita el desplazamiento después de 50 ns
				cntbit <= cntbit+1;		// Se incrementa el contador de bits
            if (cntbit[3]&cntbit[0]) 	// Si el primer y el último bit del contador valen 1 (contador = 9)...
				begin
					rdy<=1'b1; 			// Se vuelve al estado IDLE
					rdf <= 1'b1;		// Sale un dato de la FIFO
					#50
					rdf <= 0;			// Se desactiva la salida de datos de la FIFO a los 50 ns
					assign enable = 0;	// Se desactiva la habilitación del shifter
				end
        end
    end
end

// end_changed_VictorAmalia_FIFO


////////////////////////////////////////
// Receptor
////////////////////////////////////////

/// Sincronismo de reloj
reg [1:0]rrxd=0; // RXD registrada dos veces
wire resinc;         // activa si cambio en RXD (resincroniza divisor)
wire falling;          // activa si flanco de bajada en RXD (para start)
always @(posedge clk) rrxd<={rrxd[0],rxd};
assign resinc = rrxd[0]^rrxd[1];
assign falling = (~rrxd[0])&rrxd[1];

/// Divisor
// Genera un pulso en mitad de la cuenta (centro de bit)
// se reinicia con resinc
reg [BAUDBITS-1:0] divrx=0;
wire shift;		// Pulso de 1 ciclo de salida
wire clki0;		// recarga de contador
assign shift = (divrx=={1'b0,divider[BAUDBITS-1:1]});
assign clki0= (divrx==0);

always @ (posedge clk) divrx <= (resinc|clki0) ? divider: divrx-1;

reg dv=0;               // Dato válido si 1
reg ove=0;              // Overrun
reg [8:0]shrx;          // Reg. desplazamiento entrada (9 bits para stop)
reg [7:0]rbr;           // Buffer RX
reg stopb;              // Bit de stop recibido
reg [3:0]cbrx=4'b1111;  // Contador de bits / estado (1111== idle)
wire rxst;              // Guardar shr en buffer si flanco subida
assign rxst=(cbrx==4'b1111);
reg rxst0;              // Para detectar flanco

always @(posedge clk)
begin
    rxst0<=rxst;
    if (rxst & falling) cbrx<=4'h9;  // START: 9 bits a recibir
    if (shift & (~rxst)) begin       // Desplazando y contando bits
        shrx<= #1 {rrxd[0],shrx[8:1]};
        cbrx<=cbrx-1;
    end
    if (rxst & (~rxst0)) begin   // Final de cuenta
        {stopb,rbr}<=shrx;       // Guardando dato y bit STOP
        dv<=1;                   // Dato válido
        ove<=dv;                 // Overrun si ya hay dato válido
    end

    if (rd) begin   // Lectura: Borra flags
        dv<=0;
        ove<=0;
    end
end

assign fe=~stopb;   // el Flag FE es el bit de STOP invertido
assign q = rbr;
endmodule

/////////////////////////////////////////////////////////
// FIFO
// Comentada por Víctor Ramos y Amalia Gil (11/2023)
/////////////////////////////////////////////////////////

module FIFO #(parameter ADDRWIDTH=3, parameter DATAWIDTH=8) //changed_VictorAmalia_FIFO
	(output [7:0]DO, output EMPTY, output FULL, 			//changed_VictorAmalia_FIFO
	 input [7:0]DI,  input WR, 	input RD,					//changed_VictorAmalia_FIFO
	 input WCLK, 	  input RCLK,	input nINIT
	);

	reg [ADDRWIDTH-1:0]ixw=0;		// Inicialización del registro de puntero de escritura
	reg [ADDRWIDTH-1:0]ixr=0;		// Inicialización del registro de puntero de lectura

	wire [ADDRWIDTH-1:0]ixwpp;		// Cable para el próximo puntero de escritura
	wire [ADDRWIDTH-1:0]ixrpp;		// Cable para el próximo puntero de lectura
	
	// Instancia de la RAM de doble puerto
	RAM2PORT #(.addr_width(ADDRWIDTH), .data_width(DATAWIDTH))
		ram	(.din(DI), .waddr(ixw),
			 .write_en(WR & !FULL), .wclk(WCLK),
			 .raddr(ixrpp), .rclk(RCLK), .dout(DO));
			 

	assign ixwpp=ixw+1;		// Se incrementa el puntero de escritura
	assign ixrpp=ixr+1;		// Se incrementa el puntero de lectura

	assign  EMPTY = (ixw==ixrpp);		// EMPTY vale 1 si el puntero de lectura alcanza al de escritura (se dobla la vuelta)
	assign  FULL  = (ixwpp==ixr);		// FULL vale 1 si el puntero de escritura alcanza al de lectura

	always @(posedge WCLK or negedge nINIT) 
		if (~nINIT)						// nINIT equivale al reset
			ixw<=0;						// Se reinicia el puntero de escritura si nINIT es 1 (activo en baja)
		else if (WR & !FULL)			
			ixw<=ixwpp;					// Se actualiza el puntero de escritura si se permite escribir y no está llena la FIFO

	always @(posedge RCLK or negedge nINIT)
		if (~nINIT)
			ixr<={DATAWIDTH{1'b1}};		// Se reinicia el puntero de lectura si nINIT es 1 (activo en baja)
		else if (RD & !EMPTY)
			ixr<=ixrpp;					// Se actualiza el puntero de lectura si se permite leer y no está vacía la FIFO

endmodule

/////////////////////////////////////////////////////////
// MEMORIA DOBLE PUERTO
// Comentada por Víctor Ramos y Amalia Gil (11/2023)
/////////////////////////////////////////////////////////

module RAM2PORT (din, write_en, waddr, wclk,
				 raddr, rclk, dout);

	parameter addr_width = 3; // changed_VictorAmalia_FIFO 	// Parámetro que indica el ancho de las direcciones
	parameter data_width = 8; // changed_VictorAmalia_FIFO	// Parámetro que indica el tamaño de los datos

	input [addr_width-1:0] waddr, raddr;	// Direcciones de escritura y lectura
	input [data_width-1:0] din;				// Datos a escribir
	input write_en, wclk, rclk;				// Habilitación de escritura, reloj de escritura y reloj de lectura
	output reg [data_width-1:0] dout;		// Salida de datos leídos desde la memoria

	reg [data_width-1:0] mem [(1<<addr_width)-1:0];	// Memoria RAM

	integer i;
	
	always @(posedge wclk) 	// Write memory
		begin
			if (write_en)
				mem[waddr] <= din; // Using write address bus (se escriben los datos en la dirección especificada si se habilita la escritura)
		end

	always @(posedge rclk) 	// Read memory
		dout <= mem[raddr]; // Using read address bus (se leen los datos desde la dirección especificada y se asignan a la salida)

	initial
		begin
			$dumpfile("crono.vcd");	
			for (i=0; i<8; i++)
				$dumpvars(0, mem[i]);			
		end
		
endmodule


// begin_changed_VictorAmalia_FIFO
/////////////////////////////////////////////////////////
// SHIFTER
/////////////////////////////////////////////////////////

module SHIFTER(DataIn, enable, wr, outsh, clk, reset, dato_cargado);

	// Variables de entrada y salida que se van a emplear
	input [7:0] DataIn;
	input enable, wr, clk, reset;
	output  outsh;
	output reg	dato_cargado;
	
	reg [8:0] data;	// Interior del shifter (8 datos porque el primer bit es de start)
	
	always @ (posedge clk or negedge reset)
		begin
			if (~reset)				// Señal de reset activa en baja
				data = 8'b11111111;	// Si hay reset la señal vale 1
			
			else if (wr == 1)		// Si se activa la escritura en el shifter
				begin
				data[8:0] <= {DataIn, 1'b0};	//Se carga el dato de la entrada dentro del shifter + señal de start
				assign dato_cargado = 1;		// Hay dato cargado
				end
			else if (enable == 1)				// Si se activa la habilitación...
				begin
					data = {1'b1, data[8:1]};	// Se desplazan los datos hacia la derecha bit a bit
					assign dato_cargado = 0;	// Se reinicia el bit de dato cargado
				end
		end
		
	assign outsh = data[0];		// Se saca por la salida el bit menos significativo (en serie)
endmodule

// end_changed_VictorAmalia_FIFO