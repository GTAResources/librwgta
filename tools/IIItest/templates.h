#ifndef _TEMPLATES_H_
#define _TEMPLATES_H_

template<typename T, int n>
class CStore
{
public:
	int allocPtr;
	T store[n];

	T *alloc(void){
		if(this->allocPtr >= n)
			printf("Size of this thing:%d needs increasing\n", n);
		return &this->store[this->allocPtr++];
	}
	void clear(void){
		this->allocPtr = 0;
	}
};

template<typename T, typename U>
class CPool
{
	U     *m_entries;
	union Flags {
		struct {
			uchar id   : 7;
			uchar free : 1;
		};
			uchar u;
	}     *m_flags;
	int    m_size;
	int    m_allocPtr;

public:
	CPool(int size){
		m_entries = (U*)malloc(sizeof(U)*size);
		m_flags = (Flags*)malloc(sizeof(Flags)*size);
		m_size = size;
		m_allocPtr = 0;
		for(int i = 0; i < size; i++){
			m_flags[i].id   = 0;
			m_flags[i].free = 1;
		}
	}
	int GetSize(void) { return m_size; }
	T *New(void){
		bool wrapped = false;
		do
			if(++m_allocPtr == m_size){
				if(wrapped)
					return nil;
				wrapped = true;
				m_allocPtr = 0;
			}
		while(!m_flags[m_allocPtr].free);
		m_flags[m_allocPtr].free = 0;
		m_flags[m_allocPtr].id++;
		return (T*)&m_entries[m_allocPtr];
	}
	T *New(int handle){
		T *entry = (T*)m_entries[handle>>8];
		SetNotFreeAt(handle);
		return entry;
	}
	void SetNotFreeAt(int handle){
		int idx = handle>>8;
		m_flags[idx].free = 0;
		m_flags[idx].id = handle & 0x7F;
		for(m_allocPtr = 0; m_allocPtr < m_size; m_allocPtr++)
			if(m_flags[m_allocPtr].free)
				return;
	}
	void Delete(T *entry){
		int i = GetJustIndex(entry);
		m_flags[i].free = 1;
		if(i < m_allocPtr)
			m_allocPtr = i;
	}
	T *GetSlot(int i){
		return m_flags[i].free ? nil : (T*)&m_entries[i];
	}
	T *GetAt(int handle){
		return m_flags[handle>>8].u == handle & 0xFF ?
		       (T*)&m_entries[handle>>8] : nil;
	}
	int GetIndex(T *entry){
		int i = GetJustIndex(entry);
		return m_flags[i].u + (i<<8);
	}
	int GetJustIndex(T *entry){
		// TODO: the cast is unsafe
		return (int)((U*)entry - m_entries);
	}
	int GetNoOfUsedSpaces(void){
		int i;
		int n = 0;
		for(i = 0; i < m_size; i++)
			if(!m_flags[i].free)
				n++;
		return n;
	}
};

template<typename T>
class CLink
{
public:
	T item;
	CLink<T> *prev;
	CLink<T> *next;

	void Insert(CLink<T> *link){
		link->next = this->next;
		this->next->prev = link;
		link->prev = this;
		this->next = link;
	}
	void Remove(void){
		this->prev->next = this->next;
		this->next->prev = this->prev;
	}
};

template<typename T>
class CLinkList
{
public:
	CLink<T> head, tail;
	CLink<T> freeHead, freeTail;
	CLink<T> *links;

	void Init(int n){
		links = new CLink<T>[n];
		head.next = &tail;
		tail.prev = &head;
		freeHead.next = &freeTail;
		freeTail.prev = &freeHead;
		while(n--)
			freeHead.Insert(&links[n]);
	}
	// Shutdown
	void Clear(void){
		while(head.next != &tail)
			Remove(head.next);
	}
	CLink<T> *Insert(T const &item){
		CLink<T> *node = freeHead.next;
		if(node == &freeTail)
			return nil;
		node->item = item;
		node->Remove();		// remove from free list
		head.Insert(node);
		return node;
	}
	CLink<T> *InsertSorted(T const &item){
		CLink<T> *sort;
		for(sort = head.next; sort != &tail; sort = sort->next)
			if(sort->item.sort >= item.sort)
				break;
		CLink<T> *node = freeHead.next;
		if(node == &freeTail)
			return nil;
		node->item = item;
		node->Remove();		// remove from free list
		sort->prev->Insert(node);
		return node;
	}
	void Remove(CLink<T> *link){
		link->Remove();		// remove from list
		freeHead.Insert(link);	// insert into free list
	}
	int Count(void){
		int n = 0;
		CLink<T> *lnk;
		for(lnk = head.next; lnk != &tail; lnk = lnk->next)
			n++;
		return n;
	}
};

#endif
