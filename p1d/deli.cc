#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include "thread.h"
using namespace std;

unsigned int LOCK_ID = 1;
unsigned int CASHIER_CV = 1111;
unsigned int MAKER_CV = 2222;

vector<int> board;
int max_orders;
int max_cashiers;
int active_cashiers;
int current_cashier = 0;
int last_sandwich = -1;

void cashier(void* filename) {
	thread_lock(LOCK_ID);

	ifstream inFile;
	inFile.open((char*) filename);
	//cout << (char*) filename << endl;

	int cashier_num = current_cashier;
	current_cashier++;

	int sandwich;
	while (inFile >> sandwich) {
		while(board.size() >= max_orders){
			thread_signal(LOCK_ID, MAKER_CV);
			thread_wait(LOCK_ID, CASHIER_CV);
		}
		board.push_back(sandwich);
		cout << "POSTED: cashier " << cashier_num << " sandwich " << sandwich << endl;
		
		if (board.size() == max_orders)
			thread_signal(LOCK_ID, MAKER_CV);

		thread_wait(LOCK_ID, sandwich);
		cout << "READY: cashier " << cashier_num << " sandwich " << sandwich << endl;
		
		thread_signal(LOCK_ID, MAKER_CV);
		thread_wait(LOCK_ID,CASHIER_CV);
	}

	active_cashiers--;

	thread_signal(LOCK_ID,MAKER_CV);

    thread_unlock(LOCK_ID);
}

void maker(void* arg) {
	//start_preemptions(true, true, rand());

	thread_lock(LOCK_ID);

	char** argv = (char**) arg;
	for (int i = 0; i < max_cashiers; i++) {
		thread_create(cashier, argv[i + 2]);
	}

	active_cashiers = max_cashiers;

	int closest;
	int closest_loc;

	while (active_cashiers > 0) {
		while(board.size() < max_orders) {
			thread_signal(LOCK_ID, CASHIER_CV);
			thread_wait(LOCK_ID, MAKER_CV);

			if (active_cashiers < max_orders)
				max_orders = active_cashiers;

			if (active_cashiers == 0) {
				thread_unlock(LOCK_ID);
				return;
			}
		}

		int delta = 1000;
		for (int i = 0; i < board.size(); i++) {
			if (abs(last_sandwich - board[i]) < delta) {
				delta = abs(last_sandwich - board[i]);
				closest = board[i];
				closest_loc = i;
			}
		}
		
		last_sandwich = closest;
		board.erase(board.begin() + closest_loc);

		thread_signal(LOCK_ID, last_sandwich);
		thread_wait(LOCK_ID, MAKER_CV);
	}

	thread_unlock(LOCK_ID);

	return;
}

int main(int argc, char *argv[])
{
	max_cashiers = argc - 2;
	max_orders = atoi(argv[1]);
	thread_libinit(maker, argv);
	return 0;
}