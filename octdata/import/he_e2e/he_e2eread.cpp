#include "he_e2eread.h"

#include <datastruct/oct.h>
#include <datastruct/coordslo.h>
#include <datastruct/sloimage.h>
#include <datastruct/bscan.h>

#include <iostream>
#include <fstream>
#include <iomanip>

#include <opencv2/opencv.hpp>

#include <boost/filesystem.hpp>

#include <E2E/e2edata.h>
#include <E2E/dataelements/patientdataelement.h>
#include <E2E/dataelements/image.h>
#include <E2E/dataelements/segmentationdata.h>
#include <E2E/dataelements/bscanmetadataelement.h>


namespace bfs = boost::filesystem;


namespace OctData
{

	namespace
	{
		void copyPatData(Patient& pat, const E2E::Patient& e2ePat)
		{
			const E2E::PatientDataElement* e2ePatData = e2ePat.getPatientData();
			if(!e2ePatData)
				return;

			pat.setForename(e2ePatData->getForename());
			pat.setSurname (e2ePatData->getSurname ());
			pat.setId      (e2ePatData->getId      ());
			// pat.setSex     (e2ePatData.getSex     ());
			pat.setTitle   (e2ePatData->getTitle   ());
		}

		void copySlo(Series& series, const E2E::Series& e2eSeries)
		{
			const E2E::Image* e2eSlo = e2eSeries.getSloImage();
			if(!e2eSlo)
				return;

			SloImage* slo = new SloImage;

			slo->setShift(CoordSLOpx(e2eSlo->getImageRows()/2., e2eSlo->getImageCols()/2.));
			slo->setImage(e2eSlo->getImage());
			series.takeSloImage(slo);
		}

		void addSegData(BScan::Data& bscanData, BScan::SegmentlineType segType, const E2E::BScan::SegmentationMap& e2eSegMap, std::size_t index, std::size_t type)
		{
			const E2E::BScan::SegmentationMap::const_iterator segPair = e2eSegMap.find(E2E::BScan::SegPair(index, type));
			if(segPair != e2eSegMap.end())
			{
				E2E::SegmentationData* segData = segPair->second;
				if(segData)
				{
					std::vector<double> segVec(segData->begin(), segData->end());
					bscanData.segmentlines.at(static_cast<std::size_t>(segType)) = segVec;
				}
			}
		}

		void copyBScan(Series& series, const E2E::BScan& e2eBScan)
		{
			const E2E::Image* e2eBScanImg = e2eBScan.getImage();
			if(!e2eBScanImg)
				return;


			BScan::Data bscanData;

			const E2E::BScanMetaDataElement* e2eMeta = e2eBScan.getBScanMetaDataElement();
			if(e2eMeta)
			{
				uint32_t factor = 30; // TODO
				bscanData.start = CoordSLOmm(e2eMeta->getX1()*factor, e2eMeta->getY1()*factor);
				bscanData.end   = CoordSLOmm(e2eMeta->getX2()*factor, e2eMeta->getY2()*factor);
			}


			// segmenation lines
			const E2E::BScan::SegmentationMap& e2eSegMap = e2eBScan.getSegmentationMap();
			addSegData(bscanData, BScan::SegmentlineType::ILM, e2eSegMap, 0, 5);
			addSegData(bscanData, BScan::SegmentlineType::BM , e2eSegMap, 1, 2);

			// convert image
			const cv::Mat& e2eImage = e2eBScanImg->getImage();

			cv::Mat dest, bscanImageConv;
			e2eImage.convertTo(dest, CV_32FC1, 1/static_cast<double>(1 << 16), 0);
			cv::pow(dest, 8, dest);
			dest.convertTo(bscanImageConv, CV_8U, 255, 0);


			BScan* bscan = new BScan(bscanImageConv, bscanData);
			bscan->setRawImage(e2eImage);
			series.takeBScan(bscan);
		}
	}


	HeE2ERead::HeE2ERead()
	: OctFileReader({OctExtension("E2E", "Heidelberg Engineering E2E File"), OctExtension("sdb", "Heidelberg Engineering HEYEX File")})
	{
	}

	bool HeE2ERead::readFile(const boost::filesystem::path& file, OCT& oct)
	{
		if(file.extension() != ".E2E" && file.extension() != ".sdb")
			return false;

		if(!bfs::exists(file))
			return false;

		E2E::E2EData e2eData;
		e2eData.readE2EFile(file.generic_string());

		const E2E::DataRoot& e2eRoot = e2eData.getDataRoot();

		// load extra Data from patient file (pdb) and study file (edb)
		if(file.extension() == ".sdb")
		{
			for(const E2E::DataRoot::SubstructurePair& e2ePatPair : e2eRoot)
			{
				char buffer[100];
				const E2E::Patient& e2ePat = *(e2ePatPair.second);
				// 00000001.pdb
				std::sprintf(buffer, "%08d.pdb", e2ePatPair.first);
				bfs::path patientDataFile(file.branch_path() / buffer);
				if(bfs::exists(patientDataFile))
					e2eData.readE2EFile(patientDataFile.generic_string());

				for(const E2E::Patient::SubstructurePair& e2eStudyPair : e2ePat)
				{
					std::sprintf(buffer, "%08d.edb", e2eStudyPair.first);
					bfs::path studyDataFile(file.branch_path() / buffer);
					if(bfs::exists(studyDataFile))
						e2eData.readE2EFile(studyDataFile.generic_string());
				}
			}
		}

		// convert e2e structure in octdata structure
		for(const E2E::DataRoot::SubstructurePair& e2ePatPair : e2eRoot)
		{
			Patient& pat = oct.getPatient(e2ePatPair.first);
			const E2E::Patient& e2ePat = *(e2ePatPair.second);
			copyPatData(pat, e2ePat);

			for(const E2E::Patient::SubstructurePair& e2eStudyPair : e2ePat)
			{
				Study& study = pat.getStudy(e2eStudyPair.first);
				const E2E::Study& e2eStudy = *(e2eStudyPair.second);

				for(const E2E::Study::SubstructurePair& e2eSeriesPair : e2eStudy)
				{
					Series& series = study.getSeries(e2eSeriesPair.first);
					const E2E::Series& e2eSeries = *(e2eSeriesPair.second);

					copySlo(series, e2eSeries);


					for(const E2E::Series::SubstructurePair& e2eBScanPair : e2eSeries)
					{
						copyBScan(series, *(e2eBScanPair.second));
					}
				}
			}
		}

		return true;
	}

	HeE2ERead* HeE2ERead::getInstance()
	{
		static HeE2ERead instance; return &instance;
	}


}