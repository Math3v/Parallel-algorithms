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
	int numbers[] = {3,5,8,6,11,5,17,18};

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
				cout << "Value read: " << number << endl;
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
			cout << "All values have been read!" << endl;
		}
	}
	else if(myid == (numprocs - 1)) { // Last CPU
		while(true) {
			MPI_Recv(&number, 1, MPI_INT, myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
			cout << "Last CPU received: " << (int) number << endl;

				/* Receive value and insert into buffer */
				if(stat.MPI_TAG == BUF_1_TAG) {
					mynums_1.push(number);
				}
				else if(stat.MPI_TAG == BUF_2_TAG) {
					mynums_2.push(number);
				}
				if((mynums_1.size() == sizeOfBuffer && mynums_2.size() >= 1)
					|| (mynums_2.size() == sizeOfBuffer && mynums_1.size() >= 1)){
					working[myid] = true;
				}
				/* Chech if CPUi have to do something */
				if(working.find(myid) != working.end() && working.find(myid)->second == true) {
					// Output values
					if(!mynums_1.empty() && (mynums_2.empty() || mynums_1.front() < mynums_2.front())) {
						cout << mynums_1.front() << endl; 
						mynums_1.pop();
					}
					else if(!mynums_2.empty()) {
						cout << mynums_2.front() << endl;
						mynums_2.pop();
					}

					cout << "Last CPU" << endl;
					cout << "Mynums1" << endl;
					while(!mynums_1.empty()) {
						cout << mynums_1.front() << endl;
						mynums_1.pop();
					}

					cout << "Mynums2" << endl;
					while(!mynums_2.empty()) {
						cout << mynums_2.front() << endl;
						mynums_2.pop();
					}
				}
		}
	}
	else { // Other CPUs
		while(true){
			MPI_Recv(&number, 1, MPI_INT, myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
			
			/* Receive value and insert into buffer */
			if(stat.MPI_TAG == BUF_1_TAG) {
				mynums_1.push(number);
			}
			else if(stat.MPI_TAG == BUF_2_TAG) {
				mynums_2.push(number);
			}

			//cout << "[" << myid << "]: " << mynums_1.size() << "/" << mynums_2.size() << endl;

			/* Chech if CPUi have to do something */
			if((mynums_1.size() == sizeOfBuffer && mynums_2.size() >= 1)
				|| (mynums_2.size() == sizeOfBuffer && mynums_1.size() >= 1)){
				working[myid] = true;
			}
			if(working.find(myid) != working.end() && working.find(myid)->second == true) {
				if(!mynums_1.empty() && (mynums_2.empty() || mynums_1.front() > mynums_2.front())) {
					MPI_Isend(&mynums_1.front(), 1, MPI_INT, myid + 1, BUF_1_TAG, MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &stat);
					mynums_1.pop();
				}
				else if(!mynums_2.empty()) {
					MPI_Isend(&mynums_2.front(), 1, MPI_INT, myid + 1, BUF_2_TAG, MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &stat);
					mynums_2.pop();
				}
			}
		}
	}

	free(data);
	MPI_Finalize();
	return 0;
}