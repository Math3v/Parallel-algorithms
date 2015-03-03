#include <mpi.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <unistd.h> //sleep

using namespace std;

#define BUF_1_TAG 0
#define BUF_2_TAG 1
#define NUM_OF_INPUTS 8

enum which_t {
	first,
	second
};

map <int, pair<int, int> > sent;
map <int, int> counters;
map <int, int> received;
const bool debug = false;

void increment_sent(int pid, enum which_t which) {
	int recv;
	if(sent.find(pid) == sent.end()) {
		sent[pid] = make_pair(0, 0);	
	}
	if(which == first) {
		recv = sent.find(pid)->second.first;
		++recv;
		sent.find(pid)->second.first = recv;
	}
	else {
		recv = sent.find(pid)->second.second;
		++recv;
		sent.find(pid)->second.second = recv;
	}
}

bool can_send_from_both(int pid) {
	const int max = (int) pow(2.0, (float) (pid - 1));
	if(sent.find(pid) == sent.end()) {
		return true;
	}
	else {
		int fir = sent.find(pid)->second.first;
		int sec = sent.find(pid)->second.second;
		if(fir < max && sec < max){
			return true;
		}
		else {
			return false;
		}
	}
}

bool can_send_from_first(int pid) {
	const int max = (int) pow(2.0, (float) (pid - 1));
	int fir = sent.find(pid)->second.first;
	int sec = sent.find(pid)->second.second;
	if(fir < max && sec == max){
		return true;
	}
	else {
		return false;
	}
}

bool can_send_from_second(int pid) {
	const int max = (int) pow(2.0, (float) (pid - 1));
	int fir = sent.find(pid)->second.first;
	int sec = sent.find(pid)->second.second;
	if(fir == max && sec < max){
		return true;
	}
	else {
		return false;
	}
}

bool cannot_send_from_both(int pid) {
	const int max = (int) pow(2.0, (float) (pid - 1));
	int fir = sent.find(pid)->second.first;
	int sec = sent.find(pid)->second.second;

	return (fir == max && sec == max);
}

void reset_counters_sent(int pid) {
	sent.find(pid)->second.first = 0;
	sent.find(pid)->second.second = 0;
}

void increment_received(int pid) {
	if(received.find(pid) == received.end()) {
		received[pid] = 1;
	}
	else {
		int __recv = received.find(pid)->second;
		++__recv;
		received.find(pid)->second = __recv;
	}
}

bool will_recv(int pid) {
	if(received.find(pid) == received.end()) {
		return true;
	}
	else if(received.find(pid)->second >= 8) {
		return false;
	}
	return true;
}

int get_tag(int pid) {
	if(counters.find(pid) == counters.end()) {
		counters[pid] = 0;
	}
	else {
		int __cnt = counters.find(pid)->second;
		++__cnt;
		counters.find(pid)->second = __cnt;
	}
	const float outBuff = (float) ((float) ((float) counters.find(pid)->second) / (pow(2.0, (float) pid)));
	if(debug)
		cout << "Proc " << pid << " value no " << (int) counters.find(pid)->second << " sending to " << (int) fmod(outBuff, 2) << endl;

	return (int) fmod(outBuff, 2);
}

void wait_for_gdb(){
	int i = 0;
    printf("PID %d ready for attach\n", getpid());
    fflush(stdout);
    while (0 == i)
        sleep(5);
}

