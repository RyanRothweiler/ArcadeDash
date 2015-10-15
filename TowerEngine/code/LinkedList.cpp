#ifndef LINKEDLIST_CPP
#define LINKEDLIST_CPP

//TODO Right now this only works with Transient memory. Would be good to make it work with any memory given.

//TODO find a way to be able to add a new type and it works automatically. Read following note
//NOTE when adding a new link type make sure and add it's size to the switch statement in CreateLink
enum link_type
{
	LINKTYPE_GLTEXTURE,
	LINKTYPE_GLSQUARE,
	LINKTYPE_GLLINE
};

struct list_link
{
	link_type DataType;
	void *Data;
	list_link *NextLink;
};

struct list_head
{
	list_link *BottomLink;
	list_link *TopLink;
	//NOTE this count does not include the list head;
	uint32 LinkCount;
};

list_link *
GetLink(list_head *Head, uint32 LinkNum)
{
	Assert(LinkNum >= 1);

	list_link *CurrentLink = Head->TopLink;

	for (int LinkDepth = LinkNum - 1;
	     LinkDepth >= 0;
	     LinkDepth--)
	{
		if (LinkDepth == 0)
		{
			return (CurrentLink);
		}
		else
		{
			CurrentLink = CurrentLink->NextLink;
		}

	}

	//MOTE did not find the link, or there was some issue.
	Assert(0);
	return (NULL);
}

void *
GetLinkData(list_head *Head, uint32 LinkNum)
{
	return (GetLink(Head, LinkNum)->Data);
}

list_head *
CreateList(game_memory *GameMemory)
{
	list_head *ListHead = (list_head *)AllocateTransientMemory(GameMemory, sizeof(list_head));
	ListHead->LinkCount = 0;
	return (ListHead);
}

list_link *
CreateLink(list_head *Head, link_type Type, game_memory *GameMemory)
{
	list_link *NewLink = (list_link *)AllocateTransientMemory(GameMemory, sizeof(list_link));
	NewLink->DataType = Type;
	if (Head->LinkCount == 0)
	{
		Head->TopLink = NewLink;
	}

	uint32 DataSize = 0;
	switch (Type)
	{
		case LINKTYPE_GLTEXTURE:
		{
			DataSize = sizeof(gl_texture);
			break;
		}
		case LINKTYPE_GLSQUARE:
		{
			DataSize = sizeof(gl_square);
			break;
		}
		case LINKTYPE_GLLINE:
		{
			DataSize = sizeof(gl_line);
			break;
		}
	}

	//NOTE if asserted here then make sure you add the new link_type into this switch
	Assert(DataSize > 0);
	NewLink->Data = AllocateTransientMemory(GameMemory, DataSize);

	if (Head->LinkCount != 0)
	{
		list_link *LastLink = GetLink(Head, Head->LinkCount);
		LastLink->NextLink = NewLink;
	}

	Head->BottomLink = NewLink;
	Head->LinkCount++;

	return (NewLink);
}

#endif