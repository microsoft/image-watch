#include <cliext/vector>
#include <vector>
#include "vtcore.h"

#include "WatchedImage.h"
#include "WatchedImageHelpers.h"

// EnvDTE uses default parameters which are unsupported in C++/CLI. We don't 
// use them, so I guess it's fine to disable the warning.
#pragma warning(disable : 4564)

using namespace System::Collections::Generic;
using namespace Microsoft::Research::NativeImage;
using namespace Microsoft::VisualStudio;
using namespace System::Diagnostics;
using namespace Microsoft::VisualStudio::Debugger;
using namespace Microsoft::VisualStudio::Debugger::CallStack;
using namespace Microsoft::VisualStudio::Debugger::Evaluation;

bool Microsoft::ImageWatch::WatchedImageHelpers::Is64BitProcess(
	DkmInspectionContext^, DkmStackWalkFrame^ frame, bool% is64bit)
{
	is64bit = frame->Process->SystemInformation->Flags.HasFlag(
		DefaultPort::DkmSystemInformationFlags::Is64Bit);
	return true;
}

bool Microsoft::ImageWatch::WatchedImageHelpers::FollowPointer(
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame,
	UInt64 base, UInt64 ptrOffset, UInt64% target)
{
	String^ value;
	String^ type;
	
	if (!WatchedImageHelpers::EvaluateExpression(context, frame,
		String::Format("*(size_t*)({0} + {1})", base, ptrOffset),
		value, type))
		return false;

	return TryParseUInt64(value, target);
}

bool Microsoft::ImageWatch::WatchedImageHelpers::FollowTwoPointers(
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame,
	UInt64 base, UInt64 ptrOffset1, UInt64 ptrOffset2, UInt64% target)
{
	String^ value;
	String^ type;
	if (!WatchedImageHelpers::EvaluateExpression(context, frame,
		String::Format("*(size_t*)((*(size_t*)({0} + {1})) + {2})",
		base, ptrOffset1, ptrOffset2), value, type))
		return false;

	return TryParseUInt64(value, target);
}

ref class AppendGetExpressionBytesRoutines
{
public:
	AppendGetExpressionBytesRoutines(
		DkmInspectionContext^ inspectionContext,
		DkmWorkList^ workList,
		DkmLanguageExpression^ expression,
		bool enumerateChildren,
		Action<bool, String^, String^, String^>^ resultRoutine)
		: inspectionContext_(inspectionContext),
		  workList_(workList),
		  expression_(expression),
		  enumerateChildren_(enumerateChildren),
		  resultRoutine_(resultRoutine)
	{
	}

	~AppendGetExpressionBytesRoutines()
	{
		if (expression_)
			expression_->Close();
		if (evaluationResult_)
			evaluationResult_->Close();
	}

	void EvaluateExpression(DkmEvaluateExpressionAsyncResult asyncResult)
	{
		try
		{
			evaluationResult_ = asyncResult.ResultObject;
			auto successEvaluationResult = dynamic_cast<DkmSuccessEvaluationResult^>(
				asyncResult.ResultObject);
			if (successEvaluationResult == nullptr)
			{
				throw gcnew Exception("Evaluation failed");
			}

			if (successEvaluationResult->Flags.HasFlag(DkmEvaluationResultFlags::RawString))
			{
				auto underlyingString = successEvaluationResult->GetUnderlyingString();
				ReportResult(true, successEvaluationResult->Type, successEvaluationResult->Name, underlyingString);
			}
			else if (!enumerateChildren_)
			{
				ReportResult(true, successEvaluationResult->Type,
					successEvaluationResult->Name, successEvaluationResult->EditableValue);
			}
			else
			{
				evaluationResult_->GetChildren(workList_, 1024, inspectionContext_,
					gcnew DkmCompletionRoutine<DkmGetChildrenAsyncResult>(
						this, &AppendGetExpressionBytesRoutines::GetChildren));
			}
		}
		catch (...)
		{
			ReportResult(false, nullptr, nullptr, nullptr);
		}
	}

	void GetChildren(DkmGetChildrenAsyncResult asyncResult)
	{
		try
		{
			if (asyncResult.ErrorCode != 0)
			{
				throw gcnew Exception("GetChildren failed");
			}

			for each (auto child in asyncResult.InitialChildren)
			{
				try
				{
					auto successEvaluationResult = dynamic_cast<DkmSuccessEvaluationResult^>(
						child);
					if (successEvaluationResult == nullptr)
						continue;

					ReportResult(true, successEvaluationResult->Type,
						successEvaluationResult->Name, successEvaluationResult->EditableValue);
				}
				finally
				{
					child->Close();
				}
			}
		}
		catch (...)
		{
			ReportResult(false, nullptr, nullptr, nullptr);
		}
	}

private:
	void ReportResult(bool success, String^ type, String^ name, String^ value)
	{
		if (type == nullptr)
			type = String::Empty;
		if (name == nullptr)
			name = String::Empty;
		if (value == nullptr)
			value = String::Empty;
		resultRoutine_(success, type, name, value);
	}

	DkmInspectionContext^ inspectionContext_;
	DkmWorkList^ workList_;

	DkmLanguageExpression^ expression_;
	DkmEvaluationResult^ evaluationResult_ = nullptr;

	bool enumerateChildren_;
	Action<bool, String^, String^, String^>^ resultRoutine_;
};

