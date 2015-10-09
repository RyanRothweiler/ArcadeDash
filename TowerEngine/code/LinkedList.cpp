#ifndef LINKEDLIST_CPP
#define LINKEDLIST_CPP

enum link_type
{
	GLTEXTURE,
	GLSQUARE,
	GLLINE
};

struct list_link
{
	link_type DataType;
	void *Data;
	list_link *NextLink;
};

struct list_head
{
	list_link *TopLink;
};


list_head *
CreateList()
{
	list_head *ListHead = (list_head *)AllocateTransientMemory(sizeof(list_head));
	return(ListHead);
}

//TODO Next: Try allocating a new link and testing that the data worked.
//TODO also I don't know how this will work in the next cycle, test that too. I'm not good at this whole memory allocation thing.

// void
// CreateLink()


// void
// PushIntoArray(array_list *List, void *Data)
// {
// 	List->List[5] = Data;
// }

#endif