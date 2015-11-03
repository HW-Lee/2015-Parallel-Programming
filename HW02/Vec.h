#include <iostream>
#include <math.h>

#define Vec2(DT) Vec<DT, 2>
#define Vec3(DT) Vec<DT, 3>

using namespace std;

template< class DataType, unsigned int N >
class Vec {
public:
	Vec();
	DataType& operator[]( const int idx );
	void operator=( const Vec<DataType, N>& vec );
	void operator+=( const Vec<DataType, N>& vec );
	void operator-=( const Vec<DataType, N>& vec );
	void operator+=( const DataType& vec );
	void operator-=( const DataType& vec );
	void operator*=( const DataType& scale );
	void operator/=( const DataType& scale );
	Vec<DataType, N> operator+( const Vec<DataType, N>& vec );
	Vec<DataType, N> operator-( const Vec<DataType, N>& vec );
	Vec<DataType, N> operator+( const DataType& v );
	Vec<DataType, N> operator-( const DataType& v );
	Vec<DataType, N> operator*( const DataType& scale );
	Vec<DataType, N> operator/( const DataType& scale );

	template< class T >
	T norm();

	DataType* data;
};

template< class DataType, unsigned int N >
Vec<DataType, N>::Vec() {
	this->data = new DataType[N];
	for (int i = 0; i < N; i++) this->data[i] = 0;
}

template< class DataType, unsigned int N >
template< class T >
T Vec<DataType, N>::norm() {
	T norm = 0;
	for (int i = 0; i < N; i++) norm += (T) (this->data[i] * this->data[i]);
	return sqrt( norm );
}

template< class DataType, unsigned int N >
DataType& Vec<DataType, N>::operator[]( const int idx ) { return this->data[idx]; }

template< class DataType, unsigned int N >
void Vec<DataType, N>::operator=( const Vec<DataType, N>& vec ) {
	for (int i = 0; i < N; i++) this->data[i] = vec.data[i];
}

template< class DataType, unsigned int N >
void Vec<DataType, N>::operator+=( const Vec<DataType, N>& vec ) {
	for (int i = 0; i < N; i++) this->data[i] += vec.data[i];
}

template< class DataType, unsigned int N >
void Vec<DataType, N>::operator-=( const Vec<DataType, N>& vec ) {
	for (int i = 0; i < N; i++) this->data[i] -= vec.data[i];
}

template< class DataType, unsigned int N >
void Vec<DataType, N>::operator+=( const DataType& v ) {
	for (int i = 0; i < N; i++) this->data[i] += v;
}

template< class DataType, unsigned int N >
void Vec<DataType, N>::operator-=( const DataType& v ) {
	for (int i = 0; i < N; i++) this->data[i] -= v;
}

template< class DataType, unsigned int N >
void Vec<DataType, N>::operator*=( const DataType& scale ) {
	for (int i = 0; i < N; i++) this->data[i] *= scale;
}

template< class DataType, unsigned int N >
void Vec<DataType, N>::operator/=( const DataType& scale ) {
	for (int i = 0; i < N; i++) this->data[i] /= scale;
}

template< class DataType, unsigned int N >
Vec<DataType, N> Vec<DataType, N>::operator+( const Vec<DataType, N>& vec ) {
	Vec<DataType, N> _vec;
	for (int i = 0; i < N; i++) _vec.data[i] = this->data[i] + vec.data[i];
	return _vec;
}

template< class DataType, unsigned int N >
Vec<DataType, N> Vec<DataType, N>::operator-( const Vec<DataType, N>& vec ) {
	Vec<DataType, N> _vec;
	for (int i = 0; i < N; i++) _vec.data[i] = this->data[i] - vec.data[i];
	return _vec;
}

template< class DataType, unsigned int N >
Vec<DataType, N> Vec<DataType, N>::operator+( const DataType& v ) {
	Vec<DataType, N> _vec;
	for (int i = 0; i < N; i++) _vec.data[i] = this->data[i] + v;
	return _vec;
}

template< class DataType, unsigned int N >
Vec<DataType, N> Vec<DataType, N>::operator-( const DataType& v ) {
	Vec<DataType, N> _vec;
	for (int i = 0; i < N; i++) _vec.data[i] = this->data[i] - v;
	return _vec;
}

template< class DataType, unsigned int N >
Vec<DataType, N> Vec<DataType, N>::operator*( const DataType& scale ) {
	Vec<DataType, N> _vec;
	for (int i = 0; i < N; i++) _vec.data[i] = scale * this->data[i];
	return _vec;
}

template< class DataType, unsigned int N >
Vec<DataType, N> Vec<DataType, N>::operator/( const DataType& scale ) {
	Vec<DataType, N> _vec;
	for (int i = 0; i < N; i++) _vec.data[i] = this->data[i] / scale;
	return _vec;
}
