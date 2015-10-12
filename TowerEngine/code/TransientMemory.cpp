#ifndef TRANSIENTMEMORY_CPP
#define TRANSIENTMEMORY_CPP

//NOTE maybe allocate is the wrong descriptor. It doesn't allocate using any os calls. It's more of an internal allocate. Which is confusing.
void *
AllocateTransientMemory(game_memory *Memory, uint32 Size)
{
	void *CurrMemoryPos = Memory->TransientMemoryHead;
	Memory->TransientMemoryHead += Size;
	return (CurrMemoryPos);
}

#endif