int main(int argc, char **argv) {
	int numprocs;
	int myid;
	int number;
	char *data = (char *) malloc(1);
	MPI_Status stat;
	MPI_Request request;
	queue<int> mynums_1;
	queue<int> mynums_2;
	map <int, bool> working;
	int numbers[] = {3,5,8,6,11,9,17,18};
	int number_amount;
	int flag;
	int received = 0;
	bool send;

	//wait_for_gdb();

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	cout << "Num of procs: " << numprocs << " my rank is: " << myid << endl;
	const int sizeOfBuffer = pow(2.0, (float) myid - 1);

	if(myid == 0) { // Root CPU
		ifstream myFile ("random.dat", ios::binary);
		if(myFile.is_open()) {
			unsigned int cnt = 0;
			while(myFile.good()) {
				myFile.read(data ,1);
				number = (int) ((*data) + 128);
				cout << "Value read " << (int) number << endl;
				//number = numbers[cnt];
				if(cnt % 2 == 0) {
					MPI_Isend(&number, 1, MPI_INT, 1, BUF_1_TAG, MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &stat);
				}
				else {
					MPI_Isend(&number, 1, MPI_INT, 1, BUF_2_TAG, MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &stat);
				}
				++cnt;
				if(cnt == 8)
					break;
			}
			myFile.close();
			//cout << "All values have been read!" << endl;
		}
	}
	else if(myid == (numprocs - 1)) { // Last CPU
		while(true) {
			if(!mynums_1.empty() && !mynums_2.empty()) {
				if(mynums_1.front() < mynums_2.front()) {
					cout << (int) mynums_1.front() << endl;
					mynums_1.pop();
				}
				else {
					cout << (int) mynums_2.front() << endl;
					mynums_2.pop();
				}
			}
			
			else if(will_recv(myid) == false){
				if(mynums_1.empty() && !mynums_2.empty()) {
					cout << (int) mynums_2.front() << endl;
					mynums_2.pop();
				}
				else if(!mynums_1.empty() && mynums_2.empty()) {
					cout << (int) mynums_1.front() << endl;
					mynums_1.pop();
				}
				else {
					cout << "Breaking process " << myid << endl;
					break;
				}
			}

			else {
				MPI_Iprobe(myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
				if(flag == true) {
					MPI_Recv(&number, 1, MPI_INT, myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
					cout << "Value " << (int) number << " received at " << stat.MPI_TAG << endl;
					increment_received(myid);
					/* Receive value and insert into buffer */
					if(stat.MPI_TAG == BUF_1_TAG) {
						mynums_1.push(number);
					}
					else if(stat.MPI_TAG == BUF_2_TAG) {
						mynums_2.push(number);
					}
				}
			}

			
		}
	}
	else { // Other CPUs
		while(true){
			if(can_send_from_both(myid) && (!mynums_1.empty() && !mynums_2.empty())) {
				//Compare and send
				if(mynums_1.front() > mynums_2.front()) {
					MPI_Isend(&mynums_2.front(), 1, MPI_INT, myid + 1, get_tag(myid), MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &stat);
					mynums_2.pop();
					increment_sent(myid, second);
				}
				else {
					MPI_Isend(&mynums_1.front(), 1, MPI_INT, myid + 1, get_tag(myid), MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &stat);
					mynums_1.pop();
					increment_sent(myid, first);
				}
			}
			else if(can_send_from_first(myid) && !mynums_1.empty()) {
				MPI_Isend(&mynums_1.front(), 1, MPI_INT, myid + 1, get_tag(myid), MPI_COMM_WORLD, &request);
				MPI_Wait(&request, &stat);
				mynums_1.pop();
				increment_sent(myid, first);
			}
			else if(can_send_from_second(myid) && !mynums_2.empty()) {
				MPI_Isend(&mynums_2.front(), 1, MPI_INT, myid + 1, get_tag(myid), MPI_COMM_WORLD, &request);
				MPI_Wait(&request, &stat);
				mynums_2.pop();
				increment_sent(myid, second);
			}
			else if(cannot_send_from_both(myid)) {
				reset_counters_sent(myid);
			}
			/*
			else if(will_recv(myid) == false) {
				if(mynums_1.empty() && !mynums_2.empty()) {
					MPI_Isend(&mynums_2.front(), 1, MPI_INT, myid + 1, get_tag(myid), MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &stat);
					mynums_2.pop();
				}
				else if(!mynums_1.empty() && mynums_2.empty()) {
					MPI_Isend(&mynums_1.front(), 1, MPI_INT, myid + 1, get_tag(myid), MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &stat);
					mynums_1.pop();
				}
				else {
					cout << "Breaking process " << myid << endl;
					break;
				}
			}
			*/
			else {
				//Is there something to receive?
				MPI_Iprobe(myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
				if(flag == true) {
					MPI_Recv(&number, 1, MPI_INT, myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
					increment_received(myid);
					/* Receive value and insert into buffer */
					if(stat.MPI_TAG == BUF_1_TAG) {
						mynums_1.push(number);
					}
					else if(stat.MPI_TAG == BUF_2_TAG) {
						mynums_2.push(number);
					}
					continue;
				}
			}
			
		}
	}

	free(data);
	MPI_Finalize();
	return 0;
}