#include <opencv2/opencv.hpp>
#include <iostream>
#include "../../UAX/GluePathOptimizer.h"
#include "../../UAX/ShoeInsetMask.h"
void GetToolPath_CurvatureOptimized_Mask(
	cv::Mat& ImgSrc,
	const cv::Mat& Mask,              // Input mask
	double offsetPixel,
	ToolPath& toolpath,
	double epsilonFactor,
	int binaryUpper,
	int binaryLower,
	bool enableCurvatureOptimization)
{
	if (ImgSrc.empty()) throw std::invalid_argument("Input image is empty.");

	// 1. Convert to grayscale and apply ROI before erosion.
	cv::Mat gray;
	if (ImgSrc.channels() == 3) cv::cvtColor(ImgSrc, gray, cv::COLOR_BGR2GRAY);
	else gray = ImgSrc.clone();

	cv::Mat maskGray;
	if (!Mask.empty()) {
		if (Mask.channels() == 3)
			cv::cvtColor(Mask, maskGray, cv::COLOR_BGR2GRAY);
		else
			maskGray = Mask.clone();

		cv::threshold(maskGray, maskGray, 1, 255, cv::THRESH_BINARY);

		if (maskGray.size() != gray.size()) {
			cv::resize(maskGray, maskGray, gray.size(), 0, 0, cv::INTER_NEAREST);
		}

		cv::bitwise_and(gray, maskGray, gray);

		std::cout << "[DEBUG] After Mask, non-zero pixels: " << cv::countNonZero(gray) << std::endl;
	}
	else {
		maskGray = cv::Mat(gray.size(), CV_8UC1, cv::Scalar(255));
	}

	// 2. Match the binary preview: black shoe, white background.
	int lowerBound = (std::max)(0, (std::min)(255, binaryLower));
	int upperBound = (std::max)(0, (std::min)(255, binaryUpper));
	if (lowerBound > upperBound) {
		std::swap(lowerBound, upperBound);
	}
	cv::inRange(gray, cv::Scalar(lowerBound), cv::Scalar(upperBound), gray);
	// 3. Select the black shoe inside ROI, remove speckles, then inset its body.
	gray = BuildShoeInsetMask(gray, maskGray, offsetPixel);

	// 4. Extract outer contours.
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(gray, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_L1);

	toolpath.Path.clear();
	toolpath.Offset = cv::Point2d(offsetPixel, 0.0);

	// 5. Either simplify contours or keep original contour points.
	for (const auto& contour : contours)
	{
		if (contour.size() < 3) continue;

		std::vector<cv::Point> finalContour;

		if (enableCurvatureOptimization)
		{
			// Douglas-Peucker simplification.
			double arcLen = cv::arcLength(contour, true);
			double epsilon = epsilonFactor * arcLen;

			cv::approxPolyDP(contour, finalContour, epsilon, true);

			std::cout << "[INFO] Simplified contour from " << contour.size()
				<< " to " << finalContour.size() << " points (epsilon=" << epsilon << ")" << std::endl;
		}
		else
		{
			// Keep original contour points without simplification.
			finalContour = contour;

			std::cout << "[INFO] Using original contour (" << contour.size() << " points) - simplification disabled" << std::endl;
		}

		// Convert contour points to cv::Point2d and append to toolpath.
		for (const auto& point : finalContour)
		{
			toolpath.Path.emplace_back(static_cast<double>(point.x), static_cast<double>(point.y));
		}
	}

	//以下只在 debug 模式下執行
#ifdef _DEBUG
	
	// 7. Draw contours and sampled path points for debug display
	cv::drawContours(ImgSrc, contours, -1, cv::Scalar(0, 0, 255), 1); // contour outline

	cv::Scalar drawColor = enableCurvatureOptimization ? cv::Scalar(0, 255, 0) : cv::Scalar(255, 255, 0); // green=simplified, cyan=original
	for (const auto& p : toolpath.Path) {
		cv::circle(ImgSrc, cv::Point(static_cast<int>(p.x), static_cast<int>(p.y)), 2, drawColor, -1);
	}

	std::cout << "[INFO] GetToolPath_CurvatureOptimized_Mask: Generated " << toolpath.Path.size() << " points "
		<< (enableCurvatureOptimization ? "(simplified)" : "(original)") << std::endl;

	cv::Mat image = ImgSrc.clone();
	//cv::flip(image, image, -1);
	ShowZoomedImage("Masked & " + std::string(enableCurvatureOptimization ? "Reduced" : "Original") + " Points Result", image);
#endif


}
void GetToolPath_Optimized_Mask(
	cv::Mat& ImgSrc,
	const cv::Mat& Mask,
	double offsetPixel,
	double entryPointX,
	ToolPath& toolpath,
	int binaryUpper,
	int binaryLower)
{
	if (ImgSrc.empty()) {
		throw std::invalid_argument("Input image is empty.");
	}

	// Keep preprocessing identical to GetToolPath_CurvatureOptimized_Mask.
	cv::Mat gray;
	if (ImgSrc.channels() == 3) {
		cv::cvtColor(ImgSrc, gray, cv::COLOR_BGR2GRAY);
	}
	else {
		gray = ImgSrc.clone();
	}

	cv::Mat maskGray;
	if (!Mask.empty()) {
		if (Mask.channels() == 3) {
			cv::cvtColor(Mask, maskGray, cv::COLOR_BGR2GRAY);
		}
		else {
			maskGray = Mask.clone();
		}
		cv::threshold(maskGray, maskGray, 1, 255, cv::THRESH_BINARY);
		if (maskGray.size() != gray.size()) {
			cv::resize(maskGray, maskGray, gray.size(), 0, 0, cv::INTER_NEAREST);
		}
		cv::bitwise_and(gray, maskGray, gray);
	}
	else {
		maskGray = cv::Mat(gray.size(), CV_8UC1, cv::Scalar(255));
	}

	// findContours closes an object that touches the ROI along the Mask border.
	// That closing segment is not part of the inward object contour. Erode only
	// the intersection-validity Mask by one pixel so border points cannot become
	// X1/X2, while keeping the preprocessing Mask and offset behavior unchanged.
	cv::Mat safeIntersectionMask;
	constexpr int kIntersectionBoundaryMargin = 5;
	const cv::Mat maskInsetKernel =
		cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	// Explicitly treat pixels outside the image as background. OpenCV's default
	// morphology border value for erosion behaves like foreground, so a Mask
	// touching the image bottom would otherwise remain valid on that boundary.
	cv::erode(
		maskGray,
		safeIntersectionMask,
		maskInsetKernel,
		cv::Point(-1, -1),
		kIntersectionBoundaryMargin,
		cv::BORDER_CONSTANT,
		cv::Scalar(0));

	int lowerBound = (std::max)(0, (std::min)(255, binaryLower));
	int upperBound = (std::max)(0, (std::min)(255, binaryUpper));
	if (lowerBound > upperBound) {
		std::swap(lowerBound, upperBound);
	}
	cv::inRange(gray, cv::Scalar(lowerBound), cv::Scalar(upperBound), gray);
	gray = BuildShoeInsetMask(gray, maskGray, offsetPixel);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(gray, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_L1);

	toolpath.Path.clear();
	toolpath.Offset = cv::Point2d(offsetPixel, 0.0);
	if (contours.empty()) {
		return;
	}

	// The downstream side splitter expects one continuous closed contour.
	const auto largest = std::max_element(contours.begin(), contours.end(),
		[](const std::vector<cv::Point>& lhs, const std::vector<cv::Point>& rhs) {
			return std::abs(cv::contourArea(lhs)) < std::abs(cv::contourArea(rhs));
		});
	if (largest == contours.end() || largest->size() < 3) {
		return;
	}

	constexpr size_t kSampleCountPerSide = 25;
	const std::vector<cv::Point>& contour = *largest;
	auto isInsideSafeMask = [&safeIntersectionMask, kIntersectionBoundaryMargin](double x, double y) {
		const int ix = cvRound(x);
		const int iy = cvRound(y);
		// Never accept findContours' artificial closing segment on the image edge.
		return ix >= kIntersectionBoundaryMargin &&
			ix < safeIntersectionMask.cols - kIntersectionBoundaryMargin &&
			iy >= kIntersectionBoundaryMargin &&
			iy < safeIntersectionMask.rows - kIntersectionBoundaryMargin &&
			safeIntersectionMask.at<uchar>(iy, ix) != 0;
	};

	auto findSideIntersections = [&contour, &isInsideSafeMask](
		double targetY, double& leftX, double& rightX) {
		std::vector<double> intersections;
		for (size_t i = 0; i < contour.size(); ++i) {
			const cv::Point2d p0(contour[i]);
			const cv::Point2d p1(contour[(i + 1) % contour.size()]);
			const double edgeMinY = (std::min)(p0.y, p1.y);
			const double edgeMaxY = (std::max)(p0.y, p1.y);
			if (targetY < edgeMinY || targetY > edgeMaxY) {
				continue;
			}

			const double dy = p1.y - p0.y;
			if (std::abs(dy) <= 1e-9) {
				if (std::abs(targetY - p0.y) <= 1e-6) {
					if (isInsideSafeMask(p0.x, targetY)) {
						intersections.push_back(p0.x);
					}
					if (isInsideSafeMask(p1.x, targetY)) {
						intersections.push_back(p1.x);
					}
				}
				continue;
			}

			const double t = (targetY - p0.y) / dy;
			const double intersectionX = p0.x + t * (p1.x - p0.x);
			if (isInsideSafeMask(intersectionX, targetY)) {
				intersections.push_back(intersectionX);
			}
		}

		if (intersections.empty()) {
			return false;
		}
		const auto xRange = std::minmax_element(intersections.begin(), intersections.end());
		leftX = *xRange.first;
		rightX = *xRange.second;
		return rightX > leftX + 1e-6;
	};

	// The final pair is defined by the intersections of the inward contour with
	// the ROI Bottom Line. A contour clipped by the ROI forms a horizontal bottom
	// segment; its two endpoints are precisely the left/right side intersections.
	auto findRoiBottomIntersections = [&contour](
		double targetY, double& leftX, double& rightX) {
		std::vector<double> intersections;
		for (size_t i = 0; i < contour.size(); ++i) {
			const cv::Point2d p0(contour[i]);
			const cv::Point2d p1(contour[(i + 1) % contour.size()]);
			const double dy = p1.y - p0.y;
			if (std::abs(dy) <= 1e-9) {
				if (std::abs(targetY - p0.y) <= 1e-6) {
					// Keep only the endpoints as candidates. min/max below select
					// the two side intersections, never a point inside the segment.
					intersections.push_back(p0.x);
					intersections.push_back(p1.x);
				}
				continue;
			}
			const double edgeMinY = (std::min)(p0.y, p1.y);
			const double edgeMaxY = (std::max)(p0.y, p1.y);
			if (targetY < edgeMinY || targetY > edgeMaxY) {
				continue;
			}
			const double t = (targetY - p0.y) / dy;
			if (t >= -1e-9 && t <= 1.0 + 1e-9) {
				intersections.push_back(p0.x + t * (p1.x - p0.x));
			}
		}
		if (intersections.size() < 2) {
			return false;
		}
		const auto xRange = std::minmax_element(intersections.begin(), intersections.end());
		leftX = *xRange.first;
		rightX = *xRange.second;
		return rightX > leftX + 1e-6;
	};

	struct SideRow {
		double y;
		double leftX;
		double rightX;
	};
	const cv::Rect bounds = cv::boundingRect(contour);
	std::vector<SideRow> rows;
	rows.reserve(static_cast<size_t>(bounds.height));
	for (int y = bounds.y; y < bounds.y + bounds.height; ++y) {
		double leftX = 0.0;
		double rightX = 0.0;
		if (findSideIntersections(static_cast<double>(y), leftX, rightX)) {
			rows.push_back({ static_cast<double>(y), leftX, rightX });
		}
	}
	if (rows.size() < kSampleCountPerSide) {
		return;
	}

	// Find the Y of EntryPointX on the right inward contour. Use the first
	// crossing from top to bottom; if no exact crossing exists, use the closest
	// right-contour row without moving the point away from the contour.
	double entryY = rows.front().y;
	size_t entryRowIndex = 0;
	bool foundEntryCrossing = false;
	double nearestEntryDx = std::abs(rows.front().rightX - entryPointX);
	for (size_t i = 1; i < rows.size(); ++i) {
		const double currentDx = std::abs(rows[i].rightX - entryPointX);
		if (currentDx < nearestEntryDx) {
			nearestEntryDx = currentDx;
			entryY = rows[i].y;
			entryRowIndex = i;
		}

		if (rows[i].y - rows[i - 1].y > 1.0) {
			continue;
		}
		const double x0 = rows[i - 1].rightX - entryPointX;
		const double x1 = rows[i].rightX - entryPointX;
		if (x0 == 0.0 || x1 == 0.0 ||
			(x0 < 0.0 && x1 > 0.0) || (x0 > 0.0 && x1 < 0.0)) {
			const double dx = rows[i].rightX - rows[i - 1].rightX;
			const double t = std::abs(dx) > 1e-9
				? (entryPointX - rows[i - 1].rightX) / dx
				: 0.0;
			entryY = rows[i - 1].y + t * (rows[i].y - rows[i - 1].y);
			entryRowIndex = i - 1;
			foundEntryCrossing = true;
			break;
		}
	}

	// Y25 is the ROI Bottom Line. Its X1/X2 values are obtained only from the
	// inward contour. For a horizontal clipped segment, its min/max endpoints
	// are the left/right intersections with the ROI Bottom Line.
	std::vector<cv::Point> maskPoints;
	cv::findNonZero(maskGray, maskPoints);
	if (maskPoints.empty()) {
		return;
	}
	const cv::Rect maskBounds = cv::boundingRect(maskPoints);
	double bottomY = static_cast<double>(maskBounds.y + maskBounds.height - 1);
	if (bottomY <= entryY) {
		return;
	}
	double bottomLeftX = 0.0;
	double bottomRightX = 0.0;
	if (!findRoiBottomIntersections(bottomY, bottomLeftX, bottomRightX)) {
		// The detected object does not always extend to the configured ROI bottom.
		// In that case, use the last scan line that has two valid contour sides so
		// ToolPathType 1 can still produce its synchronized 25-point path.
		bottomY = rows.back().y;
		bottomLeftX = rows.back().leftX;
		bottomRightX = rows.back().rightX;
		if (bottomY <= entryY) {
			return;
		}
	}

	std::vector<cv::Point2d> leftPoints;
	std::vector<cv::Point2d> rightPoints;
	leftPoints.reserve(kSampleCountPerSide);
	rightPoints.reserve(kSampleCountPerSide);
	for (size_t sampleIndex = 0; sampleIndex < kSampleCountPerSide; ++sampleIndex) {
		const double targetY = entryY + (bottomY - entryY) *
			static_cast<double>(sampleIndex) /
			static_cast<double>(kSampleCountPerSide - 1);
		double leftX = 0.0;
		double rightX = 0.0;
		const bool isLastPoint = (sampleIndex + 1 == kSampleCountPerSide);
		bool foundIntersections = false;
		if (isLastPoint) {
			leftX = bottomLeftX;
			rightX = bottomRightX;
			foundIntersections = rightX > leftX + 1e-6;
		}
		else {
			foundIntersections = findSideIntersections(targetY, leftX, rightX);
		}
		if (!foundIntersections) {
			toolpath.Path.clear();
			return;
		}
		if (sampleIndex == 0 && foundEntryCrossing) {
			rightX = entryPointX;
		}
		leftPoints.emplace_back(leftX, targetY);
		rightPoints.emplace_back(rightX, targetY);
	}

	// ToolPathType 1 keeps its 25-point sampling, but its final pair must use
	// exactly the ToolPathType 0 bottom-point rule with the ORIGINAL inward
	// contour as source (not the already sampled 25 points).
	std::vector<cv::Point2d> rawContour;
	rawContour.reserve(contour.size());
	for (const cv::Point& point : contour) {
		rawContour.emplace_back(
			static_cast<double>(point.x),
			static_cast<double>(point.y));
	}
	GluePath sampledPath;
	sampledPath.PathRight = rightPoints;
	sampledPath.PathLeft = leftPoints;
	GluePathOptimizer::ApplyLegacyBottomPointsFromContour(rawContour, sampledPath);
	rightPoints = std::move(sampledPath.PathRight);
	leftPoints = std::move(sampledPath.PathLeft);

	// Preserve a closed-contour traversal for GluePathOptimizer::SplitByCenter.
	toolpath.Path.reserve(kSampleCountPerSide * 2);
	toolpath.Path.insert(toolpath.Path.end(), rightPoints.begin(), rightPoints.end());
	toolpath.Path.insert(toolpath.Path.end(), leftPoints.rbegin(), leftPoints.rend());

#ifdef _DEBUG
	cv::drawContours(ImgSrc, contours, -1, cv::Scalar(0, 0, 255), 1);
	for (const cv::Point2d& point : toolpath.Path) {
		cv::circle(ImgSrc, point, 3, cv::Scalar(0, 255, 0), cv::FILLED);
	}
	std::cout << "[INFO] GetToolPath_Optimized_Mask: generated "
		<< rightPoints.size() << " synchronized Y points per side" << std::endl;
#endif
}
int main(){
 auto image=cv::imread("DOC/Images/1005Debug.png",cv::IMREAD_GRAYSCALE);
 cv::Mat roi=cv::Mat::zeros(image.size(),CV_8UC1); roi(cv::Rect(400,110,650,950)).setTo(255);
 cv::Mat binary; cv::inRange(image,0,127,binary);
 cv::Mat canvas; cv::cvtColor(image,canvas,cv::COLOR_GRAY2BGR);
 for(int offset: {0,51}) {
  auto mask=BuildShoeInsetMask(binary,roi,offset);
  std::vector<std::vector<cv::Point>> cs; cv::findContours(mask,cs,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
  cv::drawContours(canvas,cs,-1,offset?cv::Scalar(0,255,0):cv::Scalar(0,0,255),2);
  ToolPath path; auto input=image.clone();
  GetToolPath_Optimized_Mask(input,roi,offset,695+5/.234856,path,127,0);
  int outside=0;
  for(auto pt:path.Path){if(mask.at<uchar>(cvRound(pt.y),cvRound(pt.x))==0) ++outside; cv::circle(canvas,pt,4,offset?cv::Scalar(0,255,255):cv::Scalar(255,0,0),-1);}
  std::cout<<"offset="<<offset<<" mask_area="<<cv::countNonZero(mask)<<" components="<<cs.size()<<" sampled="<<path.Path.size()<<" outside="<<outside<<"\n";
  cv::imwrite("tmp/inset-tests/mask"+std::to_string(offset)+".png",mask);
 }
 cv::rectangle(canvas,cv::Rect(400,110,650,950),cv::Scalar(255,255,0),2);
 cv::imwrite("tmp/inset-tests/1005_result.png",canvas);
}

