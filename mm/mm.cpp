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

/* vector of vectors of integers */
typedef vector<vector<int> > matrix;

typedef vector<int> row;

/* A * B = C*/
matrix matA, matB, matC;

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

int main(int argc, char **argv) {
	int myid, numprocs;	

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

		cout << "Column 2: ";
		print_vector(get_column(&matA, 2));
	}

	/* Synchronize all processes */
	MPI_Barrier(MPI_COMM_WORLD);


	/* Tell OpenMPI that there are no OpenMPI calls after this */
	MPI_Finalize();

	return 0;
}