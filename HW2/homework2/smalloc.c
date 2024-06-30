#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include "smalloc.h"
#include <unistd.h>
#define pageSize getpagesize() // 페이지 전체 크기

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

   smheader_ptr itr = smlist, best = NULL, last = NULL;
   // smlist는 메모리 블록의 시작
   // best는 최적의 메모리 블록을 찾는 포인터
   // last는 현재 확인 중인 블록의 이전 블록의 포인터

   size_t best_size = (m == bestfit) ? (size_t)-1 : 0;

   // 사용 가능한 메모리 블록 탐색
   while (itr)
   {
      if (!itr->used && itr->size >= s)
      {
         if (m == firstfit)
         { // First fit
            best = itr;
            break;
         }
         else if (m == bestfit && itr->size < best_size)
         { // Best fit
            best = itr;
            best_size = itr->size;
         }
         else if (m == worstfit && itr->size > best_size)
         { // Worst fit
            best = itr;
            best_size = itr->size;
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
      // 새로운 메모리 페이지 할당

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
      // 기존 블록을 할당
      best->used = 1;
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
   if (s == 0)
   {
      return NULL;
   }

   smheader_ptr itr = smlist, last = NULL, best = NULL;

   // 사용 가능한 메모리 블록 탐색 (firstfit)
   while (itr)
   {
      if (!itr->used && itr->size >= s)
      {
         best = itr;
         break;
      }
      last = itr;
      itr = itr->next;
   }

   if (!best)
   {
      size_t totalSize = sizeof(smheader) + s;
      int numPages = (totalSize + pageSize - 1) / pageSize; // 필요한 페이지 수 (올림 연산)
      size_t allocateSize = numPages * pageSize;

      best = mmap(NULL, allocateSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (best == MAP_FAILED)
      {
         perror("Memory allocation failed");
         exit(EXIT_FAILURE);
      }

      smheader_ptr current = best;
      size_t remainingSize = s;

      for (int i = 0; i < numPages; ++i)
      {
         size_t blockSize = (remainingSize > pageSize - sizeof(smheader)) ? pageSize - sizeof(smheader) : remainingSize;
         current->size = blockSize;
         current->used = 1;
         remainingSize -= blockSize;

         if (i < numPages - 1)
         {
            current->next = (smheader_ptr)((char *)current + pageSize);
            current = current->next;
         }
         else
         {
            current->next = NULL;
         }
      }

      // 남은 공간을 unused로 설정
      if (allocateSize - totalSize > sizeof(smheader))
      {
         smheader_ptr unused = (smheader_ptr)((char *)current + sizeof(smheader) + current->size);
         unused->size = allocateSize - totalSize - (numPages - 1) * sizeof(smheader);
         unused->used = 0;
         unused->next = NULL;
         current->next = unused;
      }

      if (last)
      {
         last->next = best;
      }
      else
      {
         smlist = best;
      }
   }
   else
   {
      best->used = 1;
      size_t remaining = best->size - s;
      if (remaining > sizeof(smheader))
      {
         smheader_ptr unused = (smheader_ptr)((char *)best + sizeof(smheader) + s);
         unused->size = remaining - sizeof(smheader);
         unused->used = 0;
         unused->next = best->next;
         best->next = unused;
      }
      best->size = s;
   }

   return (void *)((char *)best + sizeof(smheader));
}

void sfree(void *p)
{
   // NULL 포인터인 경우, 메모리를 해제할 수 없으므로 에러 메시지를 출력하고 함수를 종료합니다.
   if (!p)
   {
      fprintf(stderr, "Attempt to free a null pointer.\n");
      return;
   }

   // 주어진 포인터로부터 헤더를 계산합니다.
   // 메모리 블록의 헤더는 실제 데이터의 앞에 위치하므로, sizeof(smheader)만큼 빼줍니다.
   smheader_ptr header = (smheader_ptr)p - 1;

   // 이미 해제된 메모리인 경우, 이중 해제가 감지되었음을 알리는 에러 메시지를 출력하고 함수를 종료합니다.
   if (header->used == 0)
   {
      fprintf(stderr, "Double free detected.\n");
      return;
   }

   // 메모리 블록의 used 필드를 0으로 설정하여 해당 메모리가 해제되었음을 표시합니다.
   header->used = 0;

   // 해제된 메모리 블록 주변에 있는 다른 unused 블록과 병합을 시도합니다.
   // 이 기능은 메모리 관리의 최적화를 위해 사용됩니다.
}

void *srealloc(void *p, size_t s)
{
   // 포인터 p가 NULL이면 새로운 메모리를 할당하여 반환합니다.
   if (!p)
      return smalloc(s);

   // 요청한 크기가 0이면 현재 메모리를 해제하고 NULL을 반환합니다.
   if (s == 0)
   {
      sfree(p);
      return NULL;
   }

   // 현재 메모리 블록의 헤더를 찾습니다.
   smheader_ptr header = (smheader_ptr)p - 1;

   // 요청한 크기가 현재 크기와 같으면 현재 포인터를 그대로 반환합니다.
   if (header->size == s)
      return p;

   // 요청한 크기가 현재 크기보다 작으면 현재 메모리 블록을 조정합니다.
   else if (header->size > s)
   {
      // 남은 공간이 분할 가능한 최소 크기보다 크면, 현재 블록을 조정하고 남은 공간을 unused 블록으로 설정합니다.
      size_t remaining = header->size - s;
      if (remaining >= sizeof(smheader) + 1)
      {
         header->size = s; // 현재 블록의 크기를 조정합니다.
         smheader_ptr new_unused = (smheader_ptr)((char *)header + sizeof(smheader) + s);
         new_unused->size = remaining - sizeof(smheader);
         new_unused->used = 0;
         new_unused->next = header->next;
         header->next = new_unused;
      }
      return p;
   }

   // 인접한 블록이 있고, 사용되지 않으며, 현재 블록과 합칠 때 요청한 크기를 충족할 수 있으면 블록을 확장합니다.
   if (header->next && !header->next->used && (header->size + header->next->size + sizeof(smheader) >= s))
   {
      // 인접한 블록을 합칩니다.
      header->size += header->next->size + sizeof(smheader);
      header->next = header->next->next;

      // 합친 후에도 남은 공간이 있으면, 새로운 unused 블록을 생성합니다.
      if (header->size > s + sizeof(smheader))
      {
         smheader_ptr new_unused = (smheader_ptr)((char *)header + sizeof(smheader) + s);
         new_unused->size = header->size - s - sizeof(smheader);
         new_unused->used = 0;
         new_unused->next = header->next;
         header->next = new_unused;
         header->size = s; // 현재 블록의 크기를 조정합니다.
      }

      return p;
   }

   // 적절히 확장할 수 없는 경우, 새로운 메모리 공간을 할당하고 기존 데이터를 복사합니다.
   void *new_block = smalloc(s);
   if (!new_block)
      return NULL;

   // 기존 데이터를 새로운 메모리 공간으로 복사합니다.
   memcpy(new_block, p, header->size);
   // 기존 메모리를 해제합니다.
   sfree(p);
   // 새로운 메모리 공간을 반환합니다.
   return new_block;
}

void smcoalesce()
{
   smheader_ptr itr = smlist; // smlist의 첫 번째 블록을 가리키는 포인터 itr 선언

   // itr이 NULL이 아니고, itr의 다음 블록이 NULL이 아닐 때까지 반복
   while (itr != NULL && itr->next != NULL)
   {
      // 현재 블록의 페이지 번호를 계산하여 current_page 변수에 저장
      uintptr_t current_page = (uintptr_t)itr / pageSize;

      // 다음 블록의 페이지 번호를 계산하여 next_page 변수에 저장
      uintptr_t next_page = (uintptr_t)itr->next / pageSize;

      // 현재 블록과 다음 블록이 모두 사용되지 않고, 같은 페이지에 있을 경우
      if (!itr->used && !itr->next->used && current_page == next_page)
      {
         // 두 블록의 크기를 합친 후 현재 블록의 크기로 설정
         itr->size += itr->next->size + sizeof(smheader);

         // 현재 블록의 다음 블록을 두 칸 건너뛰어 연결하여 두 블록을 합침
         itr->next = itr->next->next;
      }
      else
      {
         itr = itr->next; // 다음 블록으로 이동
      }
   }
}
