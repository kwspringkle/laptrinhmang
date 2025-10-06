#include "utils.h"
Account* loggedInUser = NULL; // Đánh dấu trạng thái log in

//----------------------CHỨC NĂNG 1: TẠO TÀI KHOẢN---------------------
// Tạo 1 node để lưu thông tin người dùng mới
Account *createNewUser(const char *username, const char *password,
                       const char *email, const char *phone, int status) {
    Account *newUser = malloc(sizeof(Account));
    if (!newUser) return NULL;

    snprintf(newUser->username, MAX_CHAR_USERNAME, "%s", username);
    snprintf(newUser->password, MAX_CHAR_PASSWORD, "%s", password);
    snprintf(newUser->email, MAX_CHAR_EMAIL, "%s", email);
    snprintf(newUser->phone, MAX_CHAR_PHONE, "%s", phone);

    newUser->status = status;
    newUser->blockedTime = 0; 
    newUser->role = 0; // user default
    newUser->next = NULL;
    return newUser;
}

// Giải phóng list Account
void freeList(Account *head) {
    Account *current = head;
    while (current != NULL) {
        Account *temp = current;
        current = current->next;
        free(temp);
    }
}

// Tìm account theo Username
Account *findAccountByUsername(Account *head, const char *username) {
    Account *current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Lưu thông tin tài khoản ra file
void saveAccountToFile(const Account *account, const char *filename) {
    FILE *f = fopen(filename, "a"); 
    if (f == NULL) {
        printf("Error when opening file: %s\n", filename);
        return;
    }
    //Lưu thông tin trên cùng 1 dòng
    fprintf(f, "%s %s %s %s %d %ld %s\n",
            account->username,
            account->password,
            account->email,
            account->phone,
            account->status,
            (long)account->blockedTime,
            (account->role == 1) ? "admin" : "user");
    fclose(f);
}

// Lưu toàn bộ tài khoản ra file
void saveAllAccounts(Account *head, const char *filename) {
    FILE *f = fopen(filename, "w"); 
    if (f == NULL) {
        printf("Error when opening file: %s\n", filename);
        return;
    }
    Account *current = head;
    while (current != NULL) {
        fprintf(f, "%s %s %s %s %d %ld %s\n",
                current->username,
                current->password,
                current->email,
                current->phone,
                current->status,
                (long)current->blockedTime,
                (current->role == 1) ? "admin" : "user");
        current = current->next;
    }
    fclose(f);
}

// Lấy thông tin tài khoản từ bàn phím để tạo tài khoản
void inputNewAccount(Account **head) {
    char username[MAX_CHAR_USERNAME];
    char password[MAX_CHAR_PASSWORD];
    char email[MAX_CHAR_EMAIL];
    char phone[MAX_CHAR_PHONE];
    int status = 1;
    int role = 0; 
    
    // Phân biệt việc tạo tài khoản bởi ai
    if (loggedInUser == NULL) {
        printf("-----Register New Account-----\n");
    } else if (loggedInUser->role == 1) {
        printf("-----Admin: Create New Account-----\n");
    } else {
        printf("Please sign out to register new account.\n");
        return;
    }
    
    // Nhập username
    printf("Enter your username: ");
    fgets(username, MAX_CHAR_USERNAME, stdin);
    username[strcspn(username, "\n")] = '\0';

    if (findAccountByUsername(*head, username) != NULL) {
        printf("Username already exists.\n");
        return;
    }

    // Nhập password
    printf("Enter your password: ");
    fgets(password, MAX_CHAR_PASSWORD, stdin);
    password[strcspn(password, "\n")] = '\0';

    // Nhập email
    printf("Enter your email: ");
    fgets(email, MAX_CHAR_EMAIL, stdin);
    email[strcspn(email, "\n")] = '\0';

    // Nhập phone
    printf("Enter your phone number: ");
    fgets(phone, MAX_CHAR_PHONE, stdin);
    phone[strcspn(phone, "\n")] = '\0';

    // Chọn role (chỉ dành cho admin đang đăng nhập)
    if (loggedInUser != NULL && loggedInUser->role == 1) {
        int roleChoice;
        printf("Select role for new account:\n");
        printf("0. User\n");
        printf("1. Admin\n");
        printf("Enter your choice (0-1): ");
        scanf("%d", &roleChoice);
        getchar(); 
        
        if (roleChoice == 1) {
            role = 1;
            printf("Creating admin account...\n");
        } else {
            printf("Creating user account...\n");
        }
    }

    // Tạo node mới
    Account *newUser = createNewUser(username, password, email, phone, status);
    if (newUser == NULL) {
        printf("Error when allocating memory.\n");
        return;
    }
    
    newUser->role = role; // Set role

    // Thêm vào danh sách
    newUser->next = *head;
    *head = newUser;

    // Lưu ra file
    saveAccountToFile(newUser, "accounts.txt");

    if (loggedInUser == NULL) {
        // Người dùng bình thường đăng ký
        printf("Account registered successfully, please sign in!\n");
    } else {
        // Admin tạo tài khoản cho người khác
        printf("Account '%s' created successfully", username);
        if (role == 1) {
            printf(" with admin privileges.\n");
        }
    }
}


//----------------------CHỨC NĂNG 2: ĐĂNG NHẬP---------------------
Account* signIn(Account* head, const char* filename) {
    if(loggedInUser != NULL) {
        printf("User '%s' is already logged in. Please log out first.\n", loggedInUser->username);
        return loggedInUser;
    }
    char username[MAX_CHAR_USERNAME];
    char password[MAX_CHAR_PASSWORD];
	printf("-----Sign in-----\n");
    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    // Tìm account theo username
    Account* user = head;
    while (user != NULL) {
        if (strcmp(user->username, username) == 0) {
            // Kiểm tra trạng thái và tự động mở khóa nếu hết thời gian
            if (user->status == 0) {
                // Thử mở khóa tự động
                if (!checkAndUnblockAccount(user, filename, head)) {
                    return NULL; // Vẫn còn bị khóa
                }
                // Nếu mở khóa thành công, tiếp tục process đăng nhập
            }

            // Cho nhập password, tối đa 3 lần
            int attempts = 0;
            while (attempts < MAX_ATTEMPTS) {
                printf("Enter password: ");
                fgets(password, sizeof(password), stdin);
                password[strcspn(password, "\n")] = '\0';

                if (strcmp(user->password, password) == 0) {
                    printf("Welcome!\n");
                    printf("You are logged in as %s, role %s.\n", user->username,(user->role == 1) ? "Admin" : "User");
                    loggedInUser = user;
                    
                    // Lưu lịch sử đăng nhập
                    saveLoginHistory(user->username);
                    
					return user; // đăng nhập thành công
                } else {
                    attempts++;
                    printf("Incorrect password. Attempts left: %d\n", MAX_ATTEMPTS - attempts);
                }
            }

            // Sai quá 3 lần → khoá tài khoản
            user->status = 0;
            time(&user->blockedTime); // Lưu thời điểm khóa
            printf("Too many failed attempts. Your account is blocked for %d minutes.\n", BLOCK_DURATION_MINUTES);

            // Cập nhật file (lưu lại toàn bộ list)
            saveAllAccounts(head, filename);
            return NULL;
        }
        user = user->next;
    }

    // Không tìm thấy username
    printf("Username not found.\n");
    return NULL;
}


//----------------------CHỨC NĂNG 3: ĐỔI MẬT KHẨU---------------------
void changePassword(Account* head, const char* filename) {
    if (loggedInUser == NULL) {
        printf("Please log in to change your password.\n");
        return;
    }

    char oldPassword[MAX_CHAR_PASSWORD];
    char newPassword[MAX_CHAR_PASSWORD];
    char confirmPassword[MAX_CHAR_PASSWORD];
    int attempts = 0;

    printf("----- Change Password -----\n");

    // Nhập mật khẩu cũ (giới hạn số lần thử)
    while (attempts < MAX_ATTEMPTS) {
        printf("Enter your old password: ");
        fgets(oldPassword, sizeof(oldPassword), stdin);
        oldPassword[strcspn(oldPassword, "\n")] = '\0'; // xóa ký tự '\n'

        if (strcmp(loggedInUser->password, oldPassword) == 0) {
            // Nếu nhập đúng mật khẩu cũ thì cho đổi mật khẩu
            break;
        } else {
            attempts++;
            printf("Incorrect password. Attempts left: %d\n", MAX_ATTEMPTS - attempts);
        }
    }

    // Nếu nhập sai quá số lần cho phép
    if (attempts == MAX_ATTEMPTS) {
        printf("Too many failed attempts. Password change aborted.\n");
        return;
    }

    // Nhập mật khẩu mới + xác nhận
    printf("Enter your new password: ");
    fgets(newPassword, sizeof(newPassword), stdin);
    newPassword[strcspn(newPassword, "\n")] = '\0';

    printf("Confirm your new password: ");
    fgets(confirmPassword, sizeof(confirmPassword), stdin);
    confirmPassword[strcspn(confirmPassword, "\n")] = '\0';

    if (strcmp(newPassword, confirmPassword) == 0) {
        strcpy(loggedInUser->password, newPassword);
        saveAllAccounts(head, filename);
        printf("Password changed successfully!\n");
    } else {
        printf("Passwords do not match. Password change aborted.\n");
    }
}

//----------------------CHỨC NĂNG 4: UPDATE THÔNG TIN---------------------
void updateAccountInfo(Account* head, const char* filename) {
    if (loggedInUser == NULL) {
        printf("Please log in to update your account information.\n");
        return;
    }

    int choice;
    char newEmail[MAX_CHAR_EMAIL];
    char newPhone[MAX_CHAR_PHONE];

    printf("----- Update Account Information -----\n");
    printf("Current Email: %s\n", loggedInUser->email);
    printf("Current Phone: %s\n", loggedInUser->phone);
    printf("1. Update email\n");
    printf("2. Update phone number\n");
    printf("3. Update both\n");
    printf("Enter your choice: ");

    if (scanf("%d", &choice) != 1) {
        printf("Invalid input. Please enter a number.\n");
        while (getchar() != '\n'); // clear buffer
        return;
    }
    while (getchar() != '\n'); // clear buffer

    switch (choice) {
        case 1:
            printf("Enter your new email: ");
            fgets(newEmail, sizeof(newEmail), stdin);
            newEmail[strcspn(newEmail, "\n")] = '\0';

            if (strlen(newEmail) == 0) {
                printf("Email cannot be empty.\n");
                return;
            }
            if (strcmp(loggedInUser->email, newEmail) == 0) {
                printf("New email is the same as current email.\n");
                return;
            }

            strncpy(loggedInUser->email, newEmail, MAX_CHAR_EMAIL - 1);
            loggedInUser->email[MAX_CHAR_EMAIL - 1] = '\0';
            printf("Email updated successfully!\n");
            break;

        case 2:
            printf("Enter your new phone number: ");
            fgets(newPhone, sizeof(newPhone), stdin);
            newPhone[strcspn(newPhone, "\n")] = '\0';

            if (strlen(newPhone) == 0) {
                printf("Phone number cannot be empty.\n");
                return;
            }
            if (strcmp(loggedInUser->phone, newPhone) == 0) {
                printf("New phone number is the same as current phone number.\n");
                return;
            }

            strncpy(loggedInUser->phone, newPhone, MAX_CHAR_PHONE - 1);
            loggedInUser->phone[MAX_CHAR_PHONE - 1] = '\0';
            printf("Phone number updated successfully!\n");
            break;

        case 3:
            printf("Enter your new email: ");
            fgets(newEmail, sizeof(newEmail), stdin);
            newEmail[strcspn(newEmail, "\n")] = '\0';

            printf("Enter your new phone number: ");
            fgets(newPhone, sizeof(newPhone), stdin);
            newPhone[strcspn(newPhone, "\n")] = '\0';

            if (strlen(newEmail) == 0 || strlen(newPhone) == 0) {
                printf("Email and phone number cannot be empty.\n");
                return;
            }
            if (strcmp(loggedInUser->email, newEmail) == 0 &&
                strcmp(loggedInUser->phone, newPhone) == 0) {
                printf("New information is the same as current information.\n");
                return;
            }

            strncpy(loggedInUser->email, newEmail, MAX_CHAR_EMAIL - 1);
            loggedInUser->email[MAX_CHAR_EMAIL - 1] = '\0';
            strncpy(loggedInUser->phone, newPhone, MAX_CHAR_PHONE - 1);
            loggedInUser->phone[MAX_CHAR_PHONE - 1] = '\0';
            printf("Email and phone number updated successfully!\n");
            break;

        default:
            printf("Invalid choice.\n");
            return;
    }

    // Lưu thay đổi ra file
    saveAllAccounts(head, filename);
}

//----------------------CHỨC NĂNG 5: RESET PASSWORD---------------------
void resetPassword(Account* head, const char* filename){
	char username[MAX_CHAR_USERNAME];
	char verificationCode[10];
	char inputCode[10];
	char newPassword[MAX_CHAR_PASSWORD];
	
	printf("----- Reset Password -----\n");
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    Account* user = findAccountByUsername(head, username);
    if (user == NULL) {
        printf("Account not found.\n");
        return;
    }

    // Mã xác minh luôn luôn là "123456"
    snprintf(verificationCode, sizeof(verificationCode), "123456");
    printf("A verification code has been sent to your registered email/phone.\n");
    printf("Enter the verification code: ");
    fgets(inputCode, sizeof(inputCode), stdin);
    inputCode[strcspn(inputCode, "\n")] = '\0';
    if (strcmp(verificationCode, inputCode) != 0) {
        printf("Incorrect verification code.\n");
        return;
    }
    printf("Enter your new password: ");
    fgets(newPassword, sizeof(newPassword), stdin);
    newPassword[strcspn(newPassword, "\n")] = '\0';
    strcpy(user->password, newPassword);
    saveAllAccounts(head, filename);
    printf("Password has been reset successfully.\n");
}

//----------------------CHỨC NĂNG 6: XEM LỊCH SỬ ĐĂNG NHẬP---------------------
// Lưu lịch sử đăng nhập vào file history.txt
void saveLoginHistory(const char* username) {
    FILE *f = fopen("history.txt", "a"); 
    if (f == NULL) {
        printf("Error when opening history file.\n");
        return;
    }
    
    // Lấy thời gian hiện tại
    time_t now;
    struct tm *timeinfo;
    char dateStr[20], timeStr[20];
    time(&now);
    timeinfo = localtime(&now);
    
    // Format: dd/mm/yyyy
    strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", timeinfo);
    // Format: hh:mm:ss
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
    
    //Format: username | ngày | giờ
    fprintf(f, "%s | %s | %s\n", username, dateStr, timeStr);
    
    fclose(f);
}

//Xem lịch sử đăng nhập cho user hiện tại
void viewLoginHistory(Account* head, const char* filename) {
    if (loggedInUser == NULL) {
        printf("Please log in to view your login history.\n");
        return;
    }

    printf("----- Login History for %s -----\n", loggedInUser->username);
    
    FILE *f = fopen("history.txt", "r");
    if (f == NULL) {
        printf("No login history found.\n");
        return;
    }
    
    char line[256];
    char username[MAX_CHAR_USERNAME];
    char date[20];
    char time[20];
    bool hasHistory = false;
    
    // Đọc từng dòng và tìm các dòng thuộc về user hiện tại
    while (fgets(line, sizeof(line), f)) {
        // Parse dòng theo format: username | date | time
        if (sscanf(line, "%s | %s | %s", username, date, time) == 3) {
            if (strcmp(username, loggedInUser->username) == 0) {
                printf("Date: %s, Time: %s\n", date, time);
                hasHistory = true;
            }
        }
    }  
    if (!hasHistory) {
        printf("No login history found for this user.\n");
    }
    fclose(f);
}

//----------------------CHỨC NĂNG 7: QUẢN LÝ TRẠNG THÁI TÀI KHOẢN THEO THỜI GIAN---------------------

// Kiểm tra và mở khóa tài khoản nếu đã hết thời gian khóa
bool checkAndUnblockAccount(Account* account, const char* filename, Account* head) {
    if (account == NULL || account->status != 0) {
        return false; // Tài khoản không tồn tại hoặc không bị khóa
    }
    
    time_t currentTime;
    time(&currentTime);
    // Kiểm tra nếu đã qua thời gian khóa
    double timeDiff = difftime(currentTime, account->blockedTime);
    if (timeDiff >= (BLOCK_DURATION_MINUTES * 60)) {
        // Mở khóa tài khoản
        account->status = 1;
        account->blockedTime = 0;
        
        // Lưu lại thay đổi vào file
        saveAllAccounts(head, filename);
        
        printf("Your account has been automatically unblocked.\n");
        return true;
    } else {
        // Tính thời gian còn lại
        int remainingMinutes = (int)((BLOCK_DURATION_MINUTES * 60 - timeDiff) / 60) + 1;
        printf("Your account is blocked.\n");
        printf("Please try again in %d minute(s).\n", remainingMinutes);
        return false;
    }
}

//----------------------CHỨC NĂNG 8: PHÂN QUYỀN (ADMIN FUNCTIONS)---------------------

// Admin: Xem danh sách toàn bộ tài khoản
void viewAllAccounts(Account* head) {
    if (loggedInUser == NULL) {
        printf("Please log in as admin privileges to view all accounts.\n");
        return;
    }
    
    if (loggedInUser->role != 1) {
        printf("Access denied. Admin privileges required.\n");
        return;
    }
    
    printf("----- All User Accounts -----\n");
    printf("%-15s %-15s %-25s %-12s %-8s %-8s\n", 
           "Username", "Password", "Email", "Phone", "Status", "Role");
    printf("-------------------------------------------------------------------------------------\n");
    
    Account* current = head;
    int count = 0;
    while (current != NULL) {
        printf("%-15s %-15s %-25s %-12s %-8s %-8s\n",
               current->username,
               current->password,
               current->email,
               current->phone,
               (current->status == 1) ? "Active" : "Blocked",
               (current->role == 1) ? "Admin" : "User");
        current = current->next;
        count++;
    }
    printf("-------------------------------------------------------------------------------------\n");
    printf("Total accounts: %d\n", count);
}

// Admin: Xóa tài khoản khác
void deleteAccount(Account** head, const char* filename) {
    if (loggedInUser == NULL) {
        printf("Please log in as admin privileges to delete account.\n");
        return;
    }
    
    if (loggedInUser->role != 1) {
        printf("Access denied. Admin privileges required.\n");
        return;
    }
    
    char username[MAX_CHAR_USERNAME];
    printf("----- Delete Account -----\n");
    printf("Enter username to delete: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';
    
    // Không cho phép xóa chính mình
    if (strcmp(username, loggedInUser->username) == 0) {
        printf("Cannot delete your own account.\n");
        return;
    }
    
    Account* current = *head;
    Account* prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            // Tìm thấy user cần xóa
            char confirm[10];
            printf("Are you sure you want to delete account '%s'? (yes/no): ", username);
            fgets(confirm, sizeof(confirm), stdin);
            confirm[strcspn(confirm, "\n")] = '\0';
            
            if (strcmp(confirm, "yes") == 0 || strcmp(confirm, "y") == 0) {
                // Xóa khỏi linked list
                if (prev == NULL) {
                    *head = current->next;
                } else {
                    prev->next = current->next;
                }
                
                free(current);
                
                // Cập nhật file
                saveAllAccounts(*head, filename);
                printf("Account '%s' deleted successfully.\n", username);
            } else {
                printf("Account deletion cancelled.\n");
            }
            return;
        }
        prev = current;
        current = current->next;
    }
    
    printf("Account '%s' not found.\n", username);
}

