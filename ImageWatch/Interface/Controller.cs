using System;
using System.Collections.Generic;
using System.Linq;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Media;
using System.Windows.Threading;
using System.Diagnostics;

using EnvDTE;
using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.Evaluation;
using Microsoft.VisualStudio.Debugger.DefaultPort;
using Microsoft.VisualStudio.Debugger.Symbols;
using Microsoft.VisualStudio.Debugger.CallStack;

namespace Microsoft.ImageWatch.Interface
{
    public class WatchAddedEventArgs : EventArgs
    {
        public WatchListItem Item { get; set; }
    }
    
    public class Controller : BindableBase, IDisposable
    {
        const int TimeoutInMs = 5000;

        private EnvDTE.Events events_;
        private EnvDTE.DebuggerEvents dbgEvents_;
 
        public Controller()
        {
            // NOTE: need to keep references of BOTH these COM objects, 
            // otherwise event handler registration is deleted when event
            // object goes out of scope!
            System.Diagnostics.Debug.Assert(ImageWatchPackage.DTE != null);
            events_ = ImageWatchPackage.DTE.Events;
            dbgEvents_ = events_.DebuggerEvents;

            dbgEvents_.OnContextChanged += DebuggerEvents_OnContextChanged;
            dbgEvents_.OnEnterBreakMode += DebuggerEvents_OnEnterBreakMode;
            dbgEvents_.OnEnterRunMode += DebuggerEvents_OnEnterRunMode;
            dbgEvents_.OnEnterDesignMode += DebuggerEvents_OnEnterDesignMode;
                        
            ImageWatchPackage.AddWatchService.Add += AddWatchService_Add;

            runModeTimer.Interval = TimeSpan.FromSeconds(1.0);
            runModeTimer.Tick += runModeTimer_Tick;
        }

        public void FirstTimeEnterBreakMode()
        {
            ImageWatchPackage.InitializeTypes();

            var dbg = ImageWatchPackage.DTE.Debugger;
            if (dbg != null &&
                dbg.CurrentMode == EnvDTE.dbgDebugMode.dbgBreakMode)
            {
                SetDebuggingContext();
                OnEnterBreakMode();
            }
        }

        private DkmProcess process_;
        public DkmProcess Process
        {
            get
            {
                return process_;
            }
        }

        private DkmInspectionSession inspectionSession_;
        private DkmInspectionContext inspectionContext_;
        private DkmStackWalkFrame frame_;

        public DkmInspectionContext InspectionContext
        {
            get
            {
                return inspectionContext_;
            }
        }

        public DkmStackWalkFrame Frame
        {
            get
            {
                return frame_;
            }
        }

        private void ClearDebuggingContext()
        {
            inspectionSession_?.Close();
            inspectionSession_ = null;
            inspectionContext_ = null;
            frame_ = null;
        }

        private void SetDebuggingContext()
        {
            process_ = null;
            inspectionContext_ = null;
            frame_ = null;

            try
            {
                var sf = ImageWatchPackage.DTE.Debugger.CurrentStackFrame;
                if (sf == null)
                    return;

                var sf2 = VisualStudio.Debugger.CallStack.DkmStackFrame.ExtractFromDTEObject(sf);
                if (sf2 == null)
                    return;

                Debug.Assert(sf2.Process != null, "DkmProcess is null");
                process_ = sf2.Process;

                var instructionAddress = sf2.InstructionAddress;
                if (instructionAddress == null)
                {
                    // Current stack frame does not support inspection
                    return;
                }

                var language = sf2.Process.EngineSettings.GetLanguage(sf2.CompilerId);

                DkmWorkerProcessConnection symbolsConnection = null;
                DkmInstructionSymbol instructionSymbol = instructionAddress.GetSymbol();
                if (instructionSymbol != null)
                {
                    symbolsConnection = instructionSymbol.Module.SymbolsConnection;
                }
                else if (instructionAddress.RuntimeInstance.Id.RuntimeType == DkmRuntimeId.Native)
                {
                    try
                    {
                        symbolsConnection = DkmWorkerProcessConnection.GetLocalSymbolsConnection();
                    }
                    catch
                    {
                        // Ignore failures obtaining the symbols connection since we can evaluate in the IDE process.
                    }
                }

                inspectionSession_ = DkmInspectionSession.Create(process_, null);
                inspectionContext_ = DkmInspectionContext.Create(inspectionSession_, instructionAddress.RuntimeInstance, sf2.Thread, TimeoutInMs,
                    DkmEvaluationFlags.TreatAsExpression | DkmEvaluationFlags.NoSideEffects | DkmEvaluationFlags.ForceRealFuncEval,
                    DkmFuncEvalFlags.None, Radix: 10, language, ReturnValue: null, AdditionalVisualizationData: null,
                    AdditionalVisualizationDataPriority: DkmCompiledVisualizationDataPriority.None, ReturnValues: null, symbolsConnection);
                frame_ = sf2;
            }
            catch (Exception)
            {
                process_ = null;
                inspectionSession_?.Close();
                inspectionSession_ = null;
                inspectionContext_ = null;
                frame_ = null;
            }
        }

