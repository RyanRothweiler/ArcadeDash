#ifndef ARRAYLIST_CPP
#define ARRAYLIST_CPP

union list_types
{
	gl_texture GLTexture;
};

struct array_list
{
	uint16 Length;
	void *List;
};

void 
PushIntoArray(array_list *List, void *Data)
{
	List->List[5] = Data;
}

#endif