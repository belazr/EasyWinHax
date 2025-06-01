#include "FileLoader.h"
#include <stdio.h>

namespace hax {
	
	FileLoader::FileLoader(const char* path):
		_path{ path }, _bytes{}, _errno{}
	{
		FILE* pFile = nullptr;
		this->_errno = fopen_s(&pFile, this->_path, "rb");
		
		if (this->_errno) return;

		this->_errno = fseek(pFile, 0l, SEEK_END);

		if (this->_errno) return;

		this->_bytes.resize(ftell(pFile));
		this->_errno = fclose(pFile);

		return;
	}


	bool FileLoader::readBytes() {
		FILE* pFile = nullptr;
		this->_errno = fopen_s(&pFile, this->_path, "rb");
		
		if (this->_errno) return false;

		fread_s(this->_bytes.data(), this->_bytes.size(), sizeof(BYTE), this->_bytes.size(), pFile);
		
		this->_errno = fclose(pFile);
		
		if (this->_errno) return false;

		return true;
	}


	BYTE* FileLoader::data() const {

		return this->_bytes.data();
	}


	errno_t FileLoader::lastErrno() const {
		
		return this->_errno;
	}


	const char* FileLoader::path() const {
		
		return this->_path;
	}


	size_t FileLoader::size() const {
		
		return this->_bytes.size();
	}

}