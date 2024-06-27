#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include "smalloc.h"

// smheader 구조체와 smmode 열거형은 이미 smalloc.h에서 정의되었으므로 여기에서는 제거합니다.

smheader_ptr smlist = NULL;
void smdump()
{
   smheader_ptr itr;

   printf("==================== used memory slots ====================\n");
   int i = 0;
   for (itr = smlist; itr != 0x0; itr = itr->next)
   {
      if (itr->used == 0)
         continue;

      printf("%3d:%p:%8d:", i, ((void *)itr) + sizeof(smheader), (int)itr->size);

      int j;
      char *s = ((char *)itr) + sizeof(smheader);
      for (j = 0; j < (itr->size >= 8 ? 8 : itr->size); j++)
      {
         printf("%02x ", s[j]);
      }
      printf("\n");
      i++;
   }
   printf("\n");

   printf("==================== unused memory slots ====================\n");
   i = 0;
   for (itr = smlist; itr != 0x0; itr = itr->next, i++)
   {
      if (itr->used == 1)
         continue;

      printf("%3d:%p:%8d:", i, ((void *)itr) + sizeof(smheader), (int)itr->size);

      int j;
      char *s = ((char *)itr) + sizeof(smheader);
      for (j = 0; j < (itr->size >= 8 ? 8 : itr->size); j++)
      {
         printf("%02x ", s[j]);
      }
      printf("\n");
      i++;
   }
   printf("\n");
}

void *smalloc_mode(size_t s, smmode m)
{
   if (s == 0)
   {
      return NULL;
   }

   const size_t pageSize = 4096; // 페이지 전체 크기
   smheader_ptr itr = smlist, best = NULL, last = NULL;
   size_t best_size = (size_t)-1;

   // 사용 가능한 메모리 블록 탐색 및 할당
   while (itr)
   {
      if (!itr->used && itr->size >= s)
      {
         if (m == firstfit || (m == bestfit && itr->size < best_size) || (m == worstfit && itr->size > best_size))
         {
            best = itr;
            best_size = itr->size;
            if (m == firstfit)
               break;
         }
      }
      last = itr;
      itr = itr->next;
   }

   // 적절한 블록을 찾지 못한 경우 새 페이지 할당
   if (!best)
   {
      size_t totalSize = sizeof(smheader) + s;
      best = mmap(NULL, pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (best == MAP_FAILED)
      {
         perror("Memory allocation failed");
         return NULL;
      }
      best->size = s;
      best->used = 1;
      best->next = NULL;

      if (last)
      {
         last->next = best;
      }
      else
      {
         smlist = best;
      }

      // 남은 공간을 unused 블록으로 설정
      if (pageSize - totalSize >= sizeof(smheader))
      {
         smheader_ptr unused = (smheader_ptr)((char *)best + totalSize);
         unused->size = pageSize - totalSize;
         unused->used = 0;
         unused->next = NULL;
         best->next = unused;
      }
   }
   else
   {
      best->used = 1; // 선택된 블록을 used로 설정
      size_t remaining = best->size - s;
      if (remaining > sizeof(smheader))
      {
         smheader_ptr unused = (smheader_ptr)((char *)best + sizeof(smheader) + s);
         unused->size = remaining - sizeof(smheader);
         unused->used = 0;
         unused->next = best->next;
         best->next = unused;
      }
      best->size = s; // 할당된 블록의 크기를 조정
   }

   return (void *)((char *)best + sizeof(smheader));
}

void *smalloc(size_t s)
{
   void *ptr = smalloc_mode(s, firstfit);
   if (ptr == NULL)
   {
      fprintf(stderr, "Failed to allocate memory.\n");
      exit(EXIT_FAILURE);
   }
   return ptr;
}

void *srealloc(void *p, size_t s)
{
   if (!p)
      return smalloc_mode(s, firstfit);
   if (s == 0)
   {
      sfree(p);
      return NULL;
   }
   smheader_ptr header = (smheader_ptr)p - 1;
   if (header->size >= s)
      return p;
   void *new_block = smalloc_mode(s, firstfit);
   if (!new_block)
      return NULL;
   memcpy(new_block, p, header->size);
   sfree(p);
   return new_block;
}

void sfree(void *p)
{
   if (!p)
   {
      fprintf(stderr, "Attempt to free a null pointer.\n");
      return;
   }
   smheader_ptr header = (smheader_ptr)p - 1;
   if (header->used == 0)
   {
      fprintf(stderr, "Double free detected.\n");
      return;
   }

   // 로깅을 추가하여 어떤 주소가 해제되는지 확인
   printf("Freeing memory at %p, size %zu\n", p, header->size);

   header->used = 0; // 메모리 해제 상태로 설정
   if (munmap(header, header->size + sizeof(smheader)) == 0)
   {
      printf("Memory at %p successfully deallocated.\n", p);
      // smcoalesce(); // 메모리 해제 후 병합 시도
   }
   else
   {
      perror("Memory deallocation failed");
   }
}

void smcoalesce()
{
   smheader_ptr itr = smlist;
   // 반복문을 통해 리스트의 끝까지 순회
   while (itr != NULL && itr->next != NULL)
   {
      // 현재 블록이 사용 중이고 다음 블록이 사용되지 않을 경우
      if (itr->used == 1 && !itr->next->used)
      {
         // 두 블록의 크기를 합침
         itr->size += itr->next->size + sizeof(smheader);
         // 다음 블록을 건너뛰어 연결 (병합)
         itr->next = itr->next->next;
         // 병합 로그 출력
         printf("Merged used block at %p with unused next block. New size: %zu\n", itr, itr->size);
      }
      else
      {
         // 조건에 맞지 않으면 다음 블록으로 이동
         itr = itr->next;
      }
   }
}