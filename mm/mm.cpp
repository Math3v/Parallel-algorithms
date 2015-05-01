/*
*	Paralel Mesh Multiplication
*	Author: Matej Minarik XMINAR29
*	File: mm.cpp
*/

#define NO_DEBUG
#define NO_TIME
#define OUT

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include <climits>

using namespace std;

#define HOR_TAG 0
#define VER_TAG 1
#define RES_TAG 2

/* vector of vectors of integers */
typedef vector<vector<int> > matrix;

typedef vector<int> row;

/* A * B = C*/
matrix matA, matB, matC;
/* Notify process, it should end */
int last_number = INT_MAX;

void print_matrix(matrix m) {
	matrix::iterator mi;
	row::iterator i;

	for(mi = m.begin(); mi != m.end(); ++mi) {
		for(i = (*mi).begin(); i != (*mi).end(); ++i) {
			cout << *i << " ";
		}
		cout << endl;
	}
}

void print_vector(row v) {
	row::iterator i = v.begin();

	for(; i != v.end(); ++i) {
		cout << *i << " ";
	}

	cout << endl;
}

/* Split line to tokens */
row split_line(string line) {
	string buf;
	stringstream ss(line);
	row tokens;

	while(ss >> buf) {
		tokens.push_back(atoi(buf.c_str()));
	}

	return tokens;
}

/* Delete newline character */
void remove_newline(char **str) {
	int ln = strlen(*str);
	char c = *(*str + ln - 1);

	if( c == '\n' ){
		*(*str + ln - 1) = '\0';
	}
	else {
		return;
	}
}

/* Read matrix m from file specified by filename */
bool read_matrix(const char* filename, matrix *m) {
	FILE *fr = fopen(filename, "r");
	char *str = NULL;
	size_t len = 0;
	unsigned int cnt = 0;

	if( fr == NULL ) {
		cerr << "Cannot open file " << filename << endl;
		return false;
	}

	while( getline(&str, &len, fr) != -1 ) {
		remove_newline(&str);
		string line(str);
		++cnt;

		#ifdef DEBUG
		cout << "Line: " << line << "...";
		#endif

		if(cnt == 1) {
			#ifdef DEBUG
			cout << endl;
			#endif

			continue;
		}

		row matrix_line = split_line(line);
		#ifdef DEBUG
		cout << "pushed" << endl;
		#endif
		(*m).push_back(matrix_line);
	}

	free(str);
	fclose(fr);
	return true;
}

/* Read both matrices */
bool read_matrices() {
	return (read_matrix("mat1", &matA) && 
			read_matrix("mat2", &matB));
} 

/* Get specific row */
row get_line(matrix *m, int index) {
	int cnt = 0;

	matrix::iterator i;
	for(i = (*m).begin(); i != (*m).end(); ++i) {
		if(cnt == index) {
			return row(*i);
		}
		++cnt;
	}

	cerr << "Get row failed for index " << index << endl;
	return row();
}

/* Get specific column */
row get_column(matrix *m, int index) {
	int cnt;
	row ret;

	matrix::iterator i;
	for(i = (*m).begin(); i != (*m).end(); ++i) {
		
		row::iterator r;
		cnt = 0;
		for(r = (*i).begin(); r != (*i).end(); ++r) {
			if(cnt == index) {
				ret.push_back(*r);
			}
			++cnt;
		}
	}

	if(ret.size() > 0){
		return ret;
	}
	else{
		cerr << "Get column failed for index " << index << endl;
		return ret;
	}
}

/* Count elements in matrix */
int elements_count(matrix *m) {
	matrix::iterator i;
	int cnt = 0;
	for(i = (*m).begin(); i != (*m).end(); ++i) {
		row::iterator r;
		for(r = (*i).begin(); r != (*i).end(); ++r) {
			++cnt;
		}
	}

	return cnt;
}

int get_rows() {
	return matA.size();
}

int get_cols() {
	matB.front().size();
}

int get_rows(matrix m) {
	matrix::iterator i;
	int rows = 0;

	for(i = m.begin(); i != m.end(); ++i) {
		++rows;
	}

	return rows;
}

int get_cols(matrix m) {
	row::iterator i;
	int cols = 0;

	for(i = m.front().begin(); i != m.front().end(); ++i) {
		++cols;
	}

	return cols;
}

/* Convert array of integers into row type */
row array_to_row(int *array, int size) {
	row r;
	for(int i = 0; i < size; ++i) {
		r.push_back(array[i]);
	}

	return r;
}

