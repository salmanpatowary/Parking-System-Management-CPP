#include <iostream>
#include <string>
#include <queue>
#include <stack>
#include <fstream>
#include <ctime>
#include <cmath>
using namespace std;

// ================= Vehicle Class =================
class Vehicle {
public:
    string plate, owner, type;
    int ticket_id, booked_minutes;
    bool isVIP;
    time_t entry_time, exit_time;
    double fee;
    Vehicle* next;

    Vehicle(string p, string o, string t, int id, int mins, bool vip = false) {
        plate = p;
        owner = o;
        type = t;
        ticket_id = id;
        booked_minutes = mins;
        isVIP = vip;
        entry_time = time(NULL);
        exit_time = 0;
        fee = 0.0;
        next = nullptr;
    }

    void exitVehicle(double rate) {
        exit_time = time(NULL);
        long long stay_minutes = llround(difftime(exit_time, entry_time) / 60.0);
        if (stay_minutes < 0) stay_minutes = 0;

        if (stay_minutes <= booked_minutes)
            fee = stay_minutes * rate;
        else
            fee = (booked_minutes * rate) + ((stay_minutes - booked_minutes) * rate * 2.0);
    }

    void print(bool showExit=false) {
        cout << "Ticket " << ticket_id << " | Plate: " << plate
             << " | Owner: " << owner << " | Type: " << type
             << " | VIP: " << (isVIP ? "Yes" : "No")
             << " | Booked: " << booked_minutes << " mins"
             << " | Entry: " << ctime(&entry_time);
        if(showExit && exit_time != 0)
            cout << "   Exit: " << ctime(&exit_time) << "   Fee: " << fee << "\n";
    }
};

// ================= ParkingLot Class =================
class ParkingLot {
private:
    int cap1, cap2, vipSlots;
    int nextTicket;
    double totalRevenue;

    double carRate, bikeRate, truckRate, evRate;

    stack<Vehicle*> level1;
    stack<Vehicle*> level2;
    stack<Vehicle*> vipStack;

    queue<Vehicle*> waiting;
    queue<Vehicle*> vipQueue;

    Vehicle* historyHead;

public:
    ParkingLot(int c1,int c2,int vip) {
        cap1 = c1; cap2 = c2; vipSlots = vip;
        nextTicket = 1000;
        totalRevenue = 0;
        historyHead = nullptr;

        // Default rates per minute
        carRate = 0.2;
        bikeRate = 0.1;
        truckRate = 0.3;
        evRate = 0.15;
    }

    double getRate(const string& type) {
        if(type == "car") return carRate;
        else if(type == "bike") return bikeRate;
        else if(type == "truck") return truckRate;
        else if(type == "ev") return evRate;
        else return 0.2;
    }

    void park(string plate,string owner,string type,int mins,bool isVIP=false) {
        Vehicle* v = new Vehicle(plate, owner, type, nextTicket++, mins, isVIP);
        if(isVIP){
            if((int)vipStack.size() < vipSlots) {
                vipStack.push(v);
                cout << "VIP vehicle parked in VIP slot\n";
            } else {
                vipQueue.push(v);
                cout << "VIP vehicle added to VIP waiting queue\n";
            }
        }
        else if((type == "car" || type == "truck") && (int)level1.size() < cap1)
            level1.push(v);
        else if((type == "bike" || type == "ev") && (int)level2.size() < cap2)
            level2.push(v);
        else {
            if ((int)waiting.size() >= 10) {  // Waiting queue limit
                cout << "All slots full, waiting queue is also full. Cannot park vehicle now.\n";
                delete v;
                return;
            }
            waiting.push(v);
            cout << "All slots full, vehicle added to waiting queue\n";
        }
        cout << "Ticket issued: " << v->ticket_id << "\n";
    }

    void exitVehicle(int ticket) {
        if(exitFromStack(level1, ticket, 1)) return;
        if(exitFromStack(level2, ticket, 2)) return;
        if(exitFromStack(vipStack, ticket, 3)) return;
        bool found = false;
        int size = vipQueue.size();
        for(int i=0; i<size; i++) {
            Vehicle* v = vipQueue.front();
            vipQueue.pop();
            if(v->ticket_id == ticket) {
                v->exitVehicle(getRate(v->type));
                addHistory(v); totalRevenue += v->fee;
                cout << "VIP Vehicle exited from queue. Fee: " << v->fee << "\n";
                found = true;
            } else {
                vipQueue.push(v);
            }
        }
        if(!found) cout << "Ticket not found!\n";
        else allocateFromQueues(3);
    }

