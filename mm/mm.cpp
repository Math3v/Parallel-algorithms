/*
*	Paralel Mesh Multiplication
*	Author: Matej Minarik XMINAR29
*	File: mm.cpp
*/

#define DEBUG

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
	/*if(matA.size() > matA.front().size()){
		return matA.size();
	}
	else {
		return matA.front().size();
	}*/
	return matA.size();
}

int get_cols() {
	/*if(matB.size() > matB.front().size()){
		return matB.size();
	}
	else {
		return matB.front().size();
	}*/
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

row array_to_row(int *array, int size) {
	row r;
	for(int i = 0; i < size; ++i) {
		r.push_back(array[i]);
	}

	return r;
}

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

		cout << "MatA rows " << get_rows(matA) << " cols " << get_cols(matA) << endl;
		cout << "MatB rows " << get_rows(matB) << " cols " << get_cols(matB) << endl;

		/* TODO: Swap matrices if needed */	
		//if matA.rows == rows && matB.cols == cols
		//then DO NOT SWAP?
		//else if otherwise then swap
		//else WTF???
		if(get_rows(matA) == rows && get_cols(matB) == cols){
			cout << "Do not need swapping" << endl;
		}
		else if(get_rows(matB) == rows && get_cols(matA) == cols) {
			/* Swap matrices */
			cout << "Swapping needed" << endl;
			//swap(matA, matB);
		}
		else {
			/* Something is wrong */
			assert(3 == 2);
		}
	}

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
			//cout << "Sending column " << index << " to proc " << i << endl;
			MPI_Send(&r.front(), r.size(), MPI_INT, i, HOR_TAG, MPI_COMM_WORLD);
			++index;
		}

		a = get_line(&matA, 0);
		b = get_column(&matB, 0);

		assert(a.size() == b.size());

		for(int i = 0; i < a.size(); ++i) {
			int ai = a.at(i);
			int bi = b.at(i);

			cout << "Proc " << myid << " sending " << a.at(i) << " to " << (myid + 1) << endl;
			MPI_Send(&ai, 1, MPI_INT, (myid + 1), HOR_TAG, MPI_COMM_WORLD);
			cout << "Proc " << myid << " sending " << b.at(i) << " to " << (myid + cols) << endl;
			MPI_Send(&bi, 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);

			sum += (a.at(i) * b.at(i));
		}
	}
	/* First column but root process */
	else if(myid % cols == 0) {
		my_row = receive_vector();
		cout << "Proc " << myid << " received vector " << my_row.size();
		print_vector(my_row);
		int recv;

		for(int i = 0; i < my_row.size(); ++i) {
			int number_amount, flag;
			cout << "Proc " << myid << " waiting from " << (myid - cols) << endl;
			//MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
			//MPI_Get_count(&stat, MPI_INT, &number_amount);
			//cout << "Proc " << myid << " incoming " << number_amount << " numbers at " << stat.MPI_TAG << endl;
			MPI_Recv(&recv, 1, MPI_INT, (myid - cols), VER_TAG, MPI_COMM_WORLD, &stat);
			sum += (my_row.at(i) * recv);

			if(myid + cols < numprocs) {
				cout << "Proc " << myid << " sending " << recv << " to " << (myid + cols) << endl;
				MPI_Send(&recv, 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
				
			}
			cout << "Proc " << myid << " sending " << my_row.at(i) << " to " << (myid + 1) << endl;
			MPI_Send(&my_row.at(i), 1, MPI_INT, (myid + 1), HOR_TAG, MPI_COMM_WORLD);
		}
	}
	/* First row but root process */
	else if(myid < cols) {
		my_row = receive_vector();
		cout << "Proc " << myid << " received vector ";
		print_vector(my_row);
		int recv;

		for(int i = 0; i < my_row.size(); ++i) {
			cout << "Proc " << myid << " waiting from " << (myid - 1) << endl;
			MPI_Recv(&recv, 1, MPI_INT, (myid - 1), HOR_TAG, MPI_COMM_WORLD, &stat);
			sum += (my_row.at(i) * recv);

			if(myid + 1 < cols) {
				cout << "Proc " << myid << " sending " << recv << " to " << (myid + 1) << endl;
				MPI_Send(&recv, 1, MPI_INT, (myid + 1), HOR_TAG, MPI_COMM_WORLD);
				
			}
			cout << "Proc " << myid << " sending " << my_row.at(i) << " to " << (myid + cols) << endl;
			MPI_Send(&my_row.at(i), 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
		}

		cout << "Proc " << myid << " sending " << last_number << " to " << (myid + cols) << endl;
		MPI_Send(&last_number, 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
	}
	/* All other processes */
	else {
		int a, b, number_amount, flag;
		while(true) {
			/*MPI_Iprobe(myid - cols, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
			if(!flag){
				continue;
			}
			//MPI_Get_count(&stat, MPI_INT, &number_amount);
			//cout << "Proc " << myid << " incoming " << number_amount << " numbers at " << stat.MPI_TAG << endl;
			if(stat.MPI_TAG == HOR_TAG) {
				cout << "Proc " << myid << " waiting from " << (myid - 1) << endl;
				MPI_Recv(&a, 1, MPI_INT, (myid - 1), HOR_TAG, MPI_COMM_WORLD, &stat);
			}
			else if(stat.MPI_TAG == VER_TAG) {
				MPI_Recv(&b, 1, MPI_INT, (myid - cols), VER_TAG, MPI_COMM_WORLD, &stat);
			}
			else {
				assert(3 == 2);
			}
			*/

			MPI_Recv(&b, 1, MPI_INT, (myid - cols), VER_TAG, MPI_COMM_WORLD, &stat);
			if(b == last_number && myid + cols < numprocs) {
				cout << "Proc " << myid << " sending " << last_number << " to " << (myid + cols) << endl;
				MPI_Send(&last_number, 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
				break;
			}
			else if(b == last_number) {
				break;
			}
			MPI_Recv(&a, 1, MPI_INT, (myid - 1), HOR_TAG, MPI_COMM_WORLD, &stat);

			sum += (a*b);

			if(((myid % cols) + 1) < cols) {
				cout << "Proc " << myid << " sending " << a << " to " << (myid + 1) << endl;
				MPI_Send(&a, 1, MPI_INT, (myid + 1), HOR_TAG, MPI_COMM_WORLD);
			}
			if(myid + cols < numprocs) {
				cout << "Proc " << myid << " sending " << b << " to " << (myid + cols) << endl;
				MPI_Send(&b, 1, MPI_INT, (myid + cols), VER_TAG, MPI_COMM_WORLD);
			}
		}
	}


	//MPI_Barrier(MPI_COMM_WORLD);
	//cout << "Process " << myid << " rows " << rows << " cols " << cols << endl;
	cout << "Process " << myid << " result " << sum << endl;

	MPI_Barrier(MPI_COMM_WORLD);
	if(myid == 0) {
		my_row.clear();
		my_row.push_back(sum);
		for(int i = 1; i < numprocs; ++i) {
			int recv;
			MPI_Recv(&recv, 1, MPI_INT, i, RES_TAG, MPI_COMM_WORLD, &stat);
			if(i % cols == 0) {
				matC.push_back(my_row);
				my_row.clear();
			}
			my_row.push_back(recv);
			
		}

		matC.push_back(my_row);
		print_matrix(matC);
	}
	/* First column */
	else {
		MPI_Send(&sum, 1, MPI_INT, 0, RES_TAG, MPI_COMM_WORLD);
	}

	/* Tell OpenMPI that there are no OpenMPI calls after this */
	MPI_Finalize();

	return 0;
}