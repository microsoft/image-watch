#pragma once

#include "WatchedImageFlipOpBase.h"

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageFlipHorizOp : WatchedImageFlipOpBase
		{
		public:
			virtual property int Mode { int get() override; }
		};

		public ref class WatchedImageFlipVertOp : WatchedImageFlipOpBase
		{
		public:
			virtual property int Mode { int get() override; }
		};

		public ref class WatchedImageTransposeOp : WatchedImageFlipOpBase
		{
		public:
			virtual property int Mode { int get() override; }
		};
		
		public ref class WatchedImageRot90Op : WatchedImageFlipOpBase
		{
		public:
			virtual property int Mode { int get() override; }
		};

		public ref class WatchedImageRot180Op : WatchedImageFlipOpBase
		{
		public:
			virtual property int Mode { int get() override; }
		};

		public ref class WatchedImageRot270Op : WatchedImageFlipOpBase
		{
		public:
			virtual property int Mode { int get() override; }
		};
	}
}