static void AppendGetExpressionBytesSync(
	DkmInspectionContext^ inspectionContext, String^ expressionText, DkmStackWalkFrame^ frame,
	int evalFlags, bool enumerateChildren,
	Action<bool, String^, String^, String^>^ resultRoutine)
{
	auto workList = DkmWorkList::Create(nullptr);
	try
	{
		DkmLanguageExpression^ expression = DkmLanguageExpression::Create(
			inspectionContext->Language,
			(DkmEvaluationFlags)((int)DkmEvaluationFlags::TreatAsExpression | evalFlags),
			expressionText, nullptr);
		AppendGetExpressionBytesRoutines routines(inspectionContext, workList,
			expression, enumerateChildren, resultRoutine);
		inspectionContext->EvaluateExpression(workList, expression, frame,
			gcnew DkmCompletionRoutine<DkmEvaluateExpressionAsyncResult>(
				%routines, &AppendGetExpressionBytesRoutines::EvaluateExpression));
		workList->Execute();
	}
	catch (...)
	{
		workList->Cancel();
	}
}

ref class EvaluateExpressionReceiver
{
public:
	void Receive(bool success, String^ type, String^ name, String^ value)
	{
		success_ = success;
		type_ = type;
		(void)name; // Ignore.
		value_ = value;
	};

	property bool Succeeded { bool get() { return success_; } }
	property String^ Type { String^ get() { return type_; } }
	property String^ Value { String^ get() { return value_; } }

private:
	bool success_;
	String^ type_;
	String^ value_;
};

bool Microsoft::ImageWatch::WatchedImageHelpers::EvaluateExpression(
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame,
	String^ expression, String^% value, String^% type)
{
	return EvaluateExpression(context, frame, expression, value, type, nullptr, true);
}

bool Microsoft::ImageWatch::WatchedImageHelpers::EvaluateExpression(
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame,
	String^ expression, String^% value, String^% type, String^ format)
{
	return EvaluateExpression(context, frame, expression, value, type, format, true);
}

bool Microsoft::ImageWatch::WatchedImageHelpers::EvaluateExpression(
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame,
	String^ expression, String^% value, String^% type, String^ format,
	bool getRawValue)
{
	value = nullptr;
	type = nullptr;
	
	auto formattedExpression = format != nullptr ? 
		String::Format("{0},{1}", expression, format) :
		expression;

#ifdef _DEBUG
	Stopwatch^ sw = gcnew Stopwatch();
	sw->Start();
#endif

	try
	{
		EvaluateExpressionReceiver evaluationResult;
		AppendGetExpressionBytesSync(context, formattedExpression, frame,
			getRawValue ? (int)DkmEvaluationFlags::ShowValueRaw : 0, false,
			gcnew Action<bool, String^, String^, String^>(
				%evaluationResult, &EvaluateExpressionReceiver::Receive));

		if (!evaluationResult.Succeeded)
		{
			return false;
		}

		value = evaluationResult.Value;
		type = evaluationResult.Type;
	}
	catch (...)
	{
		return false;
	}

#ifdef _DEBUG
	Debug::WriteLine("evaluating '{0}' took {1}ms", formattedExpression,
		sw->ElapsedMilliseconds);
#endif

	return true;
}

