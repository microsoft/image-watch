#include "vtcore.h"

#include "ElFormatStringArg.h"
#include "WatchedImageHelpers.h"

using namespace Microsoft::ImageWatch;

ElFormatStringArg^ ElFormatStringArg::Parse(String^ argString)
{
	if (argString == nullptr)
		return nullptr;

	int vtElType;
	if (!WatchedImageHelpers::TryParseNeutralElFormatString(argString, vtElType))
		return nullptr;

	return gcnew ElFormatStringArg(vtElType);
}