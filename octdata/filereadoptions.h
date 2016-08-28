#pragma once

namespace OctData
{
	struct FileReadOptions
	{
		enum class E2eGrayTransform { nativ, xml, vol };

		bool fillEmptyPixelWhite = true;
		bool registerBScanns     = true;

		E2eGrayTransform e2eGray = E2eGrayTransform::xml;
	};
}