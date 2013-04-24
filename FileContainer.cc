#include "ns3/core-module.h"
#include "File.h"
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("GnutellaFileContainer");

uint32_t FileContainer::GetTotalBytes()
{
	uint32_t size = 0;
	for (size_t i = 0; i < file_vector_.size(); i++)
	{
		size += file_vector_[i]->GetData().GetSize();
	}
	return size;
}
uint32_t FileContainer::GetNumberOfFiles()
{
	return file_vector_.size();
}
void FileContainer::AddFile(File* file)
{
	file_vector_.push_back(file);
}
bool FileContainer::RemoveFile(File* file)
{
	std::vector<File*>::iterator it;
	it = std::find(file_vector_.begin(), file_vector_.end(), file);
	if (it != file_vector_.end())
	{
		file_vector_.erase(it);
		return true;
	}
	return false;
}
std::vector<File*> FileContainer::GetFileByName(std::string filename)
{
	std::vector<File*> result;
	
	for (size_t i = 0; i < file_vector_.size(); i++)
	{
//        NS_LOG_INFO ("Filename: " << file_vector_[i]->GetFileName().size());
//        NS_LOG_INFO ("Filename0: " << filename.size());
		if(file_vector_[i]->GetFileName() == filename){
//        NS_LOG_INFO ("Found: " << found);
			result.push_back(file_vector_[i]);
			break;
		}
	}
	return result;
}

std::vector<File*> FileContainer::GetAllFiles()
{
	return file_vector_;
}

int FileContainer::GetIndexByFile(File* f)
{
	for (size_t i = 0; i < file_vector_.size(); i++)
	{
		if (file_vector_[i] == f)
			return i;
	}
	return -1;
}

File* FileContainer::GetFileByIndex(size_t index)
{
    if (index >= file_vector_.size())
        return NULL;
    return file_vector_[index];
}
