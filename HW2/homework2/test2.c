#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "smalloc.h"

typedef struct node
{
	int num;
	struct node *next;
} Node;

Node list;

int is_contained(int num)
{
	Node *itr = list.next; // 리스트의 첫 번째 노드부터 탐색 시작
	while (itr != 0x0)
	{
		if (itr->num == num)
			return 1;
		itr = itr->next;
	}
	return 0;
}

void insert_number(int num)
{
	if (is_contained(num))
		return;

	// 새로운 노드 동적 할당
	Node *n = (Node *)smalloc(sizeof(Node));
	n->num = num;		 // 새로운 노드의 데이터 설정
	n->next = list.next; // 새로운 노드의 다음 노드를 리스트의 첫 번째 노드로 설정
	list.next = n;		 // 리스트의 헤드 노드가 새로운 노드를 가리키도록 설정
}

void print_numbers()
{
	Node *itr = list.next;
	while (itr != 0x0)
	{
		printf("%d ", itr->num);
		itr = itr->next;
	}
	printf("\n");
}

void remove_number(int num)
{
	Node *itr;
	for (itr = &list; itr->next != 0x0; itr = itr->next)
	{
		if (itr->next->num == num)
		{
			Node *nextnext = itr->next->next; // 다다음 노드를 가리키는 포인터 설정
			sfree(itr->next);				  // 현재 노드의 다음 노드 메모리 해제
			itr->next = nextnext;			  // 현재 노드의 다음 노드를 다다음 노드로 연결
			break;
		}
	}
}

int main()
{
	int input;
	list.next = 0x0;

	while (1)
	{
		scanf("%d", &input);
		if (input == 0)
		{
			break;
		}
		else if (input > 0)
		{
			insert_number(input);
		}
		else /* input < 0 */
		{
			remove_number(input * -1); // 숫자의 절댓값을 리스트에서 삭제
		}
	}
	print_numbers();

	smdump();

	exit(0);
}
