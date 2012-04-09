#ifndef FILE_H
#define FILE_H

#include "ns3/buffer.h"

// Data is represented by ns3::Buffer
// File names are std::strings
class File
{
	public:
		File(std::string name, ns3::Buffer data);
		std::string GetFileName();
		ns3::Buffer GetData();
		void SetFileName(std::string name);
		void SetData(ns3::Buffer data);
	private:
		std::string name_;
		ns3::Buffer data_;
};

class FileContainer
{
	public:
		uint32_t GetTotalBytes(); // Combined size of all files
		uint32_t GetNumberOfFiles();
		void AddFile(File* file);
		bool RemoveFile(File* file);
		// Return an empty vector if no file is found
		std::vector<File*> GetFileByName(std::string filename);
		std::vector<File*> GetAllFiles();
                File* GetFileByIndex(size_t index);
		int GetIndexByFile(File* f); // return index of a file, if found, otherwise return -1
	private:
		std::vector<File*> file_vector_;
};

#endif
