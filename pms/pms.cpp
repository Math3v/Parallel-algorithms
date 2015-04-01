/*
*	Pipeline Merge Sort
*	Author: Matej Minarik XMINAR29
*	File: pms.cpp
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <queue>

using namespace std;

#define OUT_VALS
#define _OUT_TIME
#define BUF_1_TAG 0
#define BUF_2_TAG 1
#define LAST_NUMBER 0xFFFFFFFE
#define FILENAME "numbers"

/* Q label */
enum which_t {
	first,
	second
};

/* map <procID, pair<sent from 1st, sent from 2nd Q> */
map <int, pair<int, int> > sent;

/*
*	Check whether or not was sent the last number.
*	After last number was sent, we can break the while loop.
*/
bool is_last_number(int *num) {
	const int last_number = LAST_NUMBER;
	return (last_number == *num);
}

/*
*	Increment sent pointer for specified PID and Q.
*/
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

/*
*	Check if we cand send value from both.
*	If so, we will send the lower/higher.
*/
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

/*
*	If we can send from 1st Q.
*/
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

/*
*	If we can send from 2nd Q.
*/
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

/*
*	Both Qs send the maximum for current iteration.
*/
bool cannot_send_from_both(int pid) {
	const int max = (int) pow(2.0, (float) (pid - 1));
	int fir = sent.find(pid)->second.first;
	int sec = sent.find(pid)->second.second;

	return (fir == max && sec == max);
}

/*
*	Reset sent counters.
*/
void reset_counters_sent(int pid) {
	sent.find(pid)->second.first = 0;
	sent.find(pid)->second.second = 0;
}

/*
*	Get tag for specified PID.
*/
int get_tag(int pid) {
	/* map<PID, Helper> */
	static map <int, int> counters;

	if(counters.find(pid) == counters.end()) {
		counters[pid] = 0;
	}
	else {
		int __cnt = counters.find(pid)->second;
		++__cnt;
		counters.find(pid)->second = __cnt;
	}
	/* (cnt_send / 2^pid) % 2 will receive 0/1, which specifies label Q of next process */
	const float outBuff = (float) ((float) ((float) counters.find(pid)->second) / (pow(2.0, (float) pid)));
	return (int) fmod(outBuff, 2);
}

/*
*	Time measurement.
*	1st call will set the timer variable, 
*	2nd call will output the time difference
*/
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

	/* OpenMPI Initialization */
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	/* Synchronize all processes */
	MPI_Barrier(MPI_COMM_WORLD);

	/* Root process */
	if(myid == 0) {
		ifstream myFile (FILENAME, ios::binary | ios::out );
		if(myFile.is_open()) {

			/* File size */
			myFile.seekg(0, ios::end);
			size_t size = myFile.tellg();
			myFile.seekg(0, ios::beg);

			/* Start time counting */
			timer();

			unsigned int cnt = 0;
			while(cnt < size) {
				myFile.read(data ,1);

				/* Number needs to be 0-255*/
				number = (int) ((unsigned char) (*data));
				#ifdef OUT_VALS
					cout << number << " ";
				#endif

				/* 1st process just send values from files, alternating the Qs */
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
			myFile.close();

			#ifdef OUT_VALS
				cout << endl;
			#endif
		}

		/* 1st process is done */
		MPI_Isend(&lnum, 1, MPI_INT, 1, BUF_1_TAG, MPI_COMM_WORLD, &request);
		MPI_Wait(&request, &stat);
	}
	/* Last process */
	else if(myid == (numprocs - 1)) {
		while(true) {

			/* If there are values in both Qs send them to stdout sorted */
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
			/* If there is nothing to send we will be receiving */
			else {
				/* Is there something to receive? */
				MPI_Iprobe(myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
				if(flag == true) {

					/* ... yes, there is. */
					MPI_Recv(&number, 1, MPI_INT, myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

					/* We received the last number, pop everything sorted and break */
					if(is_last_number(&number)) {
						while(!mynums_1.empty() || !mynums_2.empty()) {
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
	/* Other processes */
	else {		
		while(true){
			/* If there are values in both Qs send them to stdout sorted */
			if(can_send_from_both(myid) && (!mynums_1.empty() && !mynums_2.empty())) {
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
				/* Is there something to receive? */
				MPI_Iprobe(myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
				if(flag == true) {
					/* ... yes, there is. */
					MPI_Recv(&number, 1, MPI_INT, myid - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

					/* If it was last number, send and break the loop */
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

	/* Synchronize all processes */
	MPI_Barrier(MPI_COMM_WORLD);

	/* Root process measure time */
	if(myid == 0)
		timer();

	/* Tell OpenMPI that there are no OpenMPI calls after this */
	MPI_Finalize();

	/* Memory cleanup */
	free(data);

	return 0;
}
