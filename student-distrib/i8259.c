/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
	// Mask out all interrupts on the PIC for both Master and Slave
	outb(CLEAR_MASK, MASTER_DATA_PORT);
	outb(CLEAR_MASK, SLAVE_DATA_PORT);
	
	// Starts the init sequence in cascade mode
	outb(ICW1, MASTER_8259_PORT);	// outb(val,port), 
	outb(ICW1, SLAVE_8259_PORT);

	// Master PIC vect offset, ICW2
	outb(ICW2_MASTER, MASTER_DATA_PORT);

	// Slave PIC vect offset, ICW2
	outb(ICW2_SLAVE, SLAVE_DATA_PORT);

	// Notify Master PIC there is a Slave PIC at IRQ2 (0000 0100) = 0x04, ICw3
	outb(ICW3_MASTER, MASTER_DATA_PORT);

	// Notify Slave PIC its cascade identity (0000 0010) = 0x02, ICW3
	outb(ICW3_SLAVE, SLAVE_DATA_PORT);

	// Give additional info about the environment for slave and master
	outb(ICW4, MASTER_DATA_PORT);
	outb(ICW4, SLAVE_DATA_PORT);


	// set the mask on the PIC for both Master and Slave
	outb(master_mask, MASTER_DATA_PORT);
	enable_irq(2);
	outb(slave_mask, SLAVE_DATA_PORT);

}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
	uint16_t port;

	if(irq_num < SLAVE_BOUND){
		port = MASTER_DATA_PORT;
		master_mask &= ~(1 << irq_num);
		outb(master_mask, port);
	}
	else{
		port = SLAVE_DATA_PORT;
		slave_mask &= ~(1 << (irq_num - SLAVE_BOUND));
		outb(slave_mask, port);
	}

}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
	uint16_t port;

	if(irq_num < SLAVE_BOUND){
		port = MASTER_DATA_PORT;
		master_mask |= (1 << irq_num);
		outb(master_mask, port);

	}
	else{
		port = SLAVE_DATA_PORT;
		slave_mask |= (1 << (irq_num - SLAVE_BOUND));
		outb(slave_mask, port);
	}

}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
	if(irq_num >= SLAVE_BOUND){													// means command came from slave PIC so issue to both slave and master PIC's
		outb( ((irq_num - SLAVE_BOUND) | EOI), SLAVE_8259_PORT);	
		outb( (EOI | ICW3_SLAVE) , MASTER_8259_PORT);			
	}
	else																			// otherwise came from Master PIC so issue command only to master PIC
	{
		outb( (irq_num | EOI) , MASTER_8259_PORT);
	}

}
