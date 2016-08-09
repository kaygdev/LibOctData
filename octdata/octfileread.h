#pragma once

#include <vector>
#include <string>

#include "octextension.h"

namespace boost { namespace filesystem { class path; } }

namespace OctData
{
	class OCT;
	class OctFileReader;

	class OctFileRead
	{
		friend class OctFileReader;
	public:
		typedef std::vector<OctExtension> ExtensionsList;
		static OctFileRead& getInstance()                        { static OctFileRead instance; return instance; }

		static const ExtensionsList& supportedExtensions()  ;//     { return getInstance().extensions; };
		static OCT openFile(const std::string& filename);

	protected:
		void registerFileRead(OctFileReader* reader, const OctExtension& ext);


	private:
		OctFileRead();
		~OctFileRead();

		ExtensionsList extensions;

		std::vector<OctFileReader*> fileReaders;
	};
	
}

