// $Id$

#ifndef SCALER3_HH
#define SCALER3_HH

#include "Scaler.hh"
#include "PixelOperations.hh"

namespace openmsx {

/** Base class for 3x scalers
  */
template <class Pixel> class Scaler3 : public Scaler<Pixel>
{
public:
	virtual void scale192(
		FrameSource& src, unsigned srcStartY, unsigned srcEndY,
		OutputSurface& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale192(FrameSource& src0, FrameSource& src1, OutputSurface& dst,
	                      unsigned startY, unsigned endY);

	virtual void scale256(
		FrameSource& src, unsigned srcStartY, unsigned srcEndY,
		OutputSurface& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale256(FrameSource& src0, FrameSource& src1, OutputSurface& dst,
	                      unsigned startY, unsigned endY);

	virtual void scale384(
		FrameSource& src, unsigned srcStartY, unsigned srcEndY,
		OutputSurface& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale384(FrameSource& src0, FrameSource& src1, OutputSurface& dst,
	                      unsigned startY, unsigned endY);

	virtual void scale512(
		FrameSource& src, unsigned srcStartY, unsigned srcEndY,
		OutputSurface& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale512(FrameSource& src0, FrameSource& src1, OutputSurface& dst,
	                      unsigned startY, unsigned endY);

	virtual void scale640(
		FrameSource& src, unsigned srcStartY, unsigned srcEndY,
		OutputSurface& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale640(FrameSource& src0, FrameSource& src1, OutputSurface& dst,
	                      unsigned startY, unsigned endY);

	virtual void scale768(
		FrameSource& src, unsigned srcStartY, unsigned srcEndY,
		OutputSurface& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale768(FrameSource& src0, FrameSource& src1, OutputSurface& dst,
	                      unsigned startY, unsigned endY);

	virtual void scale1024(
		FrameSource& src, unsigned srcStartY, unsigned srcEndY,
		OutputSurface& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scale1024(FrameSource& src0, FrameSource& src1, OutputSurface& dst,
	                       unsigned startY, unsigned endY);

	virtual void scaleImage(
		FrameSource& src, unsigned lineWidth,
		unsigned srcStartY, unsigned srcEndY,
		OutputSurface& dst, unsigned dstStartY, unsigned dstEndY);
	virtual void scaleImage(
		FrameSource& frameEven, FrameSource& frameOdd, unsigned lineWidth,
		OutputSurface& dst, unsigned srcStartY, unsigned srcEndY);

protected:
	Scaler3(SDL_PixelFormat* format);

private:
	PixelOperations<Pixel> pixelOps;
};

} // namespace openmsx

#endif
