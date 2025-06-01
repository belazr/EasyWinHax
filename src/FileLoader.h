#pragma once
#include "Vector.h"
#include <Windows.h>

// Class to load files from disk continuosly into memory.

namespace hax {

	class FileLoader {
	private:
		const char* const _path;
		Vector<BYTE> _bytes;
		errno_t _errno;
	public:
		// Initializes members. Call getError() to check for success.
		// 
		// Parameters:
		// 
		// [in] path:
		// Absolute path of the file to be loaded
		FileLoader(const char* path);

		// Reads the file from disk and writes it to memory
		// 
		// Return:
		// True on success, false on failure. Call getError() to check the reason for failure.
		bool readBytes();

		// Gets a pointer to the array of bytes of the file in memory.
		// 
		// Return:
		// Pointer to the array of bytes of the file in memory.
		BYTE* data() const;

		// Gets the last error returned by the file operation functions. The error code can be converted to a string via strerror_s().
		// 
		// Return:
		// The last error returned by the file operation functions.
		errno_t lastErrno() const;
		
		// Gets the path of the file.
		// 
		// Return:
		// The path of the file.
		const char* path() const;

		// Gets the size of the file in bytes
		// 
		// Return:
		// Size of the file in bytes.
		size_t size() const;
		
		
	};

}