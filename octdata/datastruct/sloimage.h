#pragma once

#include "coordslo.h"

namespace cv { class Mat; }



#ifdef OCTDATA_EXPORT
	#include "octdata_EXPORTS.h"
#else
	#define Octdata_EXPORTS
#endif

namespace OctData
{
	class SloImage
	{
		cv::Mat*    image    = nullptr;
		// std::string filename;

		ScaleFactor scaleFactor;
		CoordSLOpx  shift;
		CoordTransform transform;

		int    numAverage              = 0 ;
		int    imageQuality            = 0 ;

		template<typename T, typename ParameterSet>
		static void callSubset(T& getSet, ParameterSet& p, const std::string& name)
		{
			T subSetGetSet = getSet.subSet(name);
			p.getSetParameter(subSetGetSet);
		}

		template<typename T, typename ParameterSet>
		static void getSetParameter(T& getSet, ParameterSet& p)
		{
			getSet("numAverage"  , p.numAverage  );
			getSet("imageQuality", p.imageQuality);

			callSubset(getSet, p.scaleFactor, "scaleFactor");
			callSubset(getSet, p.shift      , "shift_px"   );
			callSubset(getSet, p.transform  , "transform"  );
		}

	public:
		SloImage();
		~SloImage();

		SloImage(const SloImage& other)            = delete;
		SloImage& operator=(const SloImage& other) = delete;

		const cv::Mat& getImage()                   const           { return *image                 ; }
		void           setImage(const cv::Mat& image);

// 		const std::string& getFilename()             const          { return filename               ; }
// 		void               setFilename(const std::string& s)        {        filename = s           ; }

		const ScaleFactor&    getScaleFactor()         const        { return scaleFactor            ; }
		const CoordSLOpx&     getShift()               const        { return shift                  ; }
		const CoordTransform& getTransform()           const        { return transform              ; }
		void   setScaleFactor(const ScaleFactor& f)                 { scaleFactor = f               ; }
		void   setShift      (const CoordSLOpx&  s)                 { shift       = s               ; }
		void   setTransform  (const CoordTransform& t)              { transform   = t               ; }

		int    getNumAverage()                      const           { return numAverage             ; }
		int    getImageQuality()                    const           { return imageQuality           ; }

		bool  hasImage()                            const           { return image                  ; }
		Octdata_EXPORTS int   getWidth()            const;
		Octdata_EXPORTS int   getHeight()           const;

		template<typename T> void getSetParameter(T& getSet)           { getSetParameter(getSet, *this); }
		template<typename T> void getSetParameter(T& getSet)     const { getSetParameter(getSet, *this); }
	};
}