    bool exitFromStack(stack<Vehicle*>& stk, int ticket, int level) {
        stack<Vehicle*> temp; bool found = false;
        while(!stk.empty()) {
            Vehicle* v = stk.top(); stk.pop();
            if(v->ticket_id == ticket) {
                v->exitVehicle(getRate(v->type));
                addHistory(v); totalRevenue += v->fee;
                cout << "Vehicle exited. Fee: " << v->fee << "\n";
                found = true; break;
            }
            else temp.push(v);
        }
        while(!temp.empty()) { stk.push(temp.top()); temp.pop(); }
        if(found) allocateFromQueues(level);
        return found;
    }

    void allocateFromQueues(int levelFreed) {
        if(levelFreed == 1) {
            if(!waiting.empty()) {
                Vehicle* v = waiting.front();
                if((v->type == "car" || v->type == "truck") && (int)level1.size() < cap1) {
                    waiting.pop(); v->entry_time = time(NULL); level1.push(v);
                    cout << "Vehicle moved from waiting queue to Level1 slot\n";
                }
            }
        }
        else if(levelFreed == 2) {
            if(!waiting.empty()) {
                Vehicle* v = waiting.front();
                if((v->type == "bike" || v->type == "ev") && (int)level2.size() < cap2) {
                    waiting.pop(); v->entry_time = time(NULL); level2.push(v);
                    cout << "Vehicle moved from waiting queue to Level2 slot\n";
                }
            }
        }
        else if(levelFreed == 3) {
            if(!vipQueue.empty() && (int)vipStack.size() < vipSlots) {
                Vehicle* v = vipQueue.front();
                vipQueue.pop();
                v->entry_time = time(NULL);
                vipStack.push(v);
                cout << "VIP vehicle moved from queue to VIP slot\n";
            }
        }
    }

    void addHistory(Vehicle* v) {
        v->next = historyHead;
        historyHead = v;
    }

    void printStackSlots(stack<Vehicle*>& stk, int capacity) {
        stack<Vehicle*> temp;
        int idx = 1;
        while(!stk.empty()) {
            temp.push(stk.top());
            stk.pop();
        }
        for(int i = 0; i < capacity; i++) {
            cout << "Slot " << idx << ": ";
            if(!temp.empty()) {
                temp.top()->print();
                stk.push(temp.top());
                temp.pop();
            }
            else {
                cout << "Empty\n";
            }
            idx++;
        }
        while(!temp.empty()) {
            stk.push(temp.top());
            temp.pop();
        }
    }

    void printQueue(queue<Vehicle*> q, const string& label) {
        cout << label << "\n";
        if(q.empty()) { cout << "Empty\n"; return; }
        while(!q.empty()) {
            q.front()->print();
            q.pop();
        }
    }

    void printVipQueue(queue<Vehicle*> q) {
        if(q.empty()) { cout << "No VIPs waiting\n"; return; }
        while(!q.empty()) {
            q.front()->print();
            q.pop();
        }
    }

    void showParking() {
        cout << "\n--- VIP Slots ---\n";
        printStackSlots(vipStack, vipSlots);

        cout << "\n--- Level1 (Cars/Trucks) ---\n";
        printStackSlots(level1, cap1);

        cout << "\n--- Level2 (Bikes/EVs) ---\n";
        printStackSlots(level2, cap2);

        cout << "\n--- VIP Waiting Queue ---\n";
        printVipQueue(vipQueue);

        printQueue(waiting, "\n--- Waiting Queue ---");
    }

    void showHistory() {
        cout << "\n--- History ---\n";
        Vehicle* temp = historyHead;
        if(!temp) {
            cout << "No history yet\n";
            return;
        }
        while(temp) {
            temp->print(true);
            temp = temp->next;
        }
    }

    void saveHistory() {
        ofstream fout("history.txt");
        Vehicle* temp = historyHead;
        while(temp) {
            fout << "Ticket ID: " << temp->ticket_id << "\n";
            fout << "Plate: " << temp->plate << "\n";
            fout << "Owner Name: " << temp->owner << "\n";
            fout << "Type: " << temp->type << "\n";
            fout << "Booked Minutes: " << temp->booked_minutes << "\n";
            fout << "Entry Time: " << ctime(&temp->entry_time);
            fout << "Exit Time: " << ctime(&temp->exit_time);
            fout << "Fee: " << temp->fee << "\n";
            fout << "-------------------------\n";
            temp = temp->next;
        }
        fout.close();
        cout << "History saved\n";
    }

