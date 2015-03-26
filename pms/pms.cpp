#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <queue>

using namespace std;

#define _OUT_VALS
#define OUT_TIME
#define BUF_1_TAG 0
#define BUF_2_TAG 1
#define LAST_NUMBER 0xFFFFFFFE
#define FILENAME "numbers"

enum which_t {
	first,
	second
};

map <int, pair<int, int> > sent;
static double begin, end;

bool is_last_number(int *num) {
	const int last_number = LAST_NUMBER;
	return (last_number == *num);
}

void increment_sent(int pid, enum which_t which) {
	int recv;
	if(sent.find(pid) == sent.end()) {
		sent[pid] = make_pair(0, 0);	
		//begin = MPI_Wtime();
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

int get_tag(int pid) {
	static map <int, int> counters;

	if(counters.find(pid) == counters.end()) {
		counters[pid] = 0;
	}
	else {
		int __cnt = counters.find(pid)->second;
		++__cnt;
		counters.find(pid)->second = __cnt;
	}
	const float outBuff = (float) ((float) ((float) counters.find(pid)->second) / (pow(2.0, (float) pid)));
	return (int) fmod(outBuff, 2);
}

void timer(){
	static double timer = 0;

	if(timer == 0){
		timer = MPI_Wtime();
	}
	else{
		#ifdef OUT_TIME
		printf("%d\n", (int) ((MPI_Wtime() - timer) * 1000000));
		#else
		;
		#endif
	}
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
	int number_amount;
	int flag;
	int received = 0;
	bool send;
	int lnum = LAST_NUMBER;

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	MPI_Barrier(MPI_COMM_WORLD);
	if(myid == 0) { // Root CPU
		ifstream myFile (FILENAME, ios::binary | ios::out );
		if(myFile.is_open()) {

			/* File size */
			myFile.seekg(0, ios::end);
			size_t size = myFile.tellg();
			myFile.seekg(0, ios::beg);

			//cout << "File size is: " << size << endl;

			/* Start time counting */
			timer();
			unsigned int cnt = 0;
			while(cnt < size) {
				myFile.read(data ,1);
				//number = (int) ((*data) + 128);
				number = (int) (*data);
				#ifdef OUT_VALS
				cout << number << " ";
				#endif

				if(cnt % 2 == 0) {
					MPI_Isend(&number, 1, MPI_INT, 1, BUF_1_TAG, MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &stat);
				}
				else {
					MPI_Isend(&number, 1, MPI_INT, 1, BUF_2_TAG, MPI_COMM_WORLD, &request);
					MPI_Wait(&request, &stat);
				}
				++cnt;
			}
			end = MPI_Wtime();
			myFile.close();
			#ifdef OUT_VALS
			cout << endl;
			#endif
		}

		MPI_Isend(&lnum, 1, MPI_INT, 1, BUF_1_TAG, MPI_COMM_WORLD, &request);
		MPI_Wait(&request, &stat);
	}
	else if(myid == (numprocs - 1)) { // Last CPU	
		while(true) {
			if(!mynums_1.empty() && !mynums_2.empty()) {
				if(mynums_1.front() < mynums_2.front()) {
					#ifdef OUT_VALS
					cout << (int) mynums_1.front() << endl;
					#endif
					mynums_1.pop();
				}
				else {
					#ifdef OUT_VALS
					cout << (int) mynums_2.front() << endl;
					#endif
					mynums_2.pop();
				}
			}
			else {
				MPI_Iprobe(myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
				if(flag == true) {
					MPI_Recv(&number, 1, MPI_INT, myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

					if(is_last_number(&number)) {
						//cout << "1 " << mynums_1.front() << " 2 " << mynums_2.front() << endl;
						while(!mynums_1.empty() || !mynums_2.empty()) {
							//cout << "1 " << mynums_1.front() << " 2 " << mynums_2.front() << endl;
							if(!mynums_1.empty() && !mynums_2.empty()) {
								if(mynums_1.front() < mynums_2.front()) {
									#ifdef OUT_VALS
									cout << (int) mynums_1.front() << endl;
									#endif
									mynums_1.pop();
								}
								else {
									#ifdef OUT_VALS
									cout << (int) mynums_2.front() << endl;
									#endif
									mynums_2.pop();
								}
							}
							else if(mynums_1.empty() && !mynums_2.empty()) {
								#ifdef OUT_VALS
								cout << (int) mynums_2.front() << endl;
								#endif
								mynums_2.pop();
							}
							else if(!mynums_1.empty() && mynums_2.empty()) {
								#ifdef OUT_VALS
								cout << (int) mynums_1.front() << endl;
								#endif
								mynums_1.pop();
							}
						}
						break;
					}

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
			else {
				//Is there something to receive?
				MPI_Iprobe(myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
				if(flag == true) {
					MPI_Recv(&number, 1, MPI_INT, myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

					if(is_last_number(&number)) {
						MPI_Isend(&lnum, 1, MPI_INT, myid + 1, get_tag(myid), MPI_COMM_WORLD, &request);
						MPI_Wait(&request, &stat);
						break;
					}
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

	MPI_Barrier(MPI_COMM_WORLD);
	if(myid == 0)
		timer();

	free(data);
	MPI_Finalize();
	return 0;
}
