#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Basic vector class inspired by the STL version. It does only what is needed in this library and ommits safety checks for performance reasons.

namespace hax {

	template<typename T>
	class Vector {
	private:
		T* _data;
		size_t _size;
		size_t _capacity;

	public:
		Vector() = delete;

		Vector(const Vector&) = delete;
		Vector& operator=(const Vector&) = delete;

		Vector(Vector&&) = delete;
		Vector& operator=(Vector&&) = delete;


		// Initializes a vector with an initial capacity.
		// 
		// Parameters:
		// 
		// [in] capacity:
		// The initial capacity.
		Vector(size_t capacity) : _data{}, _size{}, _capacity{} {
			this->reserve(capacity);

			return;
		}


		~Vector() {
			this->destruct();
			free(this->_data);

			return;
		}


		T* operator[](size_t i) const {
			
			return &this->_data[i];
		}


		size_t  data() const {

			return this->_data;
		}


		size_t  size() const {

			return this->_size;
		}


		size_t  capacity() const {

			return this->_capacity;
		}


		// Increases the capacity of the vector if the desired capacity is greater than the current one.
		// 
		// Parameters:
		// 
		// [in] capacity:
		// The desired capacity.
		void reserve(size_t capacity) {

			if (capacity <= this->_capacity) return;

			T* data = reinterpret_cast<T*>(calloc(capacity, sizeof(T)));

			if (this->_data && this->_size) {
				memcpy(data, this->_data, this->_size * sizeof(T));
				free(this->_data);
			}

			this->_data = data;
			this->_capacity = capacity;

			return;
		}


		// Appends an element to the vector.
		// 
		// Parameters:
		// 
		// [in] t:
		// The element to append.
		void append(T t) {

			if (this->_size == this->_capacity) {
				this->reserve(this->_capacity + this->_capacity / 2u);
			}

			this->_data[this->_size] = t;
			this->_size++;

			return;
		}


		// Destructs all elements in the vector.
		void destruct() {

			for (size_t i = 0u; i < this->_size; i++) {
				this->_data[i].~T();
			}

			return;
		}

	};
}