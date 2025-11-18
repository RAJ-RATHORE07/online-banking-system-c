/* online_banking.c
   Simple Online Banking System in C
   Persistent storage: accounts.dat (binary)
   Compile: gcc online_banking.c -o online_banking
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DATAFILE "accounts.dat"
#define MAX_NAME 100
#define PASS_LEN 32

typedef struct {
    int acc_no;
    char name[MAX_NAME];
    char password[PASS_LEN];
    double balance;
    int active; /* 1 = active, 0 = deleted */
} Account;

/* Helper prototypes */
int get_last_account_number();
void pause_and_clear();
void create_account();
void list_accounts();
void view_account_by_accno();
void deposit_amount();
void withdraw_amount();
void transfer_amount();
void modify_account();
void delete_account();
Account *find_account_by_accno(int acc_no);
void save_account(Account *acc);
void update_account_in_file(Account *acc);
void admin_menu();
void user_menu(int acc_no);
int login_user();
void flush_input();

/* MAIN */
int main() {
    int choice;
    while (1) {
        printf("\n=== ONLINE BANKING SYSTEM ===\n");
        printf("1. Admin Login\n");
        printf("2. User Login\n");
        printf("3. Create Account\n");
        printf("4. Exit\n");
        printf("Select option: ");
        if (scanf("%d", &choice) != 1) { flush_input(); continue; }
        flush_input();
        switch (choice) {
            case 1:
                {
                    char admin_pass[PASS_LEN];
                    printf("Enter admin password: ");
                    if (fgets(admin_pass, PASS_LEN, stdin) == NULL) break;
                    admin_pass[strcspn(admin_pass, "\n")] = 0;
                    if (strcmp(admin_pass, "admin123") == 0) {
                        admin_menu();
                    } else {
                        printf("Wrong admin password.\n");
                    }
                }
                break;
            case 2:
                {
                    int acc = login_user();
                    if (acc > 0) user_menu(acc);
                }
                break;
            case 3:
                create_account();
                break;
            case 4:
                printf("Goodbye.\n");
                exit(0);
            default:
                printf("Invalid option.\n");
        }
    }
    return 0;
}

/* Utilities */

void flush_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

void pause_and_clear() {
    printf("\nPress Enter to continue...");
    getchar();
}

int get_last_account_number() {
    FILE *fp = fopen(DATAFILE, "rb");
    if (!fp) return 1000; /* start from 1001 */
    Account acc;
    int last = 1000;
    while (fread(&acc, sizeof(Account), 1, fp) == 1) {
        if (acc.acc_no > last) last = acc.acc_no;
    }
    fclose(fp);
    return last;
}

/* Find account in file and return a dynamically allocated Account (caller must free) */
Account *find_account_by_accno(int acc_no) {
    FILE *fp = fopen(DATAFILE, "rb");
    if (!fp) return NULL;
    Account *acc = NULL;
    Account tmp;
    while (fread(&tmp, sizeof(Account), 1, fp) == 1) {
        if (tmp.acc_no == acc_no && tmp.active) {
            acc = (Account *)malloc(sizeof(Account));
            if (acc) *acc = tmp;
            break;
        }
    }
    fclose(fp);
    return acc;
}

/* Save new account to file */
void save_account(Account *acc) {
    FILE *fp = fopen(DATAFILE, "ab");
    if (!fp) {
        perror("Unable to open data file for writing");
        return;
    }
    fwrite(acc, sizeof(Account), 1, fp);
    fclose(fp);
}

/* Update existing account in file (search by acc_no) */
void update_account_in_file(Account *acc) {
    FILE *fp = fopen(DATAFILE, "r+b");
    if (!fp) {
        perror("Unable to open data file for updating");
        return;
    }
    Account tmp;
    long pos;
    int found = 0;
    while (fread(&tmp, sizeof(Account), 1, fp) == 1) {
        if (tmp.acc_no == acc->acc_no) {
            found = 1;
            pos = ftell(fp) - sizeof(Account);
            fseek(fp, pos, SEEK_SET);
            fwrite(acc, sizeof(Account), 1, fp);
            break;
        }
    }
    fclose(fp);
    if (!found) {
        printf("Account not found for update.\n");
    }
}

/* Create account */
void create_account() {
    Account acc;
    memset(&acc, 0, sizeof(Account));
    int last = get_last_account_number();
    acc.acc_no = last + 1;
    printf("Creating account number: %d\n", acc.acc_no);

    printf("Enter full name: ");
    if (fgets(acc.name, MAX_NAME, stdin) == NULL) return;
    acc.name[strcspn(acc.name, "\n")] = 0;

    printf("Set password (max %d chars): ", PASS_LEN-1);
    if (fgets(acc.password, PASS_LEN, stdin) == NULL) return;
    acc.password[strcspn(acc.password, "\n")] = 0;

    double initial;
    printf("Initial deposit: ");
    if (scanf("%lf", &initial) != 1) { flush_input(); printf("Invalid amount.\n"); return; }
    flush_input();
    if (initial < 0) initial = 0.0;
    acc.balance = initial;
    acc.active = 1;

    save_account(&acc);
    printf("Account created successfully! Account No: %d\n", acc.acc_no);
}

