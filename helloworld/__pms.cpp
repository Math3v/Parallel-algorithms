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

map <int, int> counters;

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
	cout << "Proc " << pid << " value no " << (int) counters.find(pid)->second << " sending to " << (int) fmod(outBuff, 2);

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
				number = numbers[cnt];
				//cout << "Value read: " << number << endl;
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
			cout << "Waiting " << myid << " " << mynums_1.size() << ":" << mynums_2.size() << endl;
			MPI_Recv(&number, 1, MPI_INT, myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
			cout << "Last CPU received: " << (int) number <<  " at buffer " << stat.MPI_TAG << endl;	

				/* Receive value and insert into buffer */
				if(stat.MPI_TAG == BUF_1_TAG) {
					mynums_1.push(number);
				}
				else if(stat.MPI_TAG == BUF_2_TAG) {
					mynums_2.push(number);
				}
				cout << "Proc " << myid << " " << mynums_1.size() << ":" << mynums_2.size() << endl;
				if(mynums_2.size() >= 1) { //Last CPU can do some work
					if(mynums_1.front() < mynums_2.front()) {
						cout << mynums_1.front() << endl;
						mynums_1.pop();
					}
					else {
						cout << mynums_2.front() << endl;
						mynums_2.pop();
					}
				}
				else if(mynums_1.empty() && (mynums_2.size() >= 1)) { //Last element print
					cout << mynums_2.front() << endl;
					mynums_2.pop();
				}
		}
	}
	else { // Other CPUs
		while(true){
			cout << "Waiting " << myid << " " << mynums_1.size() << ":" << mynums_2.size() << endl;
			MPI_Recv(&number, 1, MPI_INT, myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);	
			
			/* Receive value and insert into buffer */
			if(stat.MPI_TAG == BUF_1_TAG) {
				mynums_1.push(number);
			}
			else if(stat.MPI_TAG == BUF_2_TAG) {
				mynums_2.push(number);
			}
			cout << "Proc " << myid << " " << mynums_1.size() << ":" << mynums_2.size() << endl;
			/* If CPUi have to do some work */
			if(mynums_2.size() >= 1) {
				if(counters.find(myid) == counters.end()) {
					counters[myid] = 0;
				}
				else {
					int __cnt = counters.find(myid)->second;
					++__cnt;
					counters.find(myid)->second = __cnt;
				}
				working[myid] = true;
				const float outBuff = (float) ((float) ((float) counters.find(myid)->second) / (pow(2.0, (float) myid)));
				cout << "Proc " << myid << " value no " << (int) counters.find(myid)->second << " sending to " << (int) fmod(outBuff, 2);
				if(mynums_1.front() < mynums_2.front()) {
					if(((int) fmod(outBuff, 2)) == 0){
						MPI_Isend(&mynums_1.front(), 1, MPI_INT, myid + 1, BUF_1_TAG, MPI_COMM_WORLD, &request);
						MPI_Wait(&request, &stat);
					}
					else if(((int) fmod(outBuff, 2)) == 1) {
						MPI_Isend(&mynums_1.front(), 1, MPI_INT, myid + 1, BUF_2_TAG, MPI_COMM_WORLD, &request);
						MPI_Wait(&request, &stat);
					}
					else {
						cout << "ERROR: " << outBuff << endl;
					}
					cout << " actual value " << mynums_1.front() << endl;
					mynums_1.pop();
				}
				else {
					if(((int) fmod(outBuff, 2)) == 0){
						MPI_Isend(&mynums_2.front(), 1, MPI_INT, myid + 1, BUF_1_TAG, MPI_COMM_WORLD, &request);
						MPI_Wait(&request, &stat);
					}
					else if(((int) fmod(outBuff, 2)) == 1) {
						MPI_Isend(&mynums_2.front(), 1, MPI_INT, myid + 1, BUF_2_TAG, MPI_COMM_WORLD, &request);
						MPI_Wait(&request, &stat);
					}
					else {
						cout << "ERROR: " << outBuff << endl;
					}
					cout << " actual value " << mynums_2.front() << endl;
					mynums_2.pop();
				}
			}
			else if(mynums_1.empty() && (mynums_2.size() >= 1)) { //Last element print
				MPI_Isend(&mynums_2.front(), 1, MPI_INT, myid + 1, BUF_2_TAG, MPI_COMM_WORLD, &request);
				MPI_Wait(&request, &stat);
				mynums_2.pop();
			}
			else if(working.find(myid) != working.end() && working.find(myid)->second == true) {
				MPI_Isend(&mynums_1.front(), 1, MPI_INT, myid + 1, BUF_2_TAG, MPI_COMM_WORLD, &request);
				MPI_Wait(&request, &stat);
				mynums_1.pop();
			}
	}

	free(data);
	MPI_Finalize();
	return 0;
}