#include "volread.h"
#include <datastruct/oct.h>
#include <datastruct/coordslo.h>
#include <datastruct/sloimage.h>
#include <datastruct/bscan.h>

#include <iostream>
#include <fstream>
#include <iomanip>

#include <chrono>
#include <ctime>

#include <opencv2/opencv.hpp>

#include <boost/filesystem.hpp>

#include "../../octdata_packhelper.h"
#include <filereadoptions.h>


namespace bfs = boost::filesystem;


namespace
{
	template<typename T>
	void readFStream(std::istream& stream, T* dest, std::size_t num = 1)
	{
		stream.read(reinterpret_cast<char*>(dest), sizeof(T)*num);
	}

	void readCVImage(std::istream& stream, cv::Mat& image, int cvFormat, int factor, std::size_t sizeX, std::size_t sizeY)
	{
		image = cv::Mat(static_cast<int>(sizeX), static_cast<int>(sizeY), cvFormat);

		std::size_t num = sizeX*sizeY;

		stream.read(reinterpret_cast<char*>(image.data), num*factor);
	}

	struct VolHeader
	{
		PACKSTRUCT(struct RawData
		{
			// char HSF-OCT-        [ 8]; // check first in function
			char     version     [ 4];
			uint32_t sizeX           ;
			uint32_t numBScans       ;
			uint32_t sizeZ           ;
			double   scaleX          ;
			double   distance        ;
			double   scaleZ          ;
			uint32_t sizeXSlo        ;
			uint32_t sizeYSlo        ;
			double   scaleXSlo       ;
			double   scaleYSlo       ;
			uint32_t fieldSizeSlo    ;
			double   scanFocus       ;
			char     scanPosition[ 4];
			uint64_t examTime        ;
			uint32_t scanPattern     ;
			uint32_t bScanHdrSize    ;
			char     id          [16];
			char     referenceID [16];
			uint32_t pid             ;
			char     patientID   [21];
			char     padding     [ 3];
			double   dob             ; // Patient date of birth
			uint32_t vid             ;
			char     visitID     [24];
			double   visitDate       ;
			int32_t  gridType        ;
			int32_t  gridOffset      ;
			char     spare       [ 8];
			char     progID      [32];
		});
		RawData data;

		void printData(std::ostream& stream)
		{
			stream << "version     : " << data.version      << '\n';
			stream << "sizeX       : " << data.sizeX        << '\n';
			stream << "numBScans   : " << data.numBScans    << '\n';
			stream << "sizeZ       : " << data.sizeZ        << '\n';
			stream << "scaleX      : " << data.scaleX       << '\n';
			stream << "distance    : " << data.distance     << '\n';
			stream << "scaleZ      : " << data.scaleZ       << '\n';
			stream << "sizeXSlo    : " << data.sizeXSlo     << '\n';
			stream << "sizeYSlo    : " << data.sizeYSlo     << '\n';
			stream << "scaleXSlo   : " << data.scaleXSlo    << '\n';
			stream << "scaleYSlo   : " << data.scaleYSlo    << '\n';
			stream << "fieldSizeSlo: " << data.fieldSizeSlo << '\n';
			stream << "scanFocus   : " << data.scanFocus    << '\n';
			stream << "scanPosition: " << data.scanPosition << '\n';
			stream << "examTime    : " << data.examTime     << '\t' << OctData::Date::fromWindowsTicks(data.examTime).timeDateStr() << '\n';
			stream << "scanPattern : " << data.scanPattern  << '\n';
			stream << "bScanHdrSize: " << data.bScanHdrSize << '\n';
			stream << "id          : " << data.id           << '\n';
			stream << "referenceID : " << data.referenceID  << '\n';
			stream << "pid         : " << data.pid          << '\n';
			stream << "patientID   : " << data.patientID    << '\n';
			stream << "padding     : " << data.padding      << '\n';
			stream << "dob         : " << data.dob          << '\t' << OctData::Date::fromWindowsTimeFormat(data.dob).timeDateStr()<< '\n';
			stream << "vid         : " << data.vid          << '\n';
			stream << "visitID     : " << data.visitID      << '\n';
			stream << "visitDate   : " << data.visitDate    << '\t' << OctData::Date::fromWindowsTimeFormat(data.visitDate).timeDateStr()<< '\n';
			stream << "gridType    : " << data.gridType     << '\n';
			stream << "gridOffset  : " << data.gridOffset   << '\n';
			stream << "progID      : ";
			for(std::size_t i=0; i<sizeof(data.progID) && data.progID[i] != 0; ++i)
				stream << data.progID[i];
			stream << std::endl;
		}

