/*
	linkedList.c

	Created on: 22.05.2017
	Author: Jasper Reichardt

	https://rextester.com/MHA94868
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "smalloc.h"

/********** GLOBALS *******************************/
#define OK 0
#define ERROR -1

/********** STRUCT AND TYPES DEFINTIONS ***********/
/* a node with key, data and reference to next node*/
typedef struct Node
{
	int key;
	char string[1024];
	struct Node *next; // pointer to next node
} Node;

/* the actual linked list: ref to first and last Node, size attribute */
typedef struct LinkedList
{
	struct Node *first;
	struct Node *last;
	int size;
} LinkedList;

LinkedList *init_list();										 // 연결 리스트를 초기화하는 함수
void insert_end(LinkedList *list, int key, char string[]);		 // 연결 리스트의 끝에 노드를 삽입하는 함수
void insert_beginning(LinkedList *list, int key, char string[]); // 연결 리스트의 시작에 노드를 삽입하는 함수
int remove_end(LinkedList *list);								 // 연결 리스트의 끝에 있는 노드를 제거하는 함수
int remove_beginning(LinkedList *list);							 // 연결 리스트의 시작에 있는 노드를 제거하는 함수
int print_list(LinkedList *list);								 // 연결 리스트의 내용을 출력하는 함수
void free_list(LinkedList *list);								 // 연결 리스트를 해제하는 함수
char *get_string(LinkedList *list, int key);					 // 주어진 키에 해당하는 노드의 데이터를 반환하는 함수

LinkedList *init_list()
{
	printf("initializing list...\n");

	LinkedList *list = (LinkedList *)smalloc(sizeof(LinkedList));

	list->first = NULL;
	list->last = NULL;
	list->size = 0;

	return list;
}

void insert_end(LinkedList *list, int key, char string[])
{
	printf("----------------------\n");

	list->size++; // increment size of list

	// intialize the new Node
	Node *newN = (Node *)smalloc(sizeof(Node));
	newN->key = key;			  // 키 설정
	strcpy(newN->string, string); // 데이터 복사
	newN->next = NULL;			  // 다음 노드 설정

	Node *oldLast = list->last; // get the old last
	oldLast->next = newN;		// make new Node the next Node for oldlast
	list->last = newN;			// set the new last  in the list

	printf("new Node(%p) at end: %d '%s' %p \n", newN, newN->key, newN->string, newN->next);
}

void insert_beginning(LinkedList *list, int key, char string[])
{
	printf("----------------------\n");

	list->size++;				  // increment size of list
	Node *oldFirst = list->first; // get the old first node

	/* intialize the new Node */
	Node *newN = (Node *)smalloc(sizeof(Node));
	newN->key = key;
	strcpy(newN->string, string);
	newN->next = oldFirst;

	list->first = newN; // set the new first

	/* special case: if list size == 1, then this new one is also the last one */
	if (list->size == 1)
		list->last = newN;

	printf("new Node(%p) at beginning: %d '%s' %p \n", newN, newN->key, newN->string, newN->next);
}

int remove_beginning(LinkedList *list)
{
	printf("----------------------\n");

	if (list->size <= 0)
		return ERROR;

	list->size--;

	Node *oldFirst = list->first;

	printf("delete Node(%p) at beginning: '%d' '%s' '%p' \n", oldFirst, oldFirst->key, oldFirst->string, oldFirst->next);

	sfree(list->first); // free it
	list->first = oldFirst->next;
	oldFirst = NULL;

	return OK;
}

int remove_end(LinkedList *list)
{
	printf("----------------------\n");

	/* special case #1 */
	if (list->size <= 0)
		return ERROR;

	/* special case #2 */
	if (list->size == 1)
	{
		sfree(list->first);
		list->first = NULL;
		list->last = NULL;
		return OK;
	}

	printf("delete Node(%p) at end: '%d' '%s' '%p' \n", list->last, list->last->key, list->last->string, list->last->next);

	list->size--; // decrement list size
	Node *startNode = list->first;

	/* find the new last node (the one before the old last one); list->size >= 2 at this point!*/
	Node *newLast = startNode;
	while (newLast->next->next != NULL)
	{
		newLast = newLast->next;
	}

	sfree(newLast->next); // free it
	newLast->next = NULL; // set to NULL to denote new end of list
	list->last = newLast; // set the new list->last

	return OK;
}

int print_list(LinkedList *list)
{

	printf("----------------------\n");

	if (list->size <= 0)
		return ERROR;

	printf("List.size = %d \n", list->size);

	Node *startN = list->first; // get first

	/* iterate through list and print contents */
	do
	{
		printf("Node#%d.string = '%s', .next = '%p' \n", startN->key, startN->string, startN->next);
		startN = startN->next;
	} while (startN != NULL);

	return OK;
}

void free_list(LinkedList *list)
{
	printf("----------------------\n");
	printf("freeing list...\n");

	if (list != NULL && list->size > 0)
	{
		Node *startN = list->first;
		Node *temp = list->first;

		do
		{
			sfree(temp);
			startN = startN->next;
			temp = startN;
		} while (startN != NULL);

		sfree(list);
	}
}

char *get_string(LinkedList *list, int key)
{
	printf("----------------------\n");

	Node *startN = list->first; // get first

	/* if only one node.. */
	if (list->size == 1)
		return startN->string;

	/* iterate through list and find Node where node->key == key */
	while (startN->next != NULL)
	{
		if (startN->key == key)
			return startN->string;
		else
			startN = startN->next;
	}

	return NULL;
}

int main(void)
{
	LinkedList *list = init_list();

	insert_beginning(list, 1, "im the first");
	insert_end(list, 2, "im the second");
	insert_end(list, 3, "im the third");
	insert_end(list, 4, "forth here");

	print_list(list);
	remove_end(list);
	print_list(list);
	remove_beginning(list);
	print_list(list);
	remove_end(list);
	print_list(list);
	printf("string at node with key %d = '%s' \n", 2, get_string(list, 2));
	free_list(list);

	return OK;
}
