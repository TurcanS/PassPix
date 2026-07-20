#include "../Include/lodepng.h"
#include "image_utils.h"
#include <iostream>
#include <vector>
#include <string>
#include <limits>

using namespace std;

void showMenu();

int main() {
    cout << "PassPix" << endl;
    cout << string(55, '=') << endl;
    
    while (true) {
        showMenu();
        
        int choice;
        cin >> choice;
        
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number." << endl;
            continue;
        }
        cin.ignore();
        
        if (choice == 1) {
            string masterPassphrase, masterConfirm, password;
            
            cout << "Enter master passphrase: ";
            getline(cin, masterPassphrase);
            cout << "Confirm master passphrase: ";
            getline(cin, masterConfirm);
            
            if (masterPassphrase != masterConfirm) {
                cout << "Passphrases do not match. Aborting." << endl;
                continue;
            }
            
            if (masterPassphrase.empty()) {
                cout << "Master passphrase cannot be empty. Aborting." << endl;
                continue;
            }
            
            cout << "Enter password to encrypt: ";
            getline(cin, password);
            
            if (password.empty()) {
                cout << "Password cannot be empty. Aborting." << endl;
                continue;
            }
            
            encryptPassword(masterPassphrase, password);
            
        } else if (choice == 2) {
            auto files = listEncFiles();
            if (files.empty()) {
                cout << "No encrypted files found." << endl;
                continue;
            }
            
            cout << "Select a file to decrypt:" << endl;
            for (size_t i = 0; i < files.size(); i++) {
                cout << (i + 1) << ". " << files[i] << endl;
            }
            
            size_t fileIndex;
            cout << "Enter file number: ";
            cin >> fileIndex;

            if (cin.fail()) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a number." << endl;
                continue;
            }
            cin.ignore();

            cout << "Enter master passphrase: ";
            string masterPassphrase;
            getline(cin, masterPassphrase);
            
            if (fileIndex < 1 || fileIndex > files.size()) {
                cout << "Invalid selection." << endl;
                continue;
            }
            
            string decrypted = decryptPassword(masterPassphrase, files[fileIndex - 1]);
            
            if (!decrypted.empty()) {
                cout << "Decrypted password: " << decrypted << endl;
            }
            
        } else if (choice == 3) {
            break;
        } else {
            cout << "Invalid choice." << endl;
        }
    }
    
    return 0;
}

void showMenu() {
    cout << "\n=== PassPix Menu ===" << endl;
    cout << "1. Encrypt Password" << endl;
    cout << "2. Decrypt Password" << endl;
    cout << "3. Exit" << endl;
    cout << "Enter your choice: ";
}