		std::size_t getSLOPixelSize() const   {
			return data.sizeXSlo*data.sizeYSlo;
		}
		std::size_t getBScanPixelSize() const {
			return data.sizeX   *data.sizeZ   *sizeof(float);
		}
		std::size_t getBScanSize() const      {
			return getBScanPixelSize() + data.bScanHdrSize;
		}

		constexpr static std::size_t getHeaderSize() {
			return 2048;
		}

	};

	struct BScanHeader
	{
		PACKSTRUCT(struct Data
		{
			double  startX ;
			double  startY ;
			double  endX   ;
			double  endY   ;
			int32_t numSeg ;
			int32_t offSeg ;
			float   quality;
			int32_t shift  ;
		});

		Data data;

		void printData() const
		{
			std::cout << data.startX  << '\t'
			          << data.startY  << '\t'
			          << data.endX    << '\t'
			          << data.endY    << '\t'
			          << data.numSeg  << '\t'
			          << data.offSeg  << '\t'
			          << data.quality << '\t'
			          << data.shift   << std::endl;

		}

	};

	void copyMetaData(const VolHeader& header, OctData::Patient& pat, OctData::Study& study, OctData::Series& series)
	{
		// Patient data
		pat.setId(header.data.patientID);
		pat.setBirthdate(OctData::Date::fromWindowsTimeFormat(header.data.dob));

		// Study data
		study.setStudyDate(OctData::Date::fromWindowsTicks(header.data.examTime));
		
		// Series data
		series.setScanDate(OctData::Date::fromWindowsTimeFormat(header.data.visitDate));
		if(strcmp("OD", header.data.scanPosition) == 0)
			series.setLaterality(OctData::Series::Laterality::OD);
		else if(strcmp("OS", header.data.scanPosition) == 0)
			series.setLaterality(OctData::Series::Laterality::OS);
		
		series.setSeriesUID   (header.data.id);
		series.setRefSeriesUID(header.data.referenceID);
		
		// series.setScanDate(OctData::Date::fromWindowsTicks(data.examTime));

		switch(header.data.scanPattern)
		{
			case 1:
				series.setScanPattern(OctData::Series::ScanPattern::SingleLine);
				break;
			case 2:
				series.setScanPattern(OctData::Series::ScanPattern::Circular);
				break;
			case 3:
				series.setScanPattern(OctData::Series::ScanPattern::Volume);
				break;
			case 4:
				series.setScanPattern(OctData::Series::ScanPattern::FastVolume);
				break;
			case 5:
				series.setScanPattern(OctData::Series::ScanPattern::Radial);
				break;
			default:
				series.setScanPattern(OctData::Series::ScanPattern::Unknown);
				break;
		}

		series.setScanFocus(header.data.scanFocus);
	}

}



namespace OctData
{

	VOLRead::VOLRead()
	: OctFileReader(OctExtension(".vol", "Heidelberg Engineering Raw File"))
	{
	}

