#ifndef TRANSIENTMEMORY_CPP
#define TRANSIENTMEMORY_CPP

//NOTE maybe allocate is the wrong descriptor. It doesn't allocate using any os calls. It's more of an internal allocate. Which is confusing.
void *
ArenaAllocate(memory_arena *Memory, uint32 Size)
{
	void *CurrMemoryPos = Memory->Head;
	Memory->Head += Size;
	Assert(Memory->Head < Memory->EndOfMemory)
	return (CurrMemoryPos);
}

//TODO need a way to free memory... and manage my own memory... jeez.

#endif