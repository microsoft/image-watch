#pragma once

namespace vt
{
	class IImageReaderWriter;
}

using namespace System;

namespace Microsoft 
{
	namespace ImageWatch
	{
		public ref class WatchedImageHelpers abstract sealed
		{
		public:
			static bool Is64BitProcess(
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				bool% is64bit);

			static bool FollowPointer(
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				UInt64 base, UInt64 ptrOffset, UInt64% target);

			static bool FollowTwoPointers(
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				UInt64 base, UInt64 ptrOffset1, UInt64 ptrOffset2, 
				UInt64% target);
			
			static bool EvaluateExpression(
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				String^ expression, String^% value, String^% type);

			static bool EvaluateExpression(
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				String^ expression, String^% value, String^% type, String^ format);

			static bool EvaluateExpression(
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				String^ expression, String^% value, String^% type, String^ format,
				bool getRawValue);

			static cli::array<Tuple<String^, String^>^>^ 
				GetEvaluatedExpressionChildrenInfo(
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				String^ expression);

			static bool GetPropertyChildValue(
				cli::array<Tuple<String^, String^>^>^ plist,
				String^ name, String^% value, String^% error);

			static bool EvaluateMemberUInt64(String^ objectPointerType, 
				UInt64 objectAddress, String^ name, 
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				UInt64% value);

			static bool EvaluateMemberUInt32(String^ objectPointerType, 
				UInt64 objectAddress, String^ name, 
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				UInt32% value);
			
			static bool EvaluateMemberInt64(String^ objectPointerType, 
				UInt64 objectAddress, String^ name, 
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				Int64% value);

			static bool EvaluateMemberInt32(String^ objectPointerType, 
				UInt64 objectAddress, String^ name, 
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				Int32% value);

			static bool EvaluateUInt32(
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				UInt64 address,
				UInt32% value);

			static bool EvaluateMemberAddress(String^ objectPointerType, 
				UInt64 objectAddress, String^ name,
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
				UInt64% address);

			static bool ReadMemory(
				Microsoft::VisualStudio::Debugger::DkmProcess^ process,
				UInt64 readPtr, void* buffer, UInt32 numBytesToRead);

			static bool LoadRemoteVTCImgInfo(int% width, int% height,
				int% type, UInt64% pixelAddress, int% numStrideBytes,
				UInt64 objectAddress, 
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame);

			static bool ValidateVTCImgInfo(int width, int height,
				int type, int numStrideBytes, UInt64 pixelAddress);

			static bool LoadRemoteVTCImg(vt::IImageReaderWriter* img, 
				UInt64 pixelAddress, int numStrideBytes, 
				Microsoft::VisualStudio::Debugger::DkmProcess^ process);

			static bool LoadRemoteVTCImgInCacheInfo(int% width, int% height, 
				int% type, UInt32% blockSizeX, UInt32% blockSizeY,
				UInt64% blocksAddress, UInt64% blocksSize, UInt64 objectAddress, 
				Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
				Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame);

			static bool ValidateVTCImgInCacheInfo(
				int type, UInt32 blockSizeX, UInt32 blockSizeY,
                UInt64 blocksAddress, UInt64 blocksSize);
			
			static bool LoadRemoteVTCImgInCache(vt::IImageReaderWriter* img, 
				UInt32 blockSizeX, UInt32 blockSizeY,
                UInt64 blocksAddress, UInt64 blocksSize,
                UInt32 * uniqueness,
				Microsoft::VisualStudio::Debugger::DkmProcess^ process);

			static int StartVTTaskManager();
			static void StopVTTaskManager();

			static String^ MakeNeutralElFormatString(int vttype);
			static bool TryParseNeutralElFormatString(String^ string, 
				int% vteltype);

			static bool TryParseUInt32(String^ s, UInt32% v);
			static bool TryParseUInt64(String^ s, UInt64% v);

			static bool TryParseInt32(String^ s, Int32% v);
			static bool TryParseInt64(String^ s, Int64% v);

			static bool TryParseDouble(String^ s, double% d);

		private:
			static Object^ BufferLock = gcnew Object();
			static bool VTTaskManagerStarted = false;

			// from Concord API
	        static Guid^ GuidFilterLocalsPlusArgs = gcnew Guid("e74721bb-10c0-40f5-807f-920d37f95419");
		};
	}
}