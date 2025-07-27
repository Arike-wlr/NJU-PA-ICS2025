/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next; // Pointer to the next watchpoint
  char expr[128];          // Expression to watch
  bool active;             // Whether the watchpoint is active
  word_t value;           // Current Value 
  word_t last_value;      // 表达式的上一个值
  int hit_count;           // Number of times triggered
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }                 //构建watchpoint 链表
  head = NULL;      //指向当前正在使用的监视点链表的头节点（即已经被分配并激活的监视点链表）
  free_ = wp_pool;  //指向空闲的监视点链表的头节点（即未被分配的监视点链表）
}

void create_watchpoint(bool *success, const char *exp) {
  if (free_ == NULL) {
    printf("No more watchpoints available.\n");
    *success = false; // Set success to false if no free watchpoints are available
    return;
  }

  WP *new_wp = free_;
  free_ = free_->next; // Move the free pointer to the next available watchpoint

  // Initialize the new watchpoint：
  new_wp->active = true;
  new_wp->hit_count = 0;
  strncpy(new_wp->expr, exp, sizeof(new_wp->expr) - 1);
  new_wp->expr[sizeof(new_wp->expr) - 1] = '\0'; // Ensure null termination
  new_wp->value = expr(new_wp->expr, success); // Evaluate the expression(之前已确保expr函数的正确性)
  new_wp->last_value = new_wp->value; // Initialize last_value

  new_wp->next = head;  // Add the new watchpoint to the head of the list
  head = new_wp;
  *success = true; // Set success to true if the watchpoint was created successfully
  printf("Watchpoint %d created for EXPR: %s\n", new_wp->NO, new_wp->expr);
}

void wp_function(bool *success) {
  for (WP *wp = head;wp != NULL;wp = wp->next) {// Iterate through the watchpoints
    if (wp->active) {
      word_t current_value = expr(wp->expr, success);
      if (!*success) {
        printf("Error evaluating watchpoint expression: %s\n", wp->expr);
        return;
      }
      if (current_value != wp->value) { // Check if the value has changed
        printf("Watchpoint %d triggered: %s changed from %u to %u\n", wp->NO, wp->expr, wp->last_value, current_value);
        wp->hit_count++;
        wp->last_value = wp->value; // Update last_value
        wp->value = current_value; // Update current value
      }
    }
  }
}

void display_wp() {
  if (head == NULL) {
    printf("No watchpoints set.\n");
    return;
  }

  printf("No.\tExpression\n");
  for (WP *wp = head; wp != NULL; wp = wp->next) {
    printf("%d\t%s\n", wp->NO, wp->expr); 
  }
}

void delete_watchpoint(int wp_num, bool *success) {
  if(head == NULL) {
    printf("No watchpoints to delete.\n");
    *success = false; // Set success to false if no watchpoints are available
    return;
  }
  for (WP *prev = NULL, *curr = head;curr != NULL;prev = curr,curr = curr->next) {
    if (curr->NO == wp_num) {
      if (prev == NULL) {
        head = curr->next; // If it's the first watchpoint
      } 
      else {
        prev->next = curr->next; // Bypass the current watchpoint
      }
      curr->next = free_; // Add it back to the free list
      free_ = curr;
      printf("Watchpoint %d deleted.\n", wp_num);
      return;
    }
  }

  printf("Watchpoint %d not found.\n", wp_num);
}