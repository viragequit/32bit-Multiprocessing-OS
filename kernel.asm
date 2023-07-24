;Edited by Vincent Wilson 4/16/22

GLOBAL k_print
GLOBAL lidtr
GLOBAL go
GLOBAL dispatch
GLOBAL outportb
GLOBAL init_timer_dev

EXTERN q
EXTERN Running
EXTERN enqueue
EXTERN enqueue_priority
EXTERN dequeue


k_print:
	push ebp
	mov ebp, esp
	pushf
	push eax
	push ecx
	push ebx
	
	
	;string address
	mov esi, [ebp + 8]
	
	
	;string length
	mov ecx, [ebp + 12]
	
	
	;offset = (row * 80 + col) * 2
	mov edi, 2
	mov ebx, [ebp + 16]
	imul ebx, 80
	add ebx, [ebp + 20]
	imul ebx, edi
	
	
	;add video address and offset
	mov edi, 0xB8000
	add edi, ebx
	mov ebx, 0
	
_loop:
	cmp ecx, 0
	je _done
	cmp edi, 0xB8000 + 80 * 25 * 2
	je _done
	
	
	;printing code here
	movsb
	
	;color byte
	mov BYTE [edi], 0x0F
	
	inc edi
	dec ecx
	jmp _loop
	
_done:
	pop ebx
	pop ecx
	pop eax
	popf
	pop ebp
	ret


lidtr:
	push ebp
	mov ebp, esp
	pushf
	push eax
	
	mov eax, [ebp + 8]
	lidt [eax]
	
	pop eax
	popf
	pop ebp
	ret


go:
	push q
	
	call dequeue

	add esp, 4
	
	mov [Running], eax
	
	mov esp, [eax]
	
	pop gs
	pop fs
	pop es
	pop ds
	popad
	iret


dispatch:
	pushad
	push ds
	push es
	push fs
	push gs
	
	mov eax, [Running]
	mov [eax], esp
	
	push eax
	push q
	
	call enqueue_priority
	
	add esp, 8
	
	
	;below is same as go
	push q
	
	call dequeue

	add esp, 4
	
	mov [Running], eax
	
	mov esp, [eax]
	
	pop gs
	pop fs
	pop es
	pop ds
	popad
	
	push eax
	
	;send EOI to PIC
	mov al, 0x20
	
	out 0x20, al
	
	pop eax
	
	iret


outportb:
	push ebp
	mov ebp, esp
	pushf
	push eax
	push edx
	
	mov edx, [ebp + 8]
	
	mov eax, [ebp + 12]
	
	out dx, al
	
	pop edx
	pop eax
	popf
	pop ebp
	ret


init_timer_dev:
	push ebp
	mov ebp, esp
	pushf
	push edx
	
	mov edx, [ebp + 8]
	
	imul dx, 1193
	
	mov al, 0b00110110
	
	out 0x43, al
	
	mov ax, dx
	
	out 0x40, al
	
	xchg ah, al
	
	out 0x40, al
	
	pop edx
	popf
	pop ebp
	ret