    void searchTicket(int ticket) {
        Vehicle* temp = historyHead;
        while(temp) {
            if(temp->ticket_id == ticket) {
                temp->print(true);
                return;
            }
            temp = temp->next;
        }

        if(searchStack(level1, ticket)) return;
        if(searchStack(level2, ticket)) return;
        if(searchStack(vipStack, ticket)) return;
        if(searchQueue(waiting, ticket)) return;
        if(searchQueue(vipQueue, ticket)) return;

        cout << "Ticket not found in system\n";
    }

    bool searchStack(stack<Vehicle*> stk, int ticket) {
        stack<Vehicle*> temp;
        bool found = false;
        while(!stk.empty()) {
            Vehicle* v = stk.top();
            stk.pop();
            if(v->ticket_id == ticket) {
                v->print();
                found = true;
            }
            temp.push(v);
        }
        while(!temp.empty()) {
            stk.push(temp.top());
            temp.pop();
        }
        return found;
    }

    bool searchQueue(queue<Vehicle*> q, int ticket) {
        while(!q.empty()) {
            if(q.front()->ticket_id == ticket) {
                q.front()->print();
                return true;
            }
            q.pop();
        }
        return false;
    }

    void adminMenu() {
        int choice;
        while(true) {
            cout << "\n--- Admin Menu ---\n";
            cout << "1. Show Parking\n2. Show History\n3. Search Ticket\n4. Save History\n5. Show Revenue\n6. Change Rates\n0. Exit Admin\nChoice: ";
            cin >> choice;
            if(choice == 1) showParking();
            else if(choice == 2) showHistory();
            else if(choice == 3) {
                int t;
                cout << "Enter Ticket: ";
                cin >> t;
                searchTicket(t);
            }
            else if(choice == 4) saveHistory();
            else if(choice == 5) cout << "Total Revenue: " << totalRevenue << "\n";
            else if(choice == 6) {
                cout << "--- Change Rates ---\n";
                cout << "Current Rates:\n";
                cout << "Car: " << carRate << " | Bike: " << bikeRate
                     << " | Truck: " << truckRate << " | EV: " << evRate << "\n";

                cout << "Enter new rate for Car: "; cin >> carRate;
                cout << "Enter new rate for Bike: "; cin >> bikeRate;
                cout << "Enter new rate for Truck: "; cin >> truckRate;
                cout << "Enter new rate for EV: "; cin >> evRate;
                cout << "Rates updated successfully!\n";
            }
            else if(choice == 0) break;
            else cout << "Invalid choice\n";
        }
    }
};

// ================= Main =================
int main() {
    int level1Slots, level2Slots, vipSlots;
    cout << "Enter number of slots for Level1 (Cars/Trucks): "; cin >> level1Slots;
    cout << "Enter number of slots for Level2 (Bikes/EVs): "; cin >> level2Slots;
    cout << "Enter number of VIP slots: "; cin >> vipSlots;

    ParkingLot lot(level1Slots, level2Slots, vipSlots);
    int choice;
    while(true) {
        cout << "\n1. Park Vehicle\n2. Exit Vehicle\n3. Show Parking\n4. Admin Panel\n0. Exit\nChoice: ";
        cin >> choice;
        if(choice == 1) {
            string plate, owner, type; int mins; char vip;
            cout << "Enter plate: "; cin >> plate;
            cout << "Enter owner: "; cin >> owner;
            cout << "Enter type (car/bike/truck/ev): "; cin >> type;
            cout << "Enter booked minutes: "; cin >> mins;
            cout << "Is VIP (y/n): "; cin >> vip;
            lot.park(plate, owner, type, mins, (vip == 'y' || vip == 'Y'));
        }
        else if(choice == 2) {
            int t;
            cout << "Enter ticket: ";
            cin >> t;
            lot.exitVehicle(t);
        }
        else if(choice == 3) lot.showParking();
        else if(choice == 4) lot.adminMenu();
        else if(choice == 0) break;
        else cout << "Invalid choice\n";
    }
    return 0;
}