/* Receive vector from another process */
row receive_vector() {
	int number_amount, *received;
	MPI_Status stat;
	row ret;

	MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
	MPI_Get_count(&stat, MPI_INT, &number_amount);

	received = (int *) malloc(number_amount * sizeof(int));
	MPI_Recv(received, number_amount, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
	
	ret = array_to_row(received, number_amount);

	free(received);

	return ret;
}

/* Measure elapsed time */
void timer(){
	static double timer = 0;

	if(timer == 0){
		timer = MPI_Wtime();
	}
	else{
		#ifdef TIME
			printf("%d\n",(int) ((MPI_Wtime() - timer) * 1000000000));
		#else
			;
		#endif
	}
}

void swapvals( int *a, int *b ) {
	int c;
	c = *a;
	*a = *b;
	*b = c;
}

int main(int argc, char **argv) {
	int myid, numprocs;	
	int rows, cols;
	row my_row;
	MPI_Status stat;
	int sum = 0;

	/* OpenMPI Initialization */
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	/* Read matrices */
	if(myid == 0) {
		read_matrices();

		rows = get_rows();
		cols = get_cols();

		#ifdef DEBUG
		cout << "Rows " << rows << " cols " << cols << endl;
		cout << "MatA rows " << get_rows(matA) << " cols " << get_cols(matA) << endl;
		cout << "MatB rows " << get_rows(matB) << " cols " << get_cols(matB) << endl;
		#endif
		
	}

	/* Broadcast n and k values */
	MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);


	/* Synchronize all processes */
	MPI_Barrier(MPI_COMM_WORLD);

	if(myid == 0) {
		row a, b;

		/* Propagate vectors to vertical processes */
		int index = 1;
		for(int i = cols; i < numprocs; i += cols) {
			row r = get_line(&matA, index);
			//cout << "Sending line " << index << " to proc " << i << endl;
			MPI_Send(&r.front(), r.size(), MPI_INT, i, VER_TAG, MPI_COMM_WORLD);
			++index;
		}

		/* Propagate vectors to horizontal processes */
		index = 1;
		for(int i = 1; i < cols; ++i) {
			row r = get_column(&matB, index);
			MPI_Send(&r.front(), r.size(), MPI_INT, i, HOR_TAG, MPI_COMM_WORLD);
			++index;
		}

		a = get_line(&matA, 0);
		b = get_column(&matB, 0);

		if( a.size() != b.size() ) {
			cerr << "Matrices cannot be multiplied" << endl;
			cerr << "A.size is " << a.size() << " B.size is " << b.size() << endl;
			return 1;
		}

		/* Start timer */
		timer();

		for(int i = 0; i < a.size(); ++i) {
			int ai = a.at(i);
			if(numprocs > 1){
				MPI_Send(&ai, 1, MPI_INT, (myid + 1), HOR_TAG, MPI_COMM_WORLD);
			}
		}

		for(int i = 0; i < b.size(); ++i) {
			int bi = b.at(i);
			if(numprocs > cols) {
				MPI_Send(&bi, 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
			}
		}

		for(int i = 0; i < a.size(); ++i) {
			int ai = a.at(i);
			int bi = b.at(i);

			sum += (a.at(i) * b.at(i));
		}
	}
	/* First column but root process */
	else if(myid % cols == 0) {
		my_row = receive_vector();
		int recv;

		/* Receive value, add multiplication to sum and send to another process */
		for(int i = 0; i < my_row.size(); ++i) {
			int number_amount, flag;
			MPI_Recv(&recv, 1, MPI_INT, (myid - cols), VER_TAG, MPI_COMM_WORLD, &stat);
			sum += (my_row.at(i) * recv);

			if(myid + cols < numprocs) {
				MPI_Send(&recv, 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
				
			}
			if(cols > 1)
				MPI_Send(&my_row.at(i), 1, MPI_INT, (myid + 1), HOR_TAG, MPI_COMM_WORLD);
		}
	}
	/* First row but root process */
	else if(myid < cols) {
		my_row = receive_vector();
		int recv;

		/* Receive value, add multiplication to sum and send to another process */
		for(int i = 0; i < my_row.size(); ++i) {
			MPI_Recv(&recv, 1, MPI_INT, (myid - 1), HOR_TAG, MPI_COMM_WORLD, &stat);
			sum += (my_row.at(i) * recv);

			if(myid + 1 < cols) {
				MPI_Send(&recv, 1, MPI_INT, (myid + 1), HOR_TAG, MPI_COMM_WORLD);
				
			}
			if(rows > 1)
				MPI_Send(&my_row.at(i), 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
		}

		if(rows > 1)
			MPI_Send(&last_number, 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
	}
	/* All other processes */
	else {
		int a, b, number_amount, flag;

		/* Receive value, add multiplication to sum and send to another process */
		while(true) {
			MPI_Recv(&b, 1, MPI_INT, (myid - cols), VER_TAG, MPI_COMM_WORLD, &stat);
			if(b == last_number && myid + cols < numprocs) {
				MPI_Send(&last_number, 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
				break;
			}
			else if(b == last_number) {
				break;
			}
			MPI_Recv(&a, 1, MPI_INT, (myid - 1), HOR_TAG, MPI_COMM_WORLD, &stat);

			sum += (a*b);

			if(((myid % cols) + 1) < cols) {
				MPI_Send(&a, 1, MPI_INT, (myid + 1), HOR_TAG, MPI_COMM_WORLD);
			}
			if(myid + cols < numprocs) {
				MPI_Send(&b, 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
			}
		}
	}

	/* Receive results and print matrix */
	MPI_Barrier(MPI_COMM_WORLD);
	if(myid == 0) {
		int r, c;
		r = c = 0;

		my_row.clear();
		my_row.push_back(sum);
		for(int i = 1; i < numprocs; ++i) {
			int recv;
			MPI_Recv(&recv, 1, MPI_INT, i, RES_TAG, MPI_COMM_WORLD, &stat);
			if(i % cols == 0) {
				matC.push_back(my_row);
				my_row.clear();
				++r;
			}
			my_row.push_back(recv);
			
		}

		#ifdef OUT
		++r;
		printf("%d:%d\n", r, cols);
		matC.push_back(my_row);
		print_matrix(matC);
		#endif

		/* End timer and print time elapsed */
		timer();
	}
	/* First column */
	else {
		MPI_Send(&sum, 1, MPI_INT, 0, RES_TAG, MPI_COMM_WORLD);
	}

	/* Tell OpenMPI that there are no OpenMPI calls after this */
	MPI_Finalize();

	return 0;
}