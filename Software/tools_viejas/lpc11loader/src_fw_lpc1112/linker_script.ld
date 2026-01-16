/* useful reference: www.linuxselfhelp.com/gnu/ld/html_chapter/ld_toc.html */
MEMORY
{
    flash : org = 0x00000000, len = 16k
    ram   : org = 0x10000000, len = 4k-256
}
  
SECTIONS
{
        
	. = ORIGIN(flash);
    .text : {
		FILL(0xFF);
		*(.vectors);	/* The interrupt vectors */
		*(.bcrp);		/* space before CRP (572 bytes) */
		. = 0x2fc; 
		*(.crp);		/* code read protection word */
		*(.text.startup);
		*(.text);
    } >flash 

	. = ORIGIN(ram);
	.data : {
		INIT_DATA_VALUES = LOADADDR(.data);
		INIT_DATA_START = .;
		*(.data);
		INIT_DATA_END = .;
	} >ram AT>flash

	BSS_START = .;
	__bss_start__ = .;
	.bss : {
		*(.bss);
	} > ram
	BSS_END = .;
	__bss_end__ = .;
}

