#pragma once

using namespace System;

namespace Microsoft
{
	namespace ImageWatch
	{
		public ref class ElFormatStringArg
		{
		public:
			static ElFormatStringArg^ Parse(String^ argString);

		public:
			UInt32 VtElType;

		private:
			ElFormatStringArg(UInt32 elType) : VtElType(elType) {};
		};
	}
}