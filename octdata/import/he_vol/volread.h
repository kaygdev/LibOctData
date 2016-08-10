#pragma once
#include <string>

#include "../octfilereader.h"

class SLOImage;
class CScan;
class AlgoBatch;

namespace OctData
{

	class VOLRead : public OctFileReader
	{
		VOLRead();
	public:

		static VOLRead* getInstance();

	    virtual bool readFile(const boost::filesystem::path& file, OCT& oct);
	};

}

