/*
*	Paralel Mesh Multiplication
*	Author: Matej Minarik XMINAR29
*	File: mm.cpp
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

#define HOR_TAG 0
#define VER_TAG 1
#define COLS_TAG 3
#define ROWS_TAG 4
#define MATA_TAG 5
#define MATB_TAG 6

/* vector of vectors of integers */
typedef vector<vector<int> > matrix;

typedef vector<int> row;

/* A * B = C*/
matrix matA, matB, matC;
const int MAX_NUMBERS = 100000;

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

row split_line(string line) {
	string buf;
	stringstream ss(line);
	row tokens;

	while(ss >> buf) {
		tokens.push_back(atoi(buf.c_str()));
	}

	return tokens;
}

bool read_matrix(const char* filename, matrix *m) {
	ifstream f;
	f.open(filename, ios::in | ios::out );

	if(!f.is_open()) {
		cerr << "Cannot open file " << filename << endl;
		return false;
	}

	string line;
	while(!getline(f, line, '\n').eof()) {

		if(line.length() < 2) {
			continue;
		}

		row matrix_line = split_line(line);
		(*m).push_back(matrix_line);
	}

	f.close();
	return true;
}

bool read_matrices() {
	return (read_matrix("mat1", &matA) && 
			read_matrix("mat2", &matB));
} 

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
	if(matA.size() > matA.front().size()){
		return matA.size();
	}
	else {
		return matA.front().size();
	}
}

int get_cols() {
	if(matB.size() > matB.front().size()){
		return matB.size();
	}
	else {
		return matB.front().size();
	}
}

row array_to_row(int *array, int size) {
	row r;
	for(int i = 0; i < size; ++i) {
		r.push_back(array[i]);
	}

	return r;
}

int main(int argc, char **argv) {
	int myid, numprocs;	
	int rows, cols;
	row my_row;
	MPI_Status stat;
	int number_amount;
	int *received;
	//my_row.reserve(10);

	/* OpenMPI Initialization */
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	if(myid == 0) {
		read_matrices();
		print_matrix(matA);
		print_matrix(matB);
		cout << "Line 0: ";
		print_vector(get_line(&matA, 0));
		cout << "Line 1: ";
		print_vector(get_line(&matA, 1));

		cout << "Column 0: ";
		print_vector(get_column(&matA, 0));

		cout << "Column 1: ";
		print_vector(get_column(&matA, 1));

		cout << "Elements A " << elements_count(&matA) << endl;
		cout << "Elements B " << elements_count(&matB) << endl;

		rows = get_rows();
		cols = get_cols();

		cout << "Rows " << rows << " cols " << cols << endl;
		cout << "MatA.front() " << matA[0][0] << endl;

		/* TODO: Swap matrices if needed */	
		//if matA.rows == rows && matB.cols == cols
		//then DO NOT SWAP?
		//else if otherwise then swap
		//else WTF???
	}

	MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

	/* Synchronize all processes */
	MPI_Barrier(MPI_COMM_WORLD);

	if(myid == 0) {

		/* Propagate vectors to vertical processes */
		int index = 1;
		for(int i = cols; i < numprocs; i += cols) {
			row r = get_line(&matA, index);
			cout << "Sending line " << index << " to proc " << i << endl;
			MPI_Send(&r.front(), r.size(), MPI_INT, i, VER_TAG, MPI_COMM_WORLD);
			++index;
		}

		/* Propagate vectors to horizontal processes */
		index = 1;
		for(int i = 1; i < cols; ++i) {
			row r = get_column(&matB, index);
			cout << "Sending column " << index << " to proc " << i << endl;
			MPI_Send(&r.front(), r.size(), MPI_INT, i, HOR_TAG, MPI_COMM_WORLD);
			++index;
		}
	}
	else if(myid == 4 || myid == 8 /* TODO */) {
		MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
		MPI_Get_count(&stat, MPI_INT, &number_amount);

		received = (int *) malloc(number_amount * sizeof(int));
		cout << "Process " << myid << " received " << number_amount << " numbers at tag " << stat.MPI_TAG << endl;
		MPI_Recv(received, number_amount, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
		
		my_row = array_to_row(received, number_amount);
		cout << "Print vector of process " << myid << " ";
		print_vector(my_row);

		free(received);
	}
	else if(myid >= 1 && myid <= 3 /* TODO */) {
		MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
		MPI_Get_count(&stat, MPI_INT, &number_amount);

		received = (int *) malloc(number_amount * sizeof(int));
		cout << "Process " << myid << " received " << number_amount << " numbers at tag " << stat.MPI_TAG << endl;
		MPI_Recv(received, number_amount, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
		
		my_row = array_to_row(received, number_amount);
		cout << "Print vector of process " << myid << " ";
		print_vector(my_row);

		free(received);
	}


	//MPI_Barrier(MPI_COMM_WORLD);
	cout << "Process " << myid << " rows " << rows << " cols " << cols << endl;

	/* Tell OpenMPI that there are no OpenMPI calls after this */
	MPI_Finalize();

	return 0;
}