/* List accounts (admin) */
void list_accounts() {
    FILE *fp = fopen(DATAFILE, "rb");
    if (!fp) {
        printf("No accounts found.\n");
        return;
    }
    Account acc;
    printf("\n%-8s %-25s %-12s\n", "AccNo", "Name", "Balance");
    printf("-------------------------------------------------\n");
    int any = 0;
    while (fread(&acc, sizeof(Account), 1, fp) == 1) {
        if (acc.active) {
            printf("%-8d %-25s Rs %.2f\n", acc.acc_no, acc.name, acc.balance);
            any = 1;
        }
    }
    if (!any) printf("No active accounts to show.\n");
    fclose(fp);
}

/* View a single account (admin) */
void view_account_by_accno() {
    int acc_no;
    printf("Enter account number: ");
    if (scanf("%d", &acc_no) != 1) { flush_input(); return; }
    flush_input();
    Account *acc = find_account_by_accno(acc_no);
    if (!acc) {
        printf("Account not found.\n");
        return;
    }
    printf("\nAccount No: %d\nName: %s\nBalance: Rs %.2f\n", acc->acc_no, acc->name, acc->balance);
    free(acc);
}

/* Deposit */
void deposit_amount() {
    int acc_no;
    printf("Enter account number: ");
    if (scanf("%d", &acc_no) != 1) { flush_input(); return; }
    flush_input();
    Account *acc = find_account_by_accno(acc_no);
    if (!acc) { printf("Account not found.\n"); return; }
    double amt;
    printf("Enter amount to deposit: ");
    if (scanf("%lf", &amt) != 1) { flush_input(); free(acc); return; }
    flush_input();
    if (amt <= 0) { printf("Invalid amount.\n"); free(acc); return; }
    acc->balance += amt;
    update_account_in_file(acc);
    printf("Deposit successful. New balance: Rs %.2f\n", acc->balance);
    free(acc);
}

/* Withdraw */
void withdraw_amount() {
    int acc_no;
    printf("Enter account number: ");
    if (scanf("%d", &acc_no) != 1) { flush_input(); return; }
    flush_input();
    Account *acc = find_account_by_accno(acc_no);
    if (!acc) { printf("Account not found.\n"); return; }
    double amt;
    printf("Enter amount to withdraw: ");
    if (scanf("%lf", &amt) != 1) { flush_input(); free(acc); return; }
    flush_input();
    if (amt <= 0) { printf("Invalid amount.\n"); free(acc); return; }
    if (amt > acc->balance) {
        printf("Insufficient funds. Current balance: Rs %.2f\n", acc->balance);
        free(acc);
        return;
    }
    acc->balance -= amt;
    update_account_in_file(acc);
    printf("Withdraw successful. New balance: Rs %.2f\n", acc->balance);
    free(acc);
}

/* Transfer */
void transfer_amount() {
    int from_acc, to_acc;
    printf("From account number: ");
    if (scanf("%d", &from_acc) != 1) { flush_input(); return; }
    printf("To account number: ");
    if (scanf("%d", &to_acc) != 1) { flush_input(); return; }
    flush_input();
    if (from_acc == to_acc) { printf("Cannot transfer to same account.\n"); return; }
    Account *src = find_account_by_accno(from_acc);
    Account *dst = find_account_by_accno(to_acc);
    if (!src || !dst) { printf("One or both accounts not found.\n"); free(src); free(dst); return; }
    double amt;
    printf("Amount to transfer: ");
    if (scanf("%lf", &amt) != 1) { flush_input(); free(src); free(dst); return; }
    flush_input();
    if (amt <= 0) { printf("Invalid amount.\n"); free(src); free(dst); return; }
    if (amt > src->balance) { printf("Insufficient funds in source account.\n"); free(src); free(dst); return; }
    src->balance -= amt;
    dst->balance += amt;
    update_account_in_file(src);
    update_account_in_file(dst);
    printf("Transfer successful. New balance of %d: Rs %.2f\n", src->acc_no, src->balance);
    printf("New balance of %d: Rs %.2f\n", dst->acc_no, dst->balance);
    free(src); free(dst);
}

/* Modify account (name/password) */
void modify_account() {
    int acc_no;
    printf("Enter account number to modify: ");
    if (scanf("%d", &acc_no) != 1) { flush_input(); return; }
    flush_input();
    Account *acc = find_account_by_accno(acc_no);
    if (!acc) { printf("Account not found.\n"); return; }
    printf("Current name: %s\n", acc->name);
    printf("Enter new name (leave blank to keep): ");
    char newname[MAX_NAME];
    if (fgets(newname, MAX_NAME, stdin) != NULL) {
        newname[strcspn(newname, "\n")] = 0;
        if (strlen(newname) > 0) strcpy(acc->name, newname);
    }
    printf("Change password? (y/n): ");
    char ch = getchar();
    flush_input();
    if (ch == 'y' || ch == 'Y') {
        char newpass[PASS_LEN];
        printf("Enter new password: ");
        if (fgets(newpass, PASS_LEN, stdin) != NULL) {
            newpass[strcspn(newpass, "\n")] = 0;
            if (strlen(newpass) > 0) strcpy(acc->password, newpass);
        }
    }
    update_account_in_file(acc);
    printf("Account updated.\n");
    free(acc);
}

