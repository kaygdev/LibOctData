#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "date.h"

namespace OctData
{
	class SloImage;
	class BScan;

	class Series
	{
		Series(const Series&)            = delete;
		Series& operator=(const Series&) = delete;

	public:
		enum class Laterality { undef, OD, OS };
		enum class ScanPattern { Unknown, SingleLine, Circular, Volume, FastVolume, Radial };
		typedef std::vector<BScan*> BScanList;

		explicit Series(int internalId);
		~Series();

		const SloImage& getSloImage() const                         { return *sloImage; }
		void takeSloImage(SloImage* sloImage);
		const BScanList getBScans() const                           { return bscans;    }
		const BScan* getBScan(std::size_t pos) const;
		std::size_t bscanCount() const                              { return bscans.size(); }

		void setLaterality(Laterality l)                            { laterality = l; }
		Laterality getLaterality() const                            { return laterality; }

		void setScanPattern(ScanPattern p)                          { scanPattern = p;    }
		ScanPattern getScanPattern() const                          { return scanPattern; }
		
		const Date& getScanDate() const                             { return scanDate; }
		void setScanDate(const Date& time)                          { scanDate = time; }
		
		const std::string& getSeriesUID() const                     { return seriesUID; }
		void setSeriesUID(const std::string& uid)                   { seriesUID = uid;  }
		
		const std::string& getRefSeriesUID() const                  { return refSeriesID; }
		void setRefSeriesUID(const std::string& uid)                { refSeriesID = uid;  }

		void takeBScan(BScan* bscan);

		int getInternalId() const                                      { return internalId; }
	private:
		const int internalId;

		SloImage*                               sloImage = nullptr;
		std::string                             seriesUID;
		std::string                             refSeriesID;

		Laterality                              laterality = Laterality::undef;
		ScanPattern                             scanPattern = ScanPattern::Unknown;
		Date                                    scanDate;

		BScanList                               bscans;
	};

}