ref class GetEvaluatedExpressionChildrenInfoReceiver
{
public:
	void Receive(bool success, String^ type, String^ name, String^ value)
	{
		success_ = success_ && success;
		(void)type; // Ignore.
		children_.push_back(gcnew Tuple<String^, String^>(name, value));
	};

	property bool Succeeded { bool get() { return success_; } }
	property int Count { int get() { return children_.size(); } }
	String^ GetName(int i) { return children_[i]->Item1; }
	String^ GetValue(int i) { return children_[i]->Item2; }

private:
	bool success_ = true;
	cliext::vector<Tuple<String^, String^>^> children_;
};

cli::array<Tuple<String^, String^>^>^
	Microsoft::ImageWatch::WatchedImageHelpers::GetEvaluatedExpressionChildrenInfo(
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame, String^ expression)
{
	try
	{
		GetEvaluatedExpressionChildrenInfoReceiver evaluationChildrenResult;
		AppendGetExpressionBytesSync(context, expression, frame, 0, true,
			gcnew Action<bool, String^, String^, String^>(%evaluationChildrenResult,
				&GetEvaluatedExpressionChildrenInfoReceiver::Receive));

		if (!evaluationChildrenResult.Succeeded)
			return gcnew cli::array<Tuple<String^, String^>^>(0);

		auto res = gcnew cli::array<Tuple<String^, String^>^>(evaluationChildrenResult.Count);
		for (int i = 0; i < res->Length; ++i)
		{
			res[i] = gcnew Tuple<String^, String^>(
				evaluationChildrenResult.GetName(i),
				evaluationChildrenResult.GetValue(i));
		}
		return res;
	}
	catch (...)
	{
		return gcnew cli::array<Tuple<String^, String^>^>(0);
	}
}

bool Microsoft::ImageWatch::WatchedImageHelpers::
	GetPropertyChildValue(cli::array<Tuple<String^, String^>^>^ plist,
	String^ name, String^% value, String^% error)
{
	value = nullptr;
	error = nullptr;
	auto vlist = gcnew List<String^>();

	if (name == nullptr)
	{
		System::Diagnostics::Debug::Assert(false);
		return false;
	}

	if (plist == nullptr)
		return false;

	for (int i=0; i<plist->Length; ++i)
	{
		if (plist[i] != nullptr && plist[i]->Item1 == name)
			vlist->Add(plist[i]->Item2);
	}

	if (vlist->Count == 0)
	{
		error = String::Format("{0} not found", name);
		return false;
	}
	else if (vlist->Count > 1)
	{
		error = String::Format("{0} has multiple ({1}) definitions", name,
			vlist->Count);
		return false;
	}

	value = vlist[0];
	return true;
}

bool Microsoft::ImageWatch::WatchedImageHelpers::EvaluateUInt32(
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame,
	UInt64 address, UInt32% value)
{
	if (context == nullptr)
		return false;

	String^ valueStr = nullptr;
	String^ unused = nullptr;
	if (!EvaluateExpression(context, frame,
		String::Format("*(unsigned int*)({0})", address), valueStr,
		unused, "u"))
		return false;

	return UInt32::TryParse(valueStr, value);
}

bool Microsoft::ImageWatch::WatchedImageHelpers::EvaluateMemberAddress(
	String^ objectPointerType, UInt64 objectAddress, String^ name,
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame, UInt64% address)
{
	if (context == nullptr)
		return false;

	if (objectAddress == 0)
		return false;

	auto imgPtrString = String::Format("(({0})(0x{1:x}))", 
		objectPointerType, objectAddress);

	String^ valueStr = nullptr;
	String^ unused = nullptr;
	if (!EvaluateExpression(context, frame,
		String::Format("(unsigned __int64)(&({0}->{1}))",
		imgPtrString, name), valueStr, unused, "u"))
		return false;

	return UInt64::TryParse(valueStr, address);
}

