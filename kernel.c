/*Edited by Vincent Wilson 4/16/22*/
#include <stdint.h>
#include <limits.h>


struct idt_entry {
	
	uint16_t base_low16;
	
	uint16_t selector;
	
	uint8_t always0;
	
	uint8_t access;
	
	uint16_t base_hi16;
	
} __attribute__((packed));

typedef struct idt_entry idt_entry_t;


struct idtr {
	
	uint16_t limit;
	
	uint32_t base;
	
} __attribute__((packed));

typedef struct idtr idtr_t;


struct pcb {
	
	uint32_t esp;
	
	int pid;
	
	uint32_t priority;
	
	struct pcb* next;
};

typedef struct pcb pcb_t;


struct pcbq {
	
	int size;
	
	pcb_t* head;
	
	pcb_t* tail;
};

typedef struct pcbq pcbq_t;


//variables and arrays
idt_entry_t idt[256];

pcb_t pcbs[5];

int num_pcbs;

uint32_t stacks[5][1024];

int num_stacks;

int pid;

pcbq_t q;

pcb_t* Running;


//assembly functions
extern void k_print(char* string, int string_length, int row, int col);

extern void lidtr(idtr_t* idtr);

extern void go();

extern void dispatch();

extern void outportb(uint16_t port, uint8_t value);

extern void init_timer_dev(int ms);


//c functions
void k_clearscr();

void print_border(int start_row, int start_col, int end_row, int end_col);

void init_idt_entry(idt_entry_t* entry, uint32_t base, uint16_t selector, uint8_t access);

void init_idt();

void default_handler();

int create_process(uint32_t processEntry, uint32_t priority);

uint32_t* allocate_stack();

pcb_t* allocatePCB();

void setup_PIC();


//queue functions
void init_q(pcbq_t* q);

void enqueue(pcbq_t* q, pcb_t* pcb);

pcb_t* dequeue(pcbq_t* q);

void enqueue_priority(pcbq_t* q, pcb_t* pcb);


//process prototypes
void p1();

void p2();

void p3();

void idle();

//-------------------------------------------------------------------------------------------------
int main() {
	
	k_clearscr();
	
	print_border(0, 0, 24, 79);
	
	k_print("Running processes...", 20, 1, 1);
	
	//turn off interrupts
	asm("cli");
	
	//initialize interrupt descriptor table
	init_idt();
	
	//initialize queue
	init_q(&q);
	
	//initialize timer device (50ms)
	init_timer_dev(50);
	
	//set up PIC
	setup_PIC();
	
	//Process Idle
	int retVal = create_process((uint32_t)idle, (uint32_t)5);
	
	if (retVal < 0) {
		
		default_handler();
	}
	
	//Process 1
	retVal = create_process((uint32_t)p1, (uint32_t)10);
	
	if (retVal < 0) {
		
		default_handler();
	}
	
	//Process 2
	retVal = create_process((uint32_t)p2, (uint32_t)10);
	
	if (retVal < 0) {
		
		default_handler();
	}
	
	//Process 3
	retVal = create_process((uint32_t)p3, (uint32_t)12);
	
	if (retVal < 0) {
		
		default_handler();
	}
	
	
	go();
	
	
	//while(1) {
		
		
	//}
	
	return 0;
}
//-------------------------------------------------------------------------------------------------

void k_clearscr() {
	
	for (int i = 0; i < 25; i++) {
		
		for (int j = 0; j < 80; j++) {
			
			k_print(" ", 1, i, j);
		}
	}
}

void print_border(int start_row, int start_col, int end_row, int end_col) {
	
	//corners of border
	k_print("+", 1, start_row, start_col);
	
	k_print("+", 1, start_row, end_col);
	
	k_print("+", 1, end_row, start_col);
	
	k_print("+", 1, end_row, end_col);
	
	//top and bottom of border
	for (int i = 1; i < end_col - start_col; i++) {
		
		k_print("-", 1, start_row, i + start_col);
		
		k_print("-", 1, end_row, i + start_col);
	}
	
	//sides of border
	for (int i = 1; i < end_row - start_row; i++) {
		
		k_print("|", 1, i + start_row, start_col);
		
		k_print("|", 1, i + start_row, end_col);
	}
}

void init_idt_entry(idt_entry_t* entry, uint32_t base, uint16_t selector, uint8_t access) {
	
	entry->base_low16 = (uint16_t)(base & 0x0000FFFF);
	
	entry->base_hi16 = (uint32_t)(0x0000FFFF & (base >> 16));
	
	entry->selector = selector;
	
	entry->access = access;
	
	entry->always0 = 0;
}

void init_idt() {
	
	//entries 0 to 31
	for (int i = 0; i <= 31; i++) {
		
		init_idt_entry(&idt[i], (uint32_t)default_handler, (uint16_t)16, (uint8_t)0x8e);
	}
	
	//entry 32 (dispatcher function)
	init_idt_entry(&idt[32], (uint32_t)dispatch, (uint16_t)16, (uint8_t)0x8e);
	
	//entries 33 to 255
	for (int i = 33; i <= 255; i++) {
		
		init_idt_entry(&idt[i], 0, 0, 0);
	}
	
	idtr_t idtr;
	
	idtr.limit = (uint16_t)((sizeof(idt)) - 1);
	
	idtr.base = (uint32_t)idt;
	
	lidtr(&idtr);
}

void default_handler() {
	
	k_clearscr();
	
	k_print("ERROR", 5, 0, 0);
	
	while (1) {
		
		
	}
}

