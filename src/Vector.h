#pragma once
#include <new>
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
		Vector() : _data{}, _size{}, _capacity{} {}


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


		Vector(Vector&& v) : _data{ v._data }, _size{ v._size }, _capacity{ v._capacity } {
			v._data = nullptr;
			v._size = 0u;
			v._capacity = 0u;
		}


		Vector(const Vector& v) : _data{}, _size{}, _capacity{} {
			
			if (v._data) {
				this->reserve(2 * v._size);

				for (size_t i = 0; i < v._size; i++) {
					this->append(T(v._data[i]));
				}

			}

		}


		Vector& operator=(const Vector& v) {
			
			if (&v == this) return *this;
			
			this->resize(0u);
			
			if (v._data) {
				this->reserve(v.size() * 2u);

				for (size_t i = 0; i < v._size; i++) {
					this->append(T(v._data[i]));
				}

			}

			return *this;
		}


		Vector& operator=(Vector&& v) {

			if (&v == this) return *this;

			this->shrink(this->_size);
			free(this->_data);
			
			this->_data = v._data;
			this->_size = v._size;
			this->_capacity = v._capacity;

			v._data = nullptr;
			v._size = 0u;
			v._capacity = 0u;

			return *this;
		}


		~Vector() {
			this->shrink(this->_size);
			free(this->_data);

			return;
		}


		T* data() const {

			return this->_data;
		}


		size_t size() const {

			return this->_size;
		}


		size_t capacity() const {

			return this->_capacity;
		}


		T& operator[](size_t i) const {

			return this->_data[i];
		}


		T* operator+(size_t i) const {

			return this->_data + i;
		}


		T* addr(size_t i) const {
			
			if (i >= this->_size) return nullptr;

			return this->_data + i;
		}


		// Increases the capacity of the vector if the desired capacity is greater than the current one.
		// 
		// Parameters:
		// 
		// [in] capacity:
		// The desired capacity.
		void reserve(size_t capacity) {

			if (capacity <= this->_capacity) return;

			T* data = nullptr;

			if (this->_data) {
				data = reinterpret_cast<T*>(realloc(this->_data, capacity * sizeof(T)));
			}
			else {
				data = reinterpret_cast<T*>(malloc(capacity * sizeof(T)));
			}

			if (data) {
				this->_data = data;
				this->_capacity = capacity;
			}

			return;
		}


		// Resizes the vector.
		// If the desired size is smaller than the current one, calls the destructors of the excessive elements.
		// If the desired size is greater than the current one, calls the trival constructors of the new elements.
		// 
		// Parameters:
		// 
		// [in] capacity:
		// The desired capacity.
		void resize(size_t size) {

			if (size == this->_size) return;

			if (size > this->_size) {
				this->grow(size - this->_size);
			}
			else {
				this->shrink(this->_size - size);
			}

			return;
		}


		// Appends an element to the vector.
		// 
		// Parameters:
		// 
		// [in] t:
		// The element to append.
		template <typename R>
		void append(R&& t) {

			if (this->_size == this->_capacity) {
				const size_t capacity = this->_capacity ? 2 * this->_capacity : 8u;
				this->reserve(capacity);
			}
			
			// inplace construction to avoid assignment operator calls
			new(&this->_data[this->_size]) T(static_cast<R&&>(t));
			this->_size++;

			return;
		}


		// Grows the size of the vector.
		//		
		// Parameters:
		// 
		// [in] n:
		// Number of elements the vector should be grown by.
		void grow(size_t n) {

			for (size_t i = 0u; i < n; i++) {
				this->append(T());
			}

			return;
		}


		// Shrinks the size of the vector.
		//		
		// Parameters:
		// 
		// [in] n:
		// Number of elements the vector should be shrunk by.
		void shrink(size_t n) {
			const size_t size = n > this->_size ? 0u : this->_size - n;

			for (size_t i = this->_size; i > size; i--) {
				this->_data[i - 1u].~T();
			}

			this->_size = size;

			return;
		}

	};

}