template <typename T>
bool EvaluateMemberWithFormat(
	String^ objectPointerType, UInt64 objectAddress, String^ name,
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame,
	T% value, String^ castType, String^ format)
{
	if (context == nullptr)
		return false;

	if (objectAddress == 0)
		return false;

	auto imgPtrString = String::Format("(({0})(0x{1:x}))", 
		objectPointerType, objectAddress);

	String^ valueStr = nullptr;
	String^ unused = nullptr;
	if (!Microsoft::ImageWatch::WatchedImageHelpers::EvaluateExpression(
		context, frame, String::Format("({0})({1}->{2})", castType,
		imgPtrString, name), valueStr, unused, format))
		return false;

	return T::TryParse(valueStr, value);
}

bool Microsoft::ImageWatch::WatchedImageHelpers::EvaluateMemberUInt64(
	String^ objectPointerType, UInt64 objectAddress, String^ name,
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame, UInt64% value)
{
	return EvaluateMemberWithFormat(objectPointerType, objectAddress,
		name, context, frame, value, "unsigned __int64", "u");
}

bool Microsoft::ImageWatch::WatchedImageHelpers::EvaluateMemberUInt32(
	String^ objectPointerType, UInt64 objectAddress, String^ name, 
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame, UInt32% value)
{
	return EvaluateMemberWithFormat(objectPointerType, objectAddress, 
		name, context, frame, value, "unsigned int", "u");	
}

bool Microsoft::ImageWatch::WatchedImageHelpers::EvaluateMemberInt64(
	String^ objectPointerType, UInt64 objectAddress, String^ name, 
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame, Int64% value)
{
	return EvaluateMemberWithFormat(objectPointerType, objectAddress, 
		name, context, frame, value, "__int64", "d");
}

bool Microsoft::ImageWatch::WatchedImageHelpers::EvaluateMemberInt32(
	String^ objectPointerType, UInt64 objectAddress, String^ name, 
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame, Int32% value)
{
	return EvaluateMemberWithFormat(objectPointerType, objectAddress, 
		name, context, frame, value, "int", "d");	
}

bool Microsoft::ImageWatch::WatchedImageHelpers::LoadRemoteVTCImgInfo(
	int% width, int% height, int% type, UInt64% pixelAddress,
	int% numStrideBytes, UInt64 objectAddress,
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame)
{
	try
	{	
#define EVAL_MEMBER(name_, var_, type_) \
		if (!EvaluateMember##type_("vt::CImg*", objectAddress, name_, \
			context, frame, var_)) \
			return false;
	
		EVAL_MEMBER("m_info.type", type, Int32);
		EVAL_MEMBER("m_pbPtr", pixelAddress, UInt64);
		EVAL_MEMBER("m_info.width", width, Int32);
		EVAL_MEMBER("m_info.height", height, Int32);
		EVAL_MEMBER("m_iStrideBytes", numStrideBytes, Int32);

#undef EVAL_MEMBER
	}
	catch (...)
	{
		return false;
	}

	return true;
}

bool Microsoft::ImageWatch::WatchedImageHelpers::ValidateVTCImgInfo(
	int width, int height, int type, int numStrideBytes, 
	UInt64 pixelAddress)
{
	// "properly" undefined is a valid state, too  
	if (type == OBJ_UNDEFINED)
		return true;

	// image was initialized, check integrity ...
#define ENFORCE(expr) if (!(expr)) return false; 

	ENFORCE(width >= 0 && width <= MAX_IMAGE_WIDTH);
	ENFORCE(height >= 0 && height <= MAX_IMAGE_HEIGHT);
	// limit stride size since this becomes a statically allocated buffer in LoadRemoteVTCImg() ...
	// ENFORCE(numStrideBytes <= (1 << 20) * 16); 

	const auto bands = VT_IMG_BANDS(type);	
	ENFORCE(bands >= 0 && bands <= VT_IMG_BANDS_MAX);

	ENFORCE(numStrideBytes >= VT_IMG_ELSIZE(type) * bands * width);

	if (width * height * bands > 0)
		ENFORCE(pixelAddress != 0);

#undef ENFORCE

	return true;
}

