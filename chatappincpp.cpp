#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <limits>
#include <algorithm>
#include <cstdlib>

using namespace std;

// -- 1. Core Data Structures --
enum MessageStatus {
    SENT,
    DELIVERED,
    SEEN
};

struct Message {
    string sender;
    string receiver;
    string content;
    MessageStatus status = SENT;
    Message* next = nullptr;
};

struct User {
    string fullName;
    string dateOfBirth;
    string username;
    string password;
    Message* inboxHead = nullptr;
    Message* outboxHead = nullptr;
    User* next = nullptr;
};

// -- 2. Helper Functions --
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int getNumericInput() {
    int value;
    while (!(cin >> value)) {
        cout << "Invalid input. Please enter a valid number: ";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return value;
}

// -- 3. Manager Classes --
class UserManager {
private:
    User* userListHead = nullptr;

    string generateUsername(string name, string dob) {
        transform(name.begin(), name.end(), name.begin(), ::tolower);
        string firstName = name.substr(0, name.find(' '));
        string dobPart = dob.substr(0, 4);
        return firstName + dobPart;
    }

public:
    ~UserManager() {
        User* currentUser = userListHead;
        while (currentUser != nullptr) {
            User* nextUser = currentUser->next;
            // Memory for Message objects is now handled solely by MessageSystem
            delete currentUser;
            currentUser = nextUser;
        }
    }

    string registerUser(const string& fullName, const string& dob, const string& password) {
        string newUsername = generateUsername(fullName, dob);
        if (findUser(newUsername) != nullptr) {
            cout << "\nError: An account with similar details already exists." << endl;
            return "";
        }
        User* newUser = new User{fullName, dob, newUsername, password};
        if (!userListHead) {
            userListHead = newUser;
        } else {
            User* current = userListHead;
            while (current->next) {
                current = current->next;
            }
            current->next = newUser;
        }
        return newUsername;
    }

    User* findUser(const string& username) {
        User* current = userListHead;
        while (current) {
            if (current->username == username) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

    User* authenticateUser(const string& username, const string& password) {
        User* user = findUser(username);
        if (user && user->password == password) {
            return user;
        }
        return nullptr;
    }
};

class MessageSystem {
private:
    queue<Message*> messageDeliveryQueue;
    vector<Message*> allMessages; // The central store for all messages

public:
    ~MessageSystem() {
        for (Message* msg : allMessages) {
            delete msg;
        }
    }

    // MODIFIED: Now creates ONE shared message object
    void sendMessage(User* sender, User* receiver, const string& text) {
        if (!sender || !receiver) return;

        Message* shared_msg = new Message{sender->username, receiver->username, text};
        allMessages.push_back(shared_msg);

        messageDeliveryQueue.push(shared_msg);

        shared_msg->next = sender->outboxHead;
        sender->outboxHead = shared_msg;

        cout << "\nMessage sent to " << receiver->username << "." << endl;
    }

    // MODIFIED: Updates the status of the shared message
    void processDeliveries(UserManager& userManager) {
        while (!messageDeliveryQueue.empty()) {
            Message* msgToDeliver = messageDeliveryQueue.front();
            messageDeliveryQueue.pop();
            User* receiver = userManager.findUser(msgToDeliver->receiver);
            if (receiver) {
                msgToDeliver->status = DELIVERED;
                msgToDeliver->next = receiver->inboxHead;
                receiver->inboxHead = msgToDeliver;
            }
        }
    }

    // MODIFIED: Now updates status to SEEN and displays the correct status string
    void displayConversationHistory(User* user, UserManager& userManager) {
        processDeliveries(userManager);

        cout << "\n--- Full Conversation History for " << user->username << " ---" << endl;

        Message* inboxCurrent = user->inboxHead;
        while (inboxCurrent) {
            if (inboxCurrent->status == DELIVERED) {
                inboxCurrent->status = SEEN;
            }
            inboxCurrent = inboxCurrent->next;
        }

        cout << "\n--- Messages You Sent (Outbox) ---" << endl;
        if (!user->outboxHead) {
            cout << "Outbox is empty." << endl;
        } else {
            Message* outboxCurrent = user->outboxHead;
            while (outboxCurrent) {
                string statusText;
                switch (outboxCurrent->status) {
                    case SENT:      statusText = "(Sent)";      break;
                    case DELIVERED: statusText = "(Delivered)"; break;
                    case SEEN:      statusText = "(Seen)";      break;
                }
                cout << "To: " << outboxCurrent->receiver << " | "
                     << "Status: " << statusText
                     << " | Message: " << outboxCurrent->content << endl;
                outboxCurrent = outboxCurrent->next;
            }
        }

        cout << "\n--- Messages You Received (Inbox) ---" << endl;
        if (!user->inboxHead) {
            cout << "Inbox is empty." << endl;
        } else {
            inboxCurrent = user->inboxHead;
            while (inboxCurrent) {
                cout << "From: " << inboxCurrent->sender << " | Message: " << inboxCurrent->content << endl;
                inboxCurrent = inboxCurrent->next;
            }
        }
        cout << "\n---------------------------------------------\n";
    }
};

// -- 4. Main Application UI and Logic --
void showLoggedInMenu(const string& username) {
    cout << "\n--- Logged in as " << username << " ---" << endl;
    cout << "1. Send a Message" << endl;
    cout << "2. View Conversation History" << endl;
    cout << "9. Logout" << endl;
    cout << "Enter choice: ";
}

void showMainMenu() {
    cout << "\n========= CHAT APPLICATION =========\n";
    cout << "1. Login" << endl;
    cout << "2. Register New Account" << endl;
    cout << "0. Exit" << endl;
    cout << "==================================\n";
    cout << "Enter choice: ";
}

int main() {
    UserManager userManager;
    MessageSystem messageSystem;
    User* currentUser = nullptr;
    int choice;

    while (true) {
        if (currentUser == nullptr) {
            showMainMenu();
            choice = getNumericInput();
            if (choice == 1) {
                string username, password;
                cout << "Enter username: ";
                getline(cin, username);
                cout << "Enter password: ";
                getline(cin, password);
                currentUser = userManager.authenticateUser(username, password);
                if (!currentUser) {
                    cout << "\nLogin failed. Invalid username or password." << endl;
                } else {
                    cout << "\nWelcome, " << currentUser->fullName << "!" << endl;
                }
            } else if (choice == 2) {
                string fullName, dob, password, confirmPassword;
                cout << "Enter your full name: ";
                getline(cin, fullName);
                cout << "Enter your date of birth (DDMMYYYY): ";
                getline(cin, dob);
                cout << "Set a password: ";
                getline(cin, password);
                cout << "Confirm your password: ";
                getline(cin, confirmPassword);
                if (password != confirmPassword) {
                    cout << "\nError: Passwords do not match." << endl;
                } else {
                    string generatedUsername = userManager.registerUser(fullName, dob, password);
                    if (!generatedUsername.empty()) {
                        cout << "\n✅ Registration successful!" << endl;
                        cout << "Your generated username is: " << generatedUsername << endl;
                        cout << "Please use this username to log in." << endl;
                    }
                }
            } else if (choice == 0) {
                break;
            } else {
                cout << "\nInvalid choice. Please enter 1, 2, or 0." << endl;
            }
        } else {
            showLoggedInMenu(currentUser->username);
            choice = getNumericInput();
            switch (choice) {
                case 1: {
                    string receiverName, messageText;
                    cout << "Enter recipient's username: ";
                    getline(cin, receiverName);
                    User* receiver = userManager.findUser(receiverName);
                    if (!receiver) {
                        cout << "\nError: User '" << receiverName << "' not found." << endl;
                    } else if (receiver->username == currentUser->username) {
                        cout << "\nError: You cannot send a message to yourself." << endl;
                    } else {
                        cout << "Enter your message: ";
                        getline(cin, messageText);
                        messageSystem.sendMessage(currentUser, receiver, messageText);
                    }
                    break;
                }
                case 2:
                    messageSystem.displayConversationHistory(currentUser, userManager);
                    break;
                case 9:
                    cout << "\nLogging out " << currentUser->username << "..." << endl;
                    currentUser = nullptr;
                    clearScreen();
                    break;
                default:
                    cout << "\nInvalid choice. Please try again." << endl;
                    break;
            }
        }
    }
    cout << "\nExiting application. Goodbye!" << endl;
    return 0;
}