// Admin: Reset mật khẩu cho user khác
void adminResetPassword(Account* head, const char* filename) {
    if (loggedInUser == NULL) {
        printf("Please log in as admin privileges to reset others' password.\n");
        return;
    }
    
    if (loggedInUser->role != 1) {
        printf("Access denied. Admin privileges required.\n");
        return;
    }
    
    char username[MAX_CHAR_USERNAME];
    char newPassword[MAX_CHAR_PASSWORD];
    
    printf("----- Admin Reset Password -----\n");
    printf("Enter username to reset password: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';
    
    Account* user = findAccountByUsername(head, username);
    if (user == NULL) {
        printf("Account not found.\n");
        return;
    }
    
    // Không cho phép reset password của chính mình mà phải sử dụng chức năng change password
    if (strcmp(username, loggedInUser->username) == 0) {
        printf("Please use 'change password' function instead.\n");
        return;
    }
    
    printf("Enter new password for '%s': ", username);
    fgets(newPassword, sizeof(newPassword), stdin);
    newPassword[strcspn(newPassword, "\n")] = '\0';
    
    char confirm[10];
    printf("Are you sure you want to reset password for '%s'? (yes/no): ", username);
    fgets(confirm, sizeof(confirm), stdin);
    confirm[strcspn(confirm, "\n")] = '\0';
    
    if (strcmp(confirm, "yes") == 0 || strcmp(confirm, "y") == 0) {
        strcpy(user->password, newPassword);
        // Mở khóa tài khoản nếu bị khóa
        user->status = 1;
        user->blockedTime = 0;
        
        saveAllAccounts(head, filename);
        printf("Password reset successfully for account '%s'.\n", username);
    } else {
        printf("Password reset cancelled.\n");
    }
}

//----------------------CHỨC NĂNG 9: ĐĂNG XUẤT---------------------
void signOut() {
    if (loggedInUser == NULL) {
        printf("No user is currently logged in.\n");
        return;
    }
    
    printf("User '%s' has been logged out successfully.\n", loggedInUser->username);
    loggedInUser = NULL; 
}

//----------------------KHÁC--------------------
// Đọc tài khoản từ file và tạo linked list
Account* loadAccountsFromFile(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        return NULL; 
    }
    
    Account* head = NULL;
    char line[512];
    
    while (fgets(line, sizeof(line), f)) {
        char username[MAX_CHAR_USERNAME];
        char password[MAX_CHAR_PASSWORD]; 
        char email[MAX_CHAR_EMAIL];
        char phone[MAX_CHAR_PHONE];
        int status;
        long blockedTime;
        char roleStr[10];
        
        // Parse dòng theo format: username password email phone status blockedTime role
        if (sscanf(line, "%s %s %s %s %d %ld %s", 
                   username, password, email, phone, &status, &blockedTime, roleStr) == 7) {
            
            Account* newUser = createNewUser(username, password, email, phone, status);
            if (newUser != NULL) {
                newUser->blockedTime = (time_t)blockedTime;
                // Convert string role về int
                newUser->role = (strcmp(roleStr, "admin") == 0) ? 1 : 0;
                
                // Thêm vào đầu list
                newUser->next = head;
                head = newUser;
            }
        }
    }
    
    fclose(f);
    return head;
}
