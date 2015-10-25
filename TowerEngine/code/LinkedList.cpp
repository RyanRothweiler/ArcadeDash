#ifndef LINKEDLIST_CPP
#define LINKEDLIST_CPP


//TODO find a way to be able to add a new type and it works automatically. Read following note
//NOTE when adding a new link type make sure and add it's size to the switch statement in CreateLink
enum link_type
{
	LinkTypeGLTexture,
	LinkTypeGLSquare,
	LinkTypeGLLine,
	LinkTypeEntity,
	LinkTypeWallCrawler,
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

	//NOTE did not find the link, or there was some issue.
	Assert(false);
	return (NULL);
}

void *
GetLinkData(list_head *Head, uint32 LinkNum)
{
	return (GetLink(Head, LinkNum)->Data);
}

list_head *
CreateList(memory_arena *Memory)
{
	list_head *ListHead = (list_head *)ArenaAllocate(Memory, sizeof(list_head));
	ListHead->LinkCount = 0;
	return (ListHead);
}

list_link *
AllocateLink(memory_arena *Memory, link_type Type)
{
	list_link *NewLink = (list_link *)ArenaAllocate(Memory, sizeof(list_link));
	NewLink->DataType = Type;

	uint32 DataSize = 0;
	switch (Type)
	{
		case LinkTypeGLTexture:
		{
			DataSize = sizeof(gl_texture);
			break;
		}
		case LinkTypeGLSquare:
		{
			DataSize = sizeof(gl_square);
			break;
		}
		case LinkTypeGLLine:
		{
			DataSize = sizeof(gl_line);
			break;
		}
		case LinkTypeEntity:
		{
			DataSize = sizeof(active_entity);
			break;
		}
		case LinkTypeWallCrawler:
		{
			DataSize = sizeof(wall_crawler);
		}
	}

	AssertM(DataSize > 0, "Make sure you add the new link_type into the above switch");
	NewLink->Data = ArenaAllocate(Memory, DataSize);

	return (NewLink);
}

list_link *
CreateLink(list_head *Head, link_type Type, memory_arena *Memory)
{
	list_link *NewLink = AllocateLink(Memory, Type);

	if (Head->LinkCount != 0)
	{
		list_link *LastLink = GetLink(Head, Head->LinkCount);
		LastLink->NextLink = NewLink;
	}
	else
	{
		Head->TopLink = NewLink;
	}

	Head->BottomLink = NewLink;
	Head->LinkCount++;

	return (NewLink);
}

//TODO look over this and clean it up. It looks messy
list_link *
CreateLink(list_head *Head, link_type Type, uint32 InsertionIndex, memory_arena *Memory)
{
	Assert(InsertionIndex >= 1);

	if (InsertionIndex == 1 && Head->LinkCount == 0)
	{
		list_link *NewLink = AllocateLink(Memory, Type);
		Head->TopLink = NewLink;
		Head->BottomLink = NewLink;

		Head->LinkCount++;
		return (NewLink);
	}

	if (InsertionIndex == 1)
	{
		list_link *NewLink = AllocateLink(Memory, Type);
		list_link *CurrentHead = Head->TopLink;
		Head->TopLink = NewLink;
		Head->TopLink->NextLink = CurrentHead;

		Head->LinkCount++;
		return (NewLink);
	}

	AssertM(InsertionIndex <= Head->LinkCount, "This checks that the place asserting is valid");
	Assert(Head->LinkCount > 0);

	if (InsertionIndex == Head->LinkCount)
	{
		list_link *NewLink = CreateLink(Head, Type, Memory);
		return (NewLink);
	}

	list_link *NewLink = AllocateLink(Memory, Type);

	list_link *ForwardLink = {};
	if (InsertionIndex != 1)
	{
		ForwardLink = GetLink(Head, InsertionIndex - 1);
	}
	if (InsertionIndex != Head->LinkCount)
	{
		list_link *BackLink = GetLink(Head, InsertionIndex);
		NewLink->NextLink = BackLink;
	}
	if (ForwardLink != NULL)
	{
		ForwardLink->NextLink = NewLink;
	}
	else
	{
		Head->TopLink = NewLink;
	}

	Head->LinkCount++;
	return (NewLink);
}

void
RemoveLink(list_head *Head, uint32 IndexRemoving)
{
	Assert(IndexRemoving >= 1);

	if (IndexRemoving == 1)
	{
		//TODO free this link
		list_link *LinkRemoving = Head->TopLink;
		Head->TopLink = Head->TopLink->NextLink;
		Head->LinkCount--;
		return;
	}

	uint32 CurrentLinkIndex = 2;
	list_link *CurrentLink = Head->TopLink;

	while (CurrentLink->NextLink)
	{
		if (IndexRemoving == CurrentLinkIndex)
		{
			//TODO free this link
			list_link *LinkRemoving = CurrentLink->NextLink;

			if (LinkRemoving->NextLink == NULL)
			{
				CurrentLink->NextLink = NULL;
			}
			else
			{
				CurrentLink->NextLink = CurrentLink->NextLink->NextLink;
			}

			Head->LinkCount--;
			return;
		}

		CurrentLinkIndex++;
		CurrentLink = CurrentLink->NextLink;
	}
}


//NOTE this doesn't check for equivalency, only if the pointers are the same
void
RemoveLink(list_head *Head, void *DataRemoving)
{
	if (Head->TopLink->Data == DataRemoving)
	{
		//TODO free this link
		list_link *LinkRemoving = Head->TopLink;
		Head->TopLink = Head->TopLink->NextLink;
		Head->LinkCount--;
		return;
	}

	list_link *CurrentLink = Head->TopLink;
	while (CurrentLink->NextLink)
	{
		if (CurrentLink->NextLink->Data == DataRemoving)
		{
			//TODO free this link
			list_link *LinkRemoving = CurrentLink->NextLink;

			if (LinkRemoving->NextLink == NULL)
			{
				CurrentLink->NextLink = NULL;
			}
			else
			{
				CurrentLink->NextLink = CurrentLink->NextLink->NextLink;
			}

			Head->LinkCount--;
			return;
		}

		CurrentLink = CurrentLink->NextLink;
	}
}

void
RemoveLink(list_head *Head, list_link *LinkRemoving)
{
	RemoveLink(Head, LinkRemoving->Data);
}

#endif