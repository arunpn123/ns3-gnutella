#ifndef FILE_DOWNLOAD_CONTAINER_H
#define FILE_DOWNLOAD_CONTAINER_H

#include "Peer.h"

// The purpose of this class is to keep track of what files we're downloading
// from whom, since the HTTP response containing the file data does not have
// the file name.  This assumes only one download from each peer at a time.
class FileDownloadContainer
{
  public:
    void AddDownload(Peer *p, std::string fn, int fi);
    void RemoveDownload(Peer *p);
    std::string GetFileName(Peer *p);
    int GetIndex(Peer *p);
  private:
	struct FileInfo
	{
		std::string file_name;
		int file_index;
	};
    std::map<Peer *, FileInfo> map_;
};

#endif