        private string errorState_ = "";
        public string ErrorState 
        {
            get
            {
                return errorState_;
            }
            set 
            {
                SetProperty(ref errorState_, value);
            }
        }
        
        private bool isVSInBreakMode_ = false;
        public bool IsVSInBreakMode
        {
            get
            {
                return isVSInBreakMode_;
            }
            set
            {
                SetProperty(ref isVSInBreakMode_, value);
            }
        }

        private bool isVSOutOfBreakModeForLonger_ = true;
        public bool IsVSOutOfBreakModeForLonger
        {
            get
            {
                return isVSOutOfBreakModeForLonger_;
            }
            set
            {
                SetProperty(ref isVSOutOfBreakModeForLonger_, value);
            }
        }

        private DispatcherTimer runModeTimer = new DispatcherTimer();

        WatchListItem MakeNewItem(string expression, bool activate)
        { 
            var item = new WatchListItem(expression) 
            {
                IsActive = activate,
                IsExpanded = ExpandNewItems,                
            };

            item.SetThumbnailProperties(ThumbnailWidth, ThumbnailHeight,
                BitmapBackground);
            item.SetAutoScaleColormap(AutoScaleColormap);
            item.SetColormapJet(ColormapJet);
            item.SetFourChannelsIgnoreAlpha(FourChannelIgnoreAlpha);

            return item;
        }

        public void AddToWatchList(string expression)
        {
            if (!string.IsNullOrWhiteSpace(expression))
            {
                watchList_.Add(MakeNewItem(expression, true));

                OnWatchAdded(watchList_.Last());
            }
        }

        public void AddAddressToWatchList(string expression)
        {
            if (ExpressionHelper.IsOperator(expression))
                return;

            var ei = ExpressionHelper.ParseImage(expression);
            if (ei == null)
                return;

            if (!typeof(WatchedImageImage).IsAssignableFrom(
                WatchedImageTypeMap.GetImageType(ei.Type)))
                return;

            if (ei.Args.Count < 2 || !(ei.Args[0] is string) ||
                !(ei.Args[1] is UInt64))
                return;

            var addr = (UInt64)ei.Args[1];
            var len = addr > 0xffffffff ? "16" : "8";
            var expr = string.Format("({0}) 0x{1:x" + len + "}", 
                ei.Args[0], addr);

            AddToWatchList(expr);
        }

        private bool expandNewItems_ = true;
        public bool ExpandNewItems
        {
            get 
            {
                return expandNewItems_;
            }
            set 
            {
                SetProperty(ref expandNewItems_, value);
            }
        }

        private bool showLargeThumbnails_ = false;
        public bool ShowLargeThumbnails
        {
            get
            {
                return showLargeThumbnails_;
            }
            set
            {
                if (SetProperty(ref showLargeThumbnails_, value))
                    ResizeAndRefreshThumbnails();
            }
        }

        private int ThumbnailWidth
        {
            get
            {
                return ShowLargeThumbnails ? 120 : 60;
            }
        }

        private int ThumbnailHeight
        {
            get
            {
                return ShowLargeThumbnails ? 90 : 45;
            }
        }

        private Color BitmapBackground
        { 
            get
            {
                var bgBrush = (MyToolWindow.FindResource("ImageViewBackground")
                    as SolidColorBrush);

                return bgBrush != null ? bgBrush.Color : Colors.Black;
            }
        }

        public void ResizeAndRefreshThumbnails()
        {
            foreach (WatchListItem item in localsList_)
            {
                item.SetThumbnailProperties(ThumbnailWidth, ThumbnailHeight,
                    BitmapBackground);
                item.NotifyAllPropertiesChanged();
            }

            foreach (WatchListItem item in watchList_)
            {
                item.SetThumbnailProperties(ThumbnailWidth, ThumbnailHeight,
                    BitmapBackground);
                item.NotifyAllPropertiesChanged();
            }
        }

