#include "utils.h"

// Hiển thị menu chính
void showMainMenu() {
    printf("\nUSER MANAGEMENT PROGRAM\n");
    printf("-----------------------------------\n");
    printf("1. Register\n");
    printf("2. Sign in\n");
    printf("3. Change password\n");
    printf("4. Update account info\n");
    printf("5. Reset password\n");
    printf("6. View login history\n");
    printf("7. Sign out\n");
    
    // Hiển thị thêm menu admin nếu là admin đã đăng nhập
    if (loggedInUser != NULL && loggedInUser->role == 1) {
        printf("----Other admin functions----\n");
        printf("8. Create account for others\n");
        printf("9. View all accounts\n");
        printf("10. Delete account\n");
        printf("11. Admin reset password\n");
        printf("Your choice (1-11, other to quit): ");
    } else {
        printf("Your choice (1-7, other to quit): ");
    }
}

// Xử lý menu chính
void handleMenu(Account** head, int choice) {
    if (loggedInUser == NULL || loggedInUser->role == 0) {
        // User thường chỉ được chọn 1-7
        if (choice < 1 || choice > 7) {
            exit(0);
        }
    }

    if (loggedInUser != NULL && loggedInUser->role == 1) {
        // Admin có thể chọn 1-11
        if (choice < 1 || choice > 11) {
            exit(0);
        }
    }
    switch (choice) {
        case 1: // Register
            inputNewAccount(head);
            break;
        case 2: // Sign in
            signIn(*head, "accounts.txt");
            break;
        case 3: // Change password
            changePassword(*head, "accounts.txt");
            break;
        case 4: // Update account info
            updateAccountInfo(*head, "accounts.txt");
            break;
        case 5: // Reset password
            resetPassword(*head, "accounts.txt");
            break;
        case 6: // View login history
            viewLoginHistory(*head, "accounts.txt");
            break;
        case 7: // Sign out
            signOut();
            break;
        case 8: // Admin: Create account for others
            inputNewAccount(head);
            break;
        case 9: // Admin: View all accounts
            viewAllAccounts(*head);
            break;
        case 10: // Admin: Delete account
            deleteAccount(head, "accounts.txt");
            break;
        case 11: // Admin: Reset password for others
            adminResetPassword(*head, "accounts.txt");
            break;
        default:
            exit(0);
            break;
    }
}

// Hàm khởi tạo tài khoản admin mặc định
void initializeDefaultAdmin(Account** head) {
    // Kiểm tra xem đã có admin
    Account* current = *head;
    while (current != NULL) {
        if (current->role == 1) {
            return; 
        }
        current = current->next;
    }
    
    // Tạo admin mặc định nếu chưa có
    printf("Creating default admin account...\n");
    Account* admin = createNewUser("admin", "admin123", "admin@gmail.com", "12345678", 1);
    if (admin != NULL) {
        admin->role = 1; 
        admin->next = *head;
        *head = admin;
        saveAccountToFile(admin, "accounts.txt");
        printf("Default admin created - Username: admin, Password: admin123\n");
    }
}

int main() {
    // Load dữ liệu từ file
    Account* head = loadAccountsFromFile("accounts.txt");
    
    // Khởi tạo admin mặc định nếu cần
    initializeDefaultAdmin(&head);
    
    int choice;
    while (1) {
        showMainMenu();
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            break;
        }
        getchar(); 
        
        handleMenu(&head, choice);
    }
    
    // Giải phóng memory
    freeList(head);
    return 0;
}