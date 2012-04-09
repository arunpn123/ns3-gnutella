#include "FileDownloadContainer.h"
#include "Peer.h"
#include <map>

void FileDownloadContainer::AddDownload(Peer *p, std::string fn, int fi)
{
	FileInfo f;
	f.file_name = fn;
	f.file_index = fi;
    map_[p] = f;
}

std::string FileDownloadContainer::GetFileName(Peer *p)
{
    std::map<Peer *, FileInfo>::iterator it = map_.find(p);
    if (it == map_.end())
        return "";
    return (it->second).file_name;    
}

int FileDownloadContainer::GetIndex(Peer *p)
{
	std::map<Peer *, FileInfo>::iterator it = map_.find(p);
    if (it == map_.end())
        return -1;
    return (it->second).file_index;
}

void FileDownloadContainer::RemoveDownload(Peer *p)
{
    map_.erase(p);
}
