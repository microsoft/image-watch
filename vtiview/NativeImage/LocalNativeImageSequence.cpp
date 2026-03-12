#include "stdafx.h"

#include <memory>
#include "NativeImageHelpers.h"
#include "LocalNativeImageSequence.h"

using namespace System;
using namespace System::Text::RegularExpressions;

BEGIN_NI_NAMESPACE;

static const int MAX_DIGITS = 32;

/// Given a path of the form "blah-2134.png", attempt to load all files with 2134 replaced by 0 to INT_MAX
/// Stop on the first failed load after 2134
LocalNativeImageSequence::LocalNativeImageSequence(String^ path)
	: path_(path)
{	
    currentFrame_ = 0;
    width_ = height_ = 0;
	images_.Reset(new vt::vector<vt::CImgInCache>);

    path->Contains("-");

    int dot = path->LastIndexOf(".");
    int dash = path->LastIndexOf("-",dot);
    String^ base = path->Substring(0,dash+1);
    String^ ext = path->Substring(dot);
    String^ num = path->Substring(dash+1,dot-dash-1);
	int start_frame_index = Convert::ToInt32(num);

    numFrames_ = 0;
    int frameNumLoad = 0;
    while (frameNumLoad < INT_MAX)
    {
		int num_digits = dot - dash - 1;
		if (num_digits < 1 || num_digits > MAX_DIGITS)
			throw gcnew ApplicationException("Num digits in filename should be 1..32");
		
		String^ format = "D" + num_digits.ToString();
		String^ file = base + frameNumLoad.ToString(format) + ext;

		msclr::interop::marshal_context mc;	
	    const wchar_t* wcpath = mc.marshal_as<const wchar_t*>(file);
        vt::CImg img;
        if (S_OK != vt::VtLoadImage(wcpath,img))
        {
            if (frameNumLoad > start_frame_index) { break; }
        }
        else
        {
            NativeImageHelpers::ThrowIfHRFailed(images_->resize(numFrames_+1));
            NativeImageHelpers::ThrowIfHRFailed((*images_.Get())[numFrames_].Create(img));
            numFrames_++;
        }
        frameNumLoad++;
    }

    if (numFrames_ > 0)
    {
        vt::CLayerImgInfo iinfo = (*images_.Get())[0].GetImgInfo();
	    width_ = iinfo.width;
	    height_ = iinfo.height;
    }
    ShareCurrentFrame();
}

LocalNativeImageSequence::~LocalNativeImageSequence()
{
	this->!LocalNativeImageSequence();
}

LocalNativeImageSequence::!LocalNativeImageSequence()
{
	images_.Release();	
}

void LocalNativeImageSequence::ShareCurrentFrame()
{

    SetExternallyManagedImage(&(*images_.Get())[currentFrame_]);

	NativeImageHelpers::ThrowIfHRFailed(BuildMipMap(0));
}

bool LocalNativeImageSequence::Reload()
{
    return true;
}

bool LocalNativeImageSequence::TrySetFrameNumber(int number)
{
	if (number == currentFrame_)
		return true;

    if ((number < 0) || (number >= (int)images_->size()))
    {
        return false;
    }
    currentFrame_ = number;
    ShareCurrentFrame();
    return true;
}

bool LocalNativeImageSequence::IsMultiFrame(String^ path)
{
	if (!Regex::IsMatch(path, "-([0-9]+).[^.]+$"))
		return false;
	int dot = path->LastIndexOf(".");
	int dash = path->LastIndexOf("-");
	int numlen = dot - dash - 1;
	return numlen > 0 && numlen <= MAX_DIGITS;
}

END_NI_NAMESPACE;