bool Microsoft::ImageWatch::WatchedImageHelpers::ReadMemory(
	Microsoft::VisualStudio::Debugger::DkmProcess^ process,
	UInt64 readPtr, void* buffer, UInt32 numBytesToRead)
{
	if (process == nullptr)
		return false;

	if (buffer == nullptr)
		return false;

	try
	{
		return process->ReadMemory(readPtr, 
			Microsoft::VisualStudio::Debugger::DkmReadMemoryFlags::None,
			buffer, (int)numBytesToRead) == (int)numBytesToRead;
	}
	catch (...)
	{
		return false;
	}
}

bool Microsoft::ImageWatch::WatchedImageHelpers::LoadRemoteVTCImg(
	vt::IImageReaderWriter* img, UInt64 pixelAddress, int numStrideBytes,
	Microsoft::VisualStudio::Debugger::DkmProcess^ process)
{
	if (img == nullptr)
		return false;

	if (pixelAddress == 0)
		return false;

	if (numStrideBytes <= 0)
		return false;

	if (process == nullptr)
		return false;

	AutoLock lock(Microsoft::ImageWatch::WatchedImageHelpers::BufferLock);
	static std::vector<unsigned char> buffer;
	try
	{
		buffer.resize(vt::VtMax(1 << 24, numStrideBytes));
	}
	catch (std::bad_alloc&)
	{
		return false;
	}

	auto info = img->GetImgInfo();

	const int maxRowsPerRead = 
		vt::VtMax((int)buffer.size() / numStrideBytes, 1);

	UInt64 readPtr = pixelAddress;
	for (int numRowsLeft = info.height; numRowsLeft > 0;)
	{
		const int numRowsToRead = Math::Min(numRowsLeft, maxRowsPerRead);
		const int numBytesToRead = numRowsToRead * numStrideBytes;
		
		if (!ReadMemory(process, readPtr, &buffer[0], numBytesToRead))
			return false;

		vt::CImg bufImg;
		if (FAILED(bufImg.Create(&buffer[0], info.width, numRowsToRead,
			numStrideBytes, info.type)))
			return false;

		if (FAILED(img->WriteRegion(
			vt::CRect(0, info.height - numRowsLeft, info.width, 
			info.height - numRowsLeft + numRowsToRead), bufImg)))
			return false;

		readPtr += numBytesToRead;
		numRowsLeft -= numRowsToRead;
	}

	return true;
}

bool Microsoft::ImageWatch::WatchedImageHelpers::LoadRemoteVTCImgInCacheInfo(
	int% width, int% height, int% type, UInt32% blockSizeX, UInt32% blockSizeY,
    UInt64% blocksAddress, UInt64% blocksSize, UInt64 objectAddress,
	DkmInspectionContext^ context, DkmStackWalkFrame^ frame)
{
	try
	{		
#define EVAL_MEMBER(name_, var_, type_) \
		if (!EvaluateMember##type_("vt::CImgInCache*", objectAddress, name_, \
			context, frame, var_)) \
			return false;
	
        UInt32 source;
        EVAL_MEMBER("m_uSourceId", source, UInt32);

        auto image =
            String::Format("m_pIC->m_vecSources._First[{0}]->vecImages._First[0].",
            source);
		EVAL_MEMBER(image + "uBlockSizeX", blockSizeX, UInt32);
		EVAL_MEMBER(image + "uBlockSizeY", blockSizeY, UInt32);

        auto level = image + "vecLevels._First[0].";
		EVAL_MEMBER(level + "info.type", type, Int32);
		EVAL_MEMBER(level + "info.width", width, Int32);
		EVAL_MEMBER(level + "info.height", height, Int32);

        auto blocks = level + "vecBlocks.";
        EVAL_MEMBER(blocks + "_First", blocksAddress, UInt64);
        EVAL_MEMBER(blocks + "_Last", blocksSize, UInt64);
        blocksSize -= blocksAddress;

#undef EVAL_MEMBER
	}
	catch (...)
	{
		return false;
	}

	return true;
}

bool Microsoft::ImageWatch::WatchedImageHelpers::ValidateVTCImgInCacheInfo(
	int type, UInt32 blockSizeX, UInt32 blockSizeY,
    UInt64 blocksAddress, UInt64 blocksSize)
{
	// "properly" undefined is a valid state, too  
	if (type == OBJ_UNDEFINED)
		return true;

	// image was initialized, check integrity ...
#define ENFORCE(expr) if (!(expr)) return false; 

	const auto bands = VT_IMG_BANDS(type);	
	ENFORCE(bands >= 0 && bands <= VT_IMG_BANDS_MAX);

    ENFORCE(blockSizeX > 0 && blockSizeX <= (1 << 16));
	ENFORCE(blockSizeY > 0 && blockSizeY <= (1 << 16));

	ENFORCE(blocksAddress != NULL && blocksSize > 0);

#undef ENFORCE

	return true;
}