/* Delete account (mark inactive) */
void delete_account() {
    int acc_no;
    printf("Enter account number to delete: ");
    if (scanf("%d", &acc_no) != 1) { flush_input(); return; }
    flush_input();
    Account *acc = find_account_by_accno(acc_no);
    if (!acc) { printf("Account not found.\n"); return; }
    printf("Are you sure you want to delete account %d (%s)? (y/n): ", acc->acc_no, acc->name);
    char c = getchar();
    flush_input();
    if (c == 'y' || c == 'Y') {
        acc->active = 0;
        update_account_in_file(acc);
        printf("Account deleted (soft delete).\n");
    } else {
        printf("Deletion cancelled.\n");
    }
    free(acc);
}

/* Admin menu */
void admin_menu() {
    int opt;
    while (1) {
        printf("\n--- ADMIN MENU ---\n");
        printf("1. Create Account\n");
        printf("2. List Accounts\n        ");
        printf("3. View Account\n");
        printf("4. Deposit\n");
        printf("5. Withdraw\n");
        printf("6. Transfer\n");
        printf("7. Modify Account\n");
        printf("8. Delete Account\n");
        printf("9. Back to Main Menu\n");
        printf("Choose: ");
        if (scanf("%d", &opt) != 1) { flush_input(); continue; }
        flush_input();
        switch (opt) {
            case 1: create_account(); break;
            case 2: list_accounts(); break;
            case 3: view_account_by_accno(); break;
            case 4: deposit_amount(); break;
            case 5: withdraw_amount(); break;
            case 6: transfer_amount(); break;
            case 7: modify_account(); break;
            case 8: delete_account(); break;
            case 9: return;
            default: printf("Invalid.\n");
        }
    }
}

/* Simple user login (returns acc_no on success, 0 on fail) */
int login_user() {
    int acc_no;
    char pass[PASS_LEN];
    printf("Account number: ");
    if (scanf("%d", &acc_no) != 1) { flush_input(); return 0; }
    flush_input();
    printf("Password: ");
    if (fgets(pass, PASS_LEN, stdin) == NULL) return 0;
    pass[strcspn(pass, "\n")] = 0;
    Account *acc = find_account_by_accno(acc_no);
    if (!acc) { printf("Account not found or deleted.\n"); return 0; }
    if (strcmp(acc->password, pass) == 0) {
        free(acc);
        printf("Login successful.\n");
        return acc_no;
    } else {
        printf("Login failed.\n");
        free(acc);
        return 0;
    }
}

/* User menu after login */
void user_menu(int acc_no) {
    int opt;
    while (1) {
        printf("\n--- USER MENU (Account %d) ---\n", acc_no);
        printf("1. View Details\n");
        printf("2. Deposit\n");
        printf("3. Withdraw\n");
        printf("4. Transfer to another account\n");
        printf("5. Change password / name\n");
        printf("6. Logout\n");
        printf("Choose: ");
        if (scanf("%d", &opt) != 1) { flush_input(); continue; }
        flush_input();
        Account *acc;
        switch (opt) {
            case 1:
                acc = find_account_by_accno(acc_no);
                if (acc) {
                    printf("\nAccount No: %d\nName: %s\nBalance: Rs %.2f\n", acc->acc_no, acc->name, acc->balance);
                    free(acc);
                } else printf("Account not found.\n");
                break;
            case 2:
                {
                    double amt;
                    printf("Amount to deposit: ");
                    if (scanf("%lf", &amt) != 1) { flush_input(); break; }
                    flush_input();
                    acc = find_account_by_accno(acc_no);
                    if (!acc) { printf("Account not found.\n"); break; }
                    if (amt <= 0) { printf("Invalid amount.\n"); free(acc); break; }
                    acc->balance += amt;
                    update_account_in_file(acc);
                    printf("Deposit successful. New balance: Rs %.2f\n", acc->balance);
                    free(acc);
                }
                break;
            case 3:
                {
                    double amt;
                    printf("Amount to withdraw: ");
                    if (scanf("%lf", &amt) != 1) { flush_input(); break; }
                    flush_input();
                    acc = find_account_by_accno(acc_no);
                    if (!acc) { printf("Account not found.\n"); break; }
                    if (amt <= 0) { printf("Invalid.\n"); free(acc); break; }
                    if (amt > acc->balance) { printf("Insufficient funds. Balance: Rs %.2f\n", acc->balance); free(acc); break; }
                    acc->balance -= amt;
                    update_account_in_file(acc);
                    printf("Withdraw successful. New balance: Rs %.2f\n", acc->balance);
                    free(acc);
                }
                break;
            case 4:
                transfer_amount();
                break;
            case 5:
                modify_account();
                break;
            case 6:
                printf("Logged out.\n");
                return;
            default:
                printf("Invalid.\n");
        }
    }
}
