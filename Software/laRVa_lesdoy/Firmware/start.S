# 1 "Firmware/start.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "Firmware/start.S"

##############################################################################
# RESET & IRQ
##############################################################################

 .global main, irq1_handler, irq2_handler, irq3_handler

 .section .boot
reset_vec:
 j start
.section .text

######################################
### Main program
######################################

start:
# DESDE AQUI changed_VictorAmalia_FIFO

 lui a0, (0xE0000)  		# Cargar los bits más significativos de la dirección
 addi a0, a0, 0x20  		# Añadir los bits menos significativos
 addi a1, zero, +7			# a1=7=0x9B (divider de la UART)
 sw a1, 4(a0)  				# divider = 7
 
 n = 0

la a1, hello

send_loop:
	
	# Cargar el carácter actual en a2
	lb a2, n(a1)

	# Si a2 es el carácter nulo, terminar
	beqz a2, end

	# Enviar el carácter a través de la UART
	sb a2, 0(a0)
	
	# Interrupción FULL
	# Se comprueba si FULL está activo. Si sí, salta a fullInt_ISR
	li a3, 0xE00000E4
	lw a4, 0(a3)
	xori a4, a4, +1
	beqz a4, fullInt_ISR

	# Incrementar el índice y avanzar al siguiente carácter
	n = n+1
	j send_loop

end:
 j end			# end

# Interrupción FULL
fullInt_ISR:
	li a3, 0xE00000E4
	lw a4, 0(a3)
	xori a4, a4, +1
	beqz a4, fullInt_ISR
	mret
		
# Interrupción DV
#dvInt_ISR:
#	li a5, 0xE00000E8
#	lw a6, 0(a5)
#	mret

# String
hello:
	.asciz "hola mundo"


#HASTA AQUI changed_VictorAmalia_FIFO
 li sp,8192

# copy data section
 la a0, _sdata
 la a1, _sdata_values
 la a2, _edata
 bge a0, a2, end_init_data
loop_init_data:
 lw a3,0(a1)
 sw a3,0(a0)
 addi a0,a0,4
 addi a1,a1,4
 blt a0, a2, loop_init_data
end_init_data:
# zero-init bss section
 la a0, _sbss
 la a1, _ebss
 bge a0, a1, end_init_bss
loop_init_bss:
 sw zero, 0(a0)
 addi a0, a0, 4
 blt a0, a1, loop_init_bss
end_init_bss:
# call main
 call main
loop:
 j loop

 .globl delay_loop
delay_loop:
 addi a0,a0,-1
 bnez a0, delay_loop
 ret
