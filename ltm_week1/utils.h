#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_CHAR_USERNAME 100
#define MAX_CHAR_PASSWORD 100
#define MAX_CHAR_EMAIL 100
#define MAX_CHAR_PHONE 11
#define MAX_ATTEMPTS 3
#define BLOCK_DURATION_MINUTES 10

// Struct lưu tài khoản người dùng
typedef struct Account {
    char username[MAX_CHAR_USERNAME];
    char password[MAX_CHAR_PASSWORD];
    char email[MAX_CHAR_EMAIL];
    char phone[MAX_CHAR_PHONE];
    int status; // 1: active, 0: blocked
    time_t blockedTime; // Thời điểm tài khoản bị khóa (0 nếu chưa bị khóa)
    char lastLoginDate[20]; 
    char lastLoginTime[20];
    int role; // 0: user, 1: admin
    struct Account *next;
} Account;

extern Account* loggedInUser; // Đánh dấu trạng thái log in

Account *createNewUser(const char *username, const char *password,
                       const char *email, const char *phone, int status);
void freeList(Account *head);
Account *findAccountByUsername(Account *head, const char *username);
void saveAccountToFile(const Account *account, const char *filename);
void saveAllAccounts(Account *head, const char *filename);
void inputNewAccount(Account **head);
Account* signIn(Account* head, const char* filename);
void changePassword(Account* head, const char* filename);
void updateAccountInfo(Account* head, const char* filename);
void resetPassword(Account* head, const char* filename);
void saveLoginHistory(const char* username);
void viewLoginHistory(Account* head, const char* filename);
bool checkAndUnblockAccount(Account* account, const char* filename, Account* head);
void viewAllAccounts(Account* head);
void deleteAccount(Account** head, const char* filename);
void adminResetPassword(Account* head, const char* filename);
void signOut();
Account* loadAccountsFromFile(const char* filename);

#endif