#pragma once
#include <Windows.h>

// Class to map files from disk continuously into memory.

namespace hax {

	class FileMapper {
	private:
		const char* _path;
		const BYTE* _data;
		size_t _size;

	public:
		// Initializes members.
		// 
		// Parameters:
		// 
		// [in] path:
		// Path of the file to be mapped.
		FileMapper(const char* path);
		~FileMapper();

		// Maps the file into memory.
		// 
		// Parameters:
		// 
		// [in] image:
		// If the file should be mapped as a PE image. This maps the image sections at the correct offset and resolves relocation of function calls.
		// 
		// Return:
		// ERROR_SUCCESS on success, last WinError on failure.
		DWORD map(bool image = false);
		
		// Unmaps the file.
		void unmap();

		// Gets the path of the file.
		// 
		// Return:
		// The path of the file.
		const char* path() const;

		// Gets a pointer to the file mapping.
		// 
		// Return:
		// Pointer to the file mapping.
		const BYTE* data() const;
		
		// Gets the size of the file in bytes
		// 
		// Return:
		// Size of the file in bytes.
		size_t size() const;
	};

}