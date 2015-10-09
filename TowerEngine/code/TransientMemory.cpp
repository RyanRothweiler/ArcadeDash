#ifndef TRANSIENTMEM_CPP
#define TRANSIENTMEM_CPP

global_variable uint8 *TransientMemoryHead;

void *
AllocateTransientMemory(uint32 Size)
{
	void *CurrMemoryPos = TransientMemoryHead;
	TransientMemoryHead += Size;
	return (CurrMemoryPos);
}


#endif