bool Microsoft::ImageWatch::WatchedImageHelpers::LoadRemoteVTCImgInCache(
	vt::IImageReaderWriter* img, UInt32 blockSizeX, UInt32 blockSizeY,
    UInt64 blocksAddress, UInt64 blocksSize, UInt32 * uniqueness,
	Microsoft::VisualStudio::Debugger::DkmProcess^ process)
{
	if (img == nullptr)
		return false;

	if (blockSizeX == 0 || blockSizeY == 0 ||
        blocksAddress == 0 || blocksSize == 0 ||
        uniqueness == NULL)
		return false;

	if (process == nullptr)
		return false;

	auto info = img->GetImgInfo();

	AutoLock lock(Microsoft::ImageWatch::WatchedImageHelpers::BufferLock);
	static std::vector<unsigned char> buffer;
	try
	{
		// Round up for align64ByteRows.
		int numStrideBytes = (blockSizeX * info.PixSize() + 0x3f) & ~0x3f;
		buffer.resize(blockSizeY * numStrideBytes);
	}
	catch (std::bad_alloc&)
	{
		return false;
	}

    // Check for 32-or-64-bit client
    UINT blocksX = (info.width  + blockSizeX - 1) / blockSizeX;
    UINT blocksY = (info.height + blockSizeY - 1) / blockSizeY;
    if (blocksX * blocksY * 12 != blocksSize &&
        blocksX * blocksY * 16 != blocksSize)
        return false;
    UINT ptrSize = (UINT) (blocksSize / (blocksX * blocksY)) - 8;

    vt::vector<UINT> blocks;
    if (FAILED(blocks.resize((UINT) (blocksSize / sizeof(UINT)))))
        return false;
    UINT * pBlock = blocks.begin();

	if (!ReadMemory(process, blocksAddress, blocks.begin(), (UINT) blocksSize))
		return false;

    vt::vector<Byte> fill;
    if (FAILED(fill.resize(info.PixSize())))
        return false;

    int i = 0;
    for (vt::CBlockIterator bi(vt::BLOCKITER_INIT(info.Rect(), blockSizeX, blockSizeY));
        !bi.Done(); bi.Advance(), i++)
    {
        UINT tract = *pBlock++;
        UINT unique = *pBlock++;
        UInt64 link = ptrSize == 4 ? pBlock[0] : *((UInt64 *) &pBlock[0]);
        pBlock += ptrSize / sizeof(UINT);
        if (unique == uniqueness[i] && (tract & BLOCK_IN_WRITE) == 0)
            continue;
        uniqueness[i] = unique;

        vt::CRect rct = bi.GetCompRect();

        if (tract & BLOCK_IN_CACHE)
        {
            UInt64 data = 0;
            UInt64 linkAddress = link + 2 * ptrSize;
		    if (!ReadMemory(process, linkAddress, &data, ptrSize))
			    return false;

            // Round up for align64ByteRows.
            int numStrideBytes = (rct.Width() * info.PixSize() + 0x3f) & ~0x3f;

		    if (!ReadMemory(process, data, &buffer[0],
                rct.Height() * numStrideBytes))
			    return false;

		    vt::CImg bufImg;
		    if (FAILED(bufImg.Create(&buffer[0], rct.Width(), rct.Height(),
			    numStrideBytes, info.type)))
			    return false;

		    if (FAILED(img->WriteRegion(rct, bufImg)))
			    return false;
        }
        else if (!(tract & BLOCK_IN_STORE) && link != NULL)
        {
		    if (!ReadMemory(process, link, fill.begin(), info.PixSize()))
			    return false;

            if (FAILED(img->Fill(fill.begin(), &rct)))
			    return false;
        }
    }

	return true;
}