//finish this
int create_process(uint32_t processEntry, uint32_t priority) {
	
	if (num_pcbs >= 5 || num_stacks >= 5) {
		
		return 1;
	}
	
	uint32_t* stackptr = allocate_stack();
	
	uint32_t* st = stackptr + 1024;
	
	//push the address of go onto the stack
	st--;
	
	*st = (uint32_t)go;
	
	//Eflags with interrupts disabled
	st--;
	
	*st = 0x200;
	
	//cs
	st--;
	
	*st = 16;
	
	//address of process (first instruction)
	st--;
	
	*st = processEntry;
	
	//ebp
	st--;
	
	*st = 0;
	
	//esp
	st--;
	
	*st = 0;
	
	//edi
	st--;
	
	*st = 0;
	
	//esi
	st--;

	*st = 0;
	
	//edx
	st--;

	*st = 0;
	
	//ecx
	st--;

	*st = 0;
	
	//ebx
	st--;

	*st = 0;
	
	//eax
	st--;

	*st = 0;
	
	//ds
	st--;

	*st = 8;
	
	//es
	st--;

	*st = 8;
	
	//fs
	st--;

	*st = 8;
	
	//gs
	st--;

	*st = 8;
	
	
	num_stacks++;
	
	
	pcb_t* pcb = allocatePCB();
	
	pcb->esp = (uint32_t)st;
	
	pid++;
	
	pcb->pid = pid;
	
	pcb->priority = priority;
	
	pcb->next = 0;
	
	num_pcbs++;
	
	//enqueue(&q, pcb);
	
	enqueue_priority(&q, pcb);
	
	return 0;
}

uint32_t* allocate_stack() {
	
	return stacks[num_stacks];
}

pcb_t* allocatePCB() {
	
	return &pcbs[num_pcbs];
}

void init_q(pcbq_t* q) {
	
	q->size = 0;
	
	q->head = 0;
	
	q->tail = 0;
}

void enqueue(pcbq_t* q, pcb_t* pcb) {
	
	if (q->size == 0) {
		
		q->head = pcb;
		
		q->tail = pcb;
	}
	
	else {
		
		q->tail->next = pcb;
		
		q->tail = pcb;
	}
	
	q->size++;
}

pcb_t* dequeue(pcbq_t* q) {
	
	if (q->size == 0) {
		
		k_clearscr();
		
		k_print("Queue is empty!", 15, 0, 0);
		
		return 0;
	}
	
	pcb_t* temp = q->head;
	
	q->head = q->head->next;
	
	if (q->head == 0) {
		
		q->tail = 0;
	}
	
	q->size--;
	
	return temp;
}

void enqueue_priority(pcbq_t* q, pcb_t* pcb) {
	
	pcb_t* curr;
	
	if (q->head == 0 || pcb->priority > q->head->priority) {
		
		pcb->next = q->head;
		
		q->head = pcb;
		
		q->tail = pcb;
	}
	
	else {
		
		curr = q->head;
		
		while (curr->next != 0 && pcb->priority < curr->priority) {
			
			curr = curr->next;
		}
		
		if (curr->next == 0) {
			
			q->tail = pcb;
		}
		
		pcb->next = curr->next;
		
		curr->next = pcb;
	}
	
	q->size++;
}

void setup_PIC() {
	
	//set up cascading mode
	outportb(0x20, 0x11);
	
	outportb(0xA0, 0x11);
	
	outportb(0x21, 0x20);
	
	outportb(0xA1, 0x28);
	
	//Tell the master that he has a slave
	outportb(0x21, 0x04);
	outportb(0xA1, 0x02);
	
	//Enable 8086 mode
	outportb(0x21, 0x01);
	outportb(0xA1, 0x01);
	
	//Reset the IRQ masks
	outportb(0x21, 0x0);
	outportb(0xA1, 0x0);
	
	//Now, enable the clock IRQ only
	outportb(0x21, 0xFE);
	outportb(0xA1, 0xFF);
}

//PROCESSES
void p1() {
	
	int n = 0;
	
	k_print("Process 1 running...", 20, 5, 1);
	
	for(int i = 0; i <= INT_MAX / 1000; i++) {
		
		char* msg = "value:     ";
		
		//calculate ones, tens, and hundreds places
		int o = n % 10;
		int t = (n / 10) % 10;
		int h = (n / 100) % 10;
		
		msg[8] = h + '0';
		msg[9] = t + '0';
		msg[10] = o + '0';
		
		k_print(msg, 11, 6, 1);
		
		n = ((n + 1) % 500);
	}
}

void p2() {
	
	int n = 0;
	
	k_print("Process 2 running...", 20, 10, 1);
	
	for(int i = 0; i <= INT_MAX / 1000; i++) {
		
		char* msg = "value:     ";
		
		//calculate ones, tens, and hundreds places
		int o = n % 10;
		int t = (n / 10) % 10;
		int h = (n / 100) % 10;
		
		msg[8] = h + '0';
		msg[9] = t + '0';
		msg[10] = o + '0';
		
		k_print(msg, 11, 11, 1);
		
		n = ((n + 1) % 500);
	}
}

void p3() {
	
	int n = 0;
	
	k_print("Process 3 running...", 20, 15, 1);
	
	for(int i = 0; i <= INT_MAX / 1000; i++) {
		
		char* msg = "value:     ";
		
		//calculate ones, tens, and hundreds places
		int o = n % 10;
		int t = (n / 10) % 10;
		int h = (n / 100) % 10;
		
		msg[8] = h + '0';
		msg[9] = t + '0';
		msg[10] = o + '0';
		
		k_print(msg, 11, 16, 1);
		
		n = ((n + 1) % 500);
	}
}

void idle() {
	
	k_print("Process idle running...", 23, 24, 1);
	
	while (1) {
		
		k_print("/", 1, 24, 24);
		
		k_print("\\", 1, 24, 24);
	}
}