	bool VOLRead::readFile(const boost::filesystem::path& file, OCT& oct, const FileReadOptions& op)
	{
		if(file.extension() != ".vol")
			return false;

		if(!bfs::exists(file))
			return false;


		std::string dir      = file.branch_path().generic_string();
		std::string filename = file.filename().generic_string();

		std::fstream stream(file.generic_string(), std::ios::binary | std::ios::in);

		if(!stream.good())
			return false;

		const std::size_t formatstringlength = 8;
		char fileformatstring[formatstringlength];
		readFStream(stream, fileformatstring, formatstringlength);
		if(memcmp(fileformatstring, "HSF-OCT-", formatstringlength) != 0) // 0 = strings are equal
			return false;

		VolHeader volHeader;

		readFStream(stream, &(volHeader.data));
		// volHeader.printData(std::cout);
		stream.seekg(VolHeader::getHeaderSize());

		Patient& pat    = oct.getPatient(volHeader.data.pid);
		Study&   study  = pat.getStudy(volHeader.data.vid);
		Series&  series = study.getSeries(1); // TODO


		copyMetaData(volHeader, pat, study, series);

		/*
		time_t time = std::chrono::system_clock::to_time_t(series.getTime());
		struct tm* tmw = std::localtime(&time);
		std::cout << (tmw->tm_mday) << "." << (tmw->tm_mon+1) << "." << (tmw->tm_year + 1900) << std::endl;
		*/

		// std::cerr << "oct.size() : " << oct.size() << '\n';

		// Read SLO
		cv::Mat sloImage;
		int sloCvFormat = CV_8UC1;
		int sloFactor   = 1;
		readCVImage(stream, sloImage, sloCvFormat, sloFactor, volHeader.data.sizeXSlo, volHeader.data.sizeYSlo);

		SloImage* slo = new SloImage;
		slo->setImage(sloImage);
		slo->setScaleFactor(ScaleFactor(volHeader.data.scaleXSlo, volHeader.data.scaleYSlo));
		series.takeSloImage(slo);

		// Read BScann
		for(std::size_t numBscan = 0; numBscan<volHeader.data.numBScans; ++numBscan)
		{
			BScanHeader bscanHeader;

			std::size_t bscanPos = VolHeader::getHeaderSize() + volHeader.getSLOPixelSize() + numBscan*volHeader.getBScanSize();

			stream.seekg(16+bscanPos);
			readFStream(stream, &(bscanHeader.data));

			// bscanHeader.printData();

			stream.seekg(volHeader.data.bScanHdrSize+bscanPos);
			cv::Mat bscanImage;
			cv::Mat bscanImagePow;
			cv::Mat bscanImageConv;
			int cvFormat = CV_32FC1;
			int factor   = sizeof(float);
			readCVImage(stream, bscanImage, cvFormat, factor, volHeader.data.sizeZ, volHeader.data.sizeX);

			if(op.fillEmptyPixelWhite)
				cv::threshold(bscanImage, bscanImage, 1.0, 1.0, cv::THRESH_TRUNC); // schneide hohe werte ab, sonst: bei der konvertierung werden sie auf 0 gesetzt
			cv::pow(bscanImage, 0.25, bscanImagePow);
			bscanImagePow.convertTo(bscanImageConv, CV_8U, 255, 0);

			BScan::Data bscanData;
			bscanData.start       = CoordSLOmm(bscanHeader.data.startX, bscanHeader.data.startY);
			bscanData.end         = CoordSLOmm(bscanHeader.data.endX  , bscanHeader.data.endY  );
			bscanData.scaleFactor = ScaleFactor(volHeader.data.scaleZ, volHeader.data.scaleX);

			bscanData.imageQuality = bscanHeader.data.quality;
			// fseek( fid, 256+2048+(header.SizeXSlo*header.SizeYSlo)+(ii*(header.BScanHdrSize+header.SizeX*header.SizeZ*4)), -1 );


			// TODO
			stream.seekg(256+bscanPos);
			for(int segNum = 0; segNum < bscanHeader.data.numSeg; ++segNum)
			{
				float value;
				std::vector<double> segVec;
				segVec.reserve(volHeader.data.sizeX);
				for(std::size_t xpos = 0; xpos<volHeader.data.sizeX; ++xpos)
				{
					stream.read(reinterpret_cast<char*>(&value), sizeof(value));
					segVec.push_back(value);
				}
				switch(segNum)
				{
				case 0:
					bscanData.segmentlines.at(static_cast<std::size_t>(BScan::SegmentlineType::ILM)) = segVec;
					break;
				case 1:
					bscanData.segmentlines.at(static_cast<std::size_t>(BScan::SegmentlineType::BM)) = segVec;
					break;

				}
				// bscan->addSegLine(segVec);
			}


			BScan* bscan = new BScan(bscanImageConv, bscanData);
			bscan->setRawImage(bscanImage);
			series.takeBScan(bscan);
		}

		return true;
	}

	VOLRead* VOLRead::getInstance()
	{
		static VOLRead instance;
		return &instance;
	}


}
