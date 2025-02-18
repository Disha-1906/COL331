#include "types.h"
#include "defs.h"
#include "x86.h"
#include "mouse.h"
#include "traps.h"

// Wait until the mouse controller is ready for us to send a packet
void 
mousewait_send(void) 
{
    // Implement your code here
    while((inb(0x64) & 0x02) !=0 ){};
    return;
}

// Wait until the mouse controller has data for us to receive
void 
mousewait_recv(void) 
{
    // Implement your code here
    while((inb(0x64) & 0x01) ==0 ){};
    return;
}

// Send a one-byte command to the mouse controller, and wait for it
// to be properly acknowledged
void 
mousecmd(uchar cmd) 
{
    // Implement your code here
    mousewait_send();
    outb(0x64, 0xD4);
    mousewait_send();
    outb(0x60, cmd);
    mousewait_recv();
    uchar ack = inb(0x60);
    while(ack!=0xFA){
        mousewait_recv();
        ack = inb(0x60);
    }
    return;
}

void
mouseinit(void)
{
    // Implement your code here
    
    mousewait_send();
    outb(0x64, 0xA8);
    mousewait_send();
    outb(0x64, 0x20);
    mousewait_recv();
    uchar status = inb(0x60);
    status = status | 0x02;
    mousewait_send();
    outb(0x64,0x60);
    mousewait_send();
    outb(0x60,status);
    mousecmd(0xF6);
    mousecmd(0xF4);
    ioapicenable(IRQ_MOUSE,0);
    cprintf("Mouse has been initialized\n");
    return;
}

void
mouseintr(void)
{
    // Implement your code here
    mousewait_recv();
    while((inb(0x64) & 0x01) != 0){
        mousewait_recv();
        uchar status = inb(0x64);
        uchar click = inb(0x60);
        mousewait_recv();
        inb(0x60);
        mousewait_recv();
        inb(0x60);
        if((status & 0x01)){
            if((click & 0x01) !=0 ) cprintf("LEFT\n");
            else if((click & 0x02) !=0) cprintf("RIGHT\n");
            else if((click & 0x04) !=0) cprintf("MID\n");

        }
        mousewait_recv();
    }
    return;
}