        private bool autoScaleColormap_ = false;
        public bool AutoScaleColormap
        {
            get
            {
                return autoScaleColormap_;
            }
            set
            {
                if (SetProperty(ref autoScaleColormap_, value))
                {
                    foreach (var item in LocalsList.Concat(WatchList))
                        item.SetAutoScaleColormap(value);

                    RefreshActiveList();
                }
            }
        }

        private bool colormapJet_ = false;
        public bool ColormapJet
        {
            get
            {
                return colormapJet_;
            }
            set
            {
                if (SetProperty(ref colormapJet_, value))
                {
                    foreach (var item in LocalsList.Concat(WatchList))
                        item.SetColormapJet(value);

                    RefreshActiveList();
                }
            }
        }

        private bool fourChannelIgnoreAlpha_ = false;
        public bool FourChannelIgnoreAlpha
        {
            get
            {
                return fourChannelIgnoreAlpha_;
            }
            set
            {
                if (SetProperty(ref fourChannelIgnoreAlpha_, value))
                {
                    foreach (var item in LocalsList.Concat(WatchList))
                        item.SetFourChannelsIgnoreAlpha(value);

                    RefreshActiveList();
                }
            }
        }

        private bool linkViews_ = false;
        public bool LinkViews
        {
            get
            {
                return linkViews_;
            }
            set
            {
                SetProperty(ref linkViews_, value);
            }
        }

        private WatchListItemView view_ = new WatchListItemView();
        public void InitializeView(int width, int height)
        {
            view_.Initialize(width, height, BitmapBackground, true);
            view_.FitScaleFactor = 0.97;
            view_.EnableGrid(true);
        }

        public void AttachViewToItem(WatchListItem vi)
        {
            Point viewCenter = new Point();
            double viewScale = 1.0;
            Size viewImageSize = new Size();
            bool wasAttached = false;
            foreach (var item in WatchList.Concat(LocalsList))
            {
                if (item.DetachView(ref viewCenter, ref viewScale,
                    ref viewImageSize))
                    wasAttached = true;
            }

            if (wasAttached && LinkViews)
            {
                foreach (var item in WatchList.Concat(LocalsList))
                {
                    item.ModifyViewTransformIfSizeMatches(viewCenter,
                        viewScale, viewImageSize);
                }
            }

            if (vi != null)
                vi.AttachView(view_);
        }

        private ObservableCollection<WatchListItem> watchList_ =
            new ObservableCollection<WatchListItem>();
        public ObservableCollection<WatchListItem> WatchList
        {
            get
            {
                return watchList_;
            }
            private set
            {
                SetProperty(ref watchList_, value);
            }
        }

        public event EventHandler WatchAdded;
        private void OnWatchAdded(WatchListItem item)
        {
            if (WatchAdded != null)
            {
                WatchAdded(this, new WatchAddedEventArgs()
                {
                    Item = item
                });
            }
        }
        
        private bool localsSelected_ = true;
        public bool LocalsSelected
        {
            get
            {
                return localsSelected_;
            }
            set
            {
                if (SetProperty(ref localsSelected_, value))
                    RefreshActiveList();
            }
        }

        public event EventHandler PreLocalsUpdated;
        private void OnPreLocalsUpdated()
        {
            if (PreLocalsUpdated != null)
                PreLocalsUpdated(this, EventArgs.Empty);
        }

        public event EventHandler LocalsUpdated;
        private void OnLocalsUpdated()
        {
            if (LocalsUpdated != null)
                LocalsUpdated(this, EventArgs.Empty);                        
        }
        
        private ObservableCollection<WatchListItem> localsList_ =
           new ObservableCollection<WatchListItem>();
        public ObservableCollection<WatchListItem> LocalsList
        {
            get
            {
                return localsList_;
            }
            private set
            {
                SetProperty(ref localsList_, value);
            }
        }

        void UpdateLocalsList()
        {
            if (ImageWatchPackage.DTE == null)
                return;

            List<string> newItems = new List<string>();
            foreach (EnvDTE.Expression expr in
                ImageWatchPackage.DTE.Debugger.CurrentStackFrame.Locals)
            {
                if (WatchedImageTypeMap.GetImageType(
                    expr.Type.TrimEnd(" &*^".ToCharArray())) != null)
                {
                    // suppress VS return values in locals window and duplicate
                    // locals (happens in release mode debugging sometimes)
                    if (!expr.Name.EndsWith(" returned") &&
                        !newItems.Contains(expr.Name))
                    {
                        newItems.Add(expr.Name);
                    }
                }
            }

            var oldItems = localsList_.ToDictionary(i => i.Expression,
                i => i);
            var currentWatchList = watchList_.ToDictionary(i => i.Expression, i => i);

            OnPreLocalsUpdated();

            localsList_.Clear();
            foreach (var i in newItems)
            {
                if (oldItems.ContainsKey(i))
                {
                    localsList_.Add(oldItems[i]);
                    oldItems.Remove(i);
                }
                else
                {
                    localsList_.Add(MakeNewItem(i, false));
                }
            }

            OnLocalsUpdated();
            var localListDict = localsList_.ToDictionary(i => i.Expression, i => i);
            foreach (var i in currentWatchList)
            {
                if (oldItems.ContainsKey(i.Key))
                {
                    i.Value.IsInScope = false;
                }
                else if (localListDict.ContainsKey(i.Key))
                {
                    i.Value.IsInScope = true;
                }
            }
            foreach (var i in oldItems)
                i.Value.Dispose();
        }

        void AddWatchService_Add(object sender, EventArgs e)
        {
            var args = e as ImageWatchAddWatchEventArgs;
            if (args == null)
                return;

            AddToWatchList(args.Expression);
        }

        void runModeTimer_Tick(object sender, EventArgs e)
        {
            IsVSOutOfBreakModeForLonger = true;
            runModeTimer.Stop();
        }

        private void DebuggerEvents_OnEnterRunMode(dbgEventReason Reason)
        {
            if (Reason == EnvDTE.dbgEventReason.dbgEventReasonLaunchProgram ||
                Reason == EnvDTE.dbgEventReason.dbgEventReasonAttachProgram ||
                Reason == EnvDTE.dbgEventReason.dbgEventReasonGo)
            {
#if DEBUG
                System.Diagnostics.Debug.WriteLine("%% Entering run mode: {0}",
                    Reason);
#endif

                ImageWatchPackage.InitializeTypes();
            }

            ClearDebuggingContext();
            IsVSInBreakMode = false;
            runModeTimer.Start();
        }

        void RefreshActiveList()
        {
            var activeList = LocalsSelected ? LocalsList : WatchList;
            var inactiveList = LocalsSelected ? WatchList : LocalsList;

            foreach (var item in activeList)
            {
                item.IsActive = true;
                item.NotifyAllPropertiesChanged();
            }

            foreach (var item in inactiveList)
                item.IsActive = false;
        }

        void OnEnterBreakMode()
        {
            runModeTimer.Stop();
            IsVSOutOfBreakModeForLonger = false;
            IsVSInBreakMode = true;

            foreach (var item in WatchList)
                item.MarkExpressionOutOfDate();

            UpdateLocalsList();

            foreach (var item in LocalsList)
                item.MarkExpressionOutOfDate();

            RefreshActiveList();
        }

        private void DebuggerEvents_OnEnterBreakMode(dbgEventReason Reason, ref dbgExecutionAction ExecutionAction)
        {
            SetDebuggingContext();
            OnEnterBreakMode();
        }

        private void DebuggerEvents_OnContextChanged(EnvDTE.Process NewProcess, EnvDTE.Program NewProgram, EnvDTE.Thread NewThread, EnvDTE.StackFrame NewStackFrame)
        {
            // this gets also called after Shift-F5 (stop debugging),
            // with NewStackFrame=null
            if (NewStackFrame == null)
                return;

            SetDebuggingContext();

            foreach (var item in WatchList)
                item.MarkContextOutOfDate();

            UpdateLocalsList();

            foreach (var item in LocalsList)
                item.MarkContextOutOfDate();

            RefreshActiveList();
        }

        private void DebuggerEvents_OnEnterDesignMode(dbgEventReason Reason)
        {
            ClearDebuggingContext();

            foreach (var item in WatchList)
            {
                // free resources but keep expessions in list
                item.MarkInfoOutOfDate();
                item.Dispose();
            }

            foreach (var item in LocalsList)
                item.Dispose();
            LocalsList.Clear();

            IsVSOutOfBreakModeForLonger = true;
        }

        public void Dispose()
        {
            Dispose(true);

            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                foreach (var item in WatchList)
                    item.Dispose();
                foreach (var item in LocalsList)
                    item.Dispose();

                if (view_ != null)
                    view_.Dispose();
            }
        }
    }
}