int Microsoft::ImageWatch::WatchedImageHelpers::StartVTTaskManager()
{
	if (VTTaskManagerStarted)
		return S_OK;

	HRESULT hr = vt::TaskManagerStartup();

	VTTaskManagerStarted = SUCCEEDED(hr);

	return (int)hr;
}

void Microsoft::ImageWatch::WatchedImageHelpers::StopVTTaskManager()
{
	if (VTTaskManagerStarted)
		vt::TaskManagerShutdown();
}

String^ Microsoft::ImageWatch::WatchedImageHelpers
	::MakeNeutralElFormatString(int vttype)
{
	switch (EL_FORMAT(vttype))
	{	
	case EL_FORMAT_BYTE:
		return L"UINT8";
	case EL_FORMAT_SBYTE:
		return L"INT8";
	case EL_FORMAT_SHORT:
		return L"UINT16";
	case EL_FORMAT_SSHORT:
		return L"INT16";
	case EL_FORMAT_INT:
		return L"INT32";
	case EL_FORMAT_HALF_FLOAT:
		return L"FLOAT16";
	case EL_FORMAT_FLOAT:
		return L"FLOAT32";
	case EL_FORMAT_DOUBLE:
		return L"FLOAT64";
	default:
		return L"UNKNOWN";
	}
}

bool Microsoft::ImageWatch::WatchedImageHelpers
	::TryParseNeutralElFormatString(String^ string, int% vteltype)
{
	if (String::IsNullOrWhiteSpace(string))
		return false;

	auto ls = string->Trim()->ToUpper();

#define PFSCASE(str_, fmt_) if (ls == str_) { vteltype = fmt_; return true; }

	PFSCASE(L"UINT8", EL_FORMAT_BYTE);
	PFSCASE(L"INT8", EL_FORMAT_SBYTE);
	PFSCASE(L"UINT16", EL_FORMAT_SHORT);
	PFSCASE(L"INT16", EL_FORMAT_SSHORT);
	PFSCASE(L"INT32", EL_FORMAT_INT);
	PFSCASE(L"FLOAT16", EL_FORMAT_HALF_FLOAT);
	PFSCASE(L"FLOAT32", EL_FORMAT_FLOAT);
	PFSCASE(L"FLOAT64", EL_FORMAT_DOUBLE);

#undef PFSCASE

	return false;
}

bool Microsoft::ImageWatch::WatchedImageHelpers::TryParseUInt32(
	String^ s, UInt32% v)
{
	UInt64 v64 = 0;
	if (!TryParseUInt64(s, v64))
		return false;

	if (v64 > UInt32::MaxValue)
		return false;
	
	v = (UInt32)v64;

	return true;
}

bool Microsoft::ImageWatch::WatchedImageHelpers::TryParseUInt64(
	String^ s, UInt64% v)
{
	v = 0;

	if (s == nullptr)
		return false;

	s = s->Trim();

	if (s->ToLower()->StartsWith("0x"))
	{
		return UInt64::TryParse(s->Substring(2), 
			System::Globalization::NumberStyles::HexNumber, 
			System::Globalization::CultureInfo::InvariantCulture,
			v);
	}
	else
	{
		return UInt64::TryParse(s, v);
	}
}

bool Microsoft::ImageWatch::WatchedImageHelpers::TryParseInt32(
	String^ s, Int32% v)
{
	Int64 v64 = 0;
	if (!TryParseInt64(s, v64))
		return false;

	if (v64 < Int32::MinValue || v64 > Int32::MaxValue)
		return false;
	
	v = (Int32)v64;

	return true;
}

bool Microsoft::ImageWatch::WatchedImageHelpers::TryParseInt64(
	String^ s, Int64% v)
{
	v = 0;

	if (s == nullptr)
		return false;

	s = s->Trim();

	if (s->ToLower()->StartsWith("0x"))
	{
		return Int64::TryParse(s->Substring(2), 
			System::Globalization::NumberStyles::HexNumber, 
			System::Globalization::CultureInfo::InvariantCulture,
			v);
	}
	else
	{
		return Int64::TryParse(s, v);
	}
}

bool Microsoft::ImageWatch::WatchedImageHelpers::TryParseDouble(String^ s, 
																double% v)
{
	v = 0;

	if (s == nullptr)
		return false;

	s = s->Trim();

	return Double::TryParse